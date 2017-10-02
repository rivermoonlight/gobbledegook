// Copyright 2017 Paul Nettle.
//
// This file is part of Gobbledegook.
//
// Gobbledegook is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Gobbledegook is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Gobbledegook.  If not, see <http://www.gnu.org/licenses/>.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// >>
// >>>  INSIDE THIS FILE
// >>
//
// Herein lies the code that manages the full initialization (including the running) of the server
//
// >>
// >>>  DISCUSSION
// >>
//
// This file contains the highest-level framework for our server:
//
//    Initialization
//    Adapter configuration (mode, settings, name, etc.)
//    GATT server registration with BlueZ
//    Signal handling (such as CTRL-C)
//    Event management
//    Graceful shutdown
//
// Want to poke around and see how things work? Here's a tip: Start at the bottom of the file and work upwards. It'll make a lot
// more sense, I promise.
//
// Want to become your own boss while working from home? (Just kidding.)
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gio/gio.h>
#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <thread>

#include "Server.h"
#include "Globals.h"
#include "Mgmt.h"
#include "HciAdapter.h"
#include "DBusObject.h"
#include "DBusInterface.h"
#include "GattCharacteristic.h"
#include "GattProperty.h"
#include "Logger.h"
#include "Init.h"

namespace ggk {

//
// Constants
//

static const int kPeriodicTimerFrequencySeconds = 1;
static const int kRetryDelaySeconds = 2;
static const int kIdleFrequencyMS = 10;

//
// Retries
//

static time_t retryTimeStart = 0;

//
// Adapter configuration
//

GDBusConnection *pBusConnection = nullptr;
static guint ownedNameId = 0;
static guint periodicTimeoutId = 0;
static std::vector<guint> registeredObjectIds;
static std::atomic<GMainLoop *> pMainLoop(nullptr);
static GDBusObjectManager *pBluezObjectManager = nullptr;
static GDBusObject *pBluezAdapterObject = nullptr;
static GDBusObject *pBluezDeviceObject = nullptr;
static GDBusProxy *pBluezGattManagerProxy = nullptr;
static GDBusProxy *pBluezAdapterInterfaceProxy = nullptr;
static GDBusProxy *pBluezDeviceInterfaceProxy = nullptr;
static GDBusProxy *pBluezAdapterPropertiesInterfaceProxy = nullptr;
static bool bOwnedNameAcquired = false;
static bool bAdapterConfigured = false;
static bool bApplicationRegistered = false;
static std::string bluezGattManagerInterfaceName = "";

//
// Externs
//

extern void setServerRunState(enum GGKServerRunState newState);
extern void setServerHealth(enum GGKServerHealth newHealth);

//
// Forward declarations
//

static void initializationStateProcessor();

// ---------------------------------------------------------------------------------------------------------------------------------
//  ___    _ _           __      _       _                                             _
// |_ _|__| | | ___     / /   __| | __ _| |_ __ _    _ __  _ __ ___   ___ ___  ___ ___(_)_ __   __ _
//  | |/ _` | |/ _ \   / /   / _` |/ _` | __/ _` |  | '_ \| '__/ _ \ / __/ _ \/ __/ __| | '_ \ / _` |
//  | | (_| | |  __/  / /   | (_| | (_| | || (_| |  | |_) | | | (_) | (_|  __/\__ \__ \ | | | | (_| |
// |___\__,_|_|\___| /_/     \__,_|\__,_|\__\__,_|  | .__/|_|  \___/ \___\___||___/___/_|_| |_|\__, |
//                                                  |_|                                        |___/
//
// Our idle funciton is what processes data updates. We handle this in a simple way. We update the data directly in our global
// `TheServer` object, then call `ggkPushUpdateQueue` to trigger that data to be updated (in whatever way the service responsible
// for that data() sees fit.
//
// This is done using the `ggkPushUpdateQueue` / `ggkPopUpdateQueue` methods to manage the queue of pending update messages. Each
// entry represents an interface that needs to be updated. The idleFunc calls the interface's `onUpdatedValue` method for each
// update.
//
// The idle processor will perform one update per idle tick, however, it will notify that there is more data so the idle ticks
// do not lag behind.
// ---------------------------------------------------------------------------------------------------------------------------------

// Our idle function
//
// This method is used to process data on the same thread as our main loop. This allows us to communicate with our service from
// the outside.
//
// IMPORTANT: This method must return 'true' if any work was performed, otherwise it must return 'false'. Returning 'true' will
// cause the idle loop to continue to call this method to process data at the maximum rate (which can peg the CPU at 100%.) By
// returning false when there is no work to do, we are nicer to the system.
bool idleFunc(void *pUserData)
{

	// Don't do anything unless we're running
	if (ggkGetServerRunState() != ERunning)
	{
		return false;
	}

	// Try to get an update
	const int kQueueEntryLen = 1024;
	char queueEntry[kQueueEntryLen];
	if (ggkPopUpdateQueue(queueEntry, kQueueEntryLen, 0) != 1)
	{
		return false;
	}

	std::string entryString = queueEntry;
	auto token = entryString.find('|');
	if (token == std::string::npos)
	{
		Logger::error("Queue entry was not formatted properly - could not find separating token");
		return false;
	}

	DBusObjectPath objectPath = DBusObjectPath(entryString.substr(0, token));
	std::string interfaceName = entryString.substr(token+1);

	// We have an update - call the onUpdatedValue method on the interface
	std::shared_ptr<const DBusInterface> pInterface = TheServer->findInterface(objectPath, interfaceName);
	if (nullptr == pInterface)
	{
		Logger::warn(SSTR << "Unable to find interface for update: path[" << objectPath << "], name[" << interfaceName << "]");
	}
	else
	{
		// Is it a characteristic?
		if (std::shared_ptr<const GattCharacteristic> pCharacteristic = TRY_GET_CONST_INTERFACE_OF_TYPE(pInterface, GattCharacteristic))
		{
			Logger::debug(SSTR << "Processing updated value for interface '" << interfaceName << "' at path '" << objectPath << "'");
			pCharacteristic->callOnUpdatedValue(pBusConnection, pUserData);
			return true;
		}
	}

	return false;
}

// ---------------------------------------------------------------------------------------------------------------------------------
//  ____       _       _ _   _       _ _          _   _
// |  _ \  ___(_)_ __ (_) |_(_) __ _| (_)______ _| |_(_) ___  _ ___
// | | | |/ _ \ | '_ \| | __| |/ _` | | |_  / _` | __| |/ _ \| '_  |
// | |_| |  __/ | | | | | |_| | (_| | | |/ / (_| | |_| | (_) | | | |
// |____/ \___|_|_| |_|_|\__|_|\__,_|_|_/___\__,_|\__|_|\___/|_| |_|
//
//
// ---------------------------------------------------------------------------------------------------------------------------------

// Perform final cleanup of various resources that were allocated while the server was initialized and/or running
void uninit()
{
  	// We've left our main loop - nullify its pointer so we know we're no longer running
  	pMainLoop = nullptr;

	if (nullptr != pBluezAdapterObject)
	{
		g_object_unref(pBluezAdapterObject);
		pBluezAdapterObject = nullptr;
	}

	if (nullptr != pBluezDeviceObject)
	{
		g_object_unref(pBluezDeviceObject);
		pBluezDeviceObject = nullptr;
	}

	if (nullptr != pBluezAdapterInterfaceProxy)
	{
		g_object_unref(pBluezAdapterInterfaceProxy);
		pBluezAdapterInterfaceProxy = nullptr;
	}

	if (nullptr != pBluezDeviceInterfaceProxy)
	{
		g_object_unref(pBluezDeviceInterfaceProxy);
		pBluezDeviceInterfaceProxy = nullptr;
	}

	if (nullptr != pBluezAdapterPropertiesInterfaceProxy)
	{
		g_object_unref(pBluezAdapterPropertiesInterfaceProxy);
		pBluezAdapterPropertiesInterfaceProxy = nullptr;
	}

	if (nullptr != pBluezGattManagerProxy)
	{
		g_object_unref(pBluezGattManagerProxy);
		pBluezGattManagerProxy = nullptr;
	}

	if (nullptr != pBluezObjectManager)
	{
		g_object_unref(pBluezObjectManager);
		pBluezObjectManager = nullptr;
	}

	if (!registeredObjectIds.empty())
	{
		for (guint id : registeredObjectIds)
		{
			g_dbus_connection_unregister_object(pBusConnection, id);
		}
		registeredObjectIds.clear();
	}

	if (0 != periodicTimeoutId)
	{
		g_source_remove(periodicTimeoutId);
		periodicTimeoutId = 0;
	}

  	if (ownedNameId > 0)
  	{
		g_bus_unown_name(ownedNameId);
	}

	if (nullptr != pBusConnection)
	{
		g_object_unref(pBusConnection);
		pBusConnection = nullptr;
	}

	if (nullptr != pMainLoop)
	{
		g_main_loop_unref(pMainLoop);
		pMainLoop = nullptr;
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------
//  ____  _           _      _
// / ___|| |__  _   _| |_ __| | _____      ___ ___
// \___ \| '_ \| | | | __/ _` |/ _ \ \ /\ / / '_  |
//  ___) | | | | |_| | || (_| | (_) \ V  V /| | | |
// |____/|_| |_|\__,_|\__\__,_|\___/ \_/\_/ |_| |_|
//
// This is how we shutdown our server gracefully.
// ---------------------------------------------------------------------------------------------------------------------------------

// Trigger a graceful, asynchronous shutdown of the server
//
// This method is non-blocking and as such, will only trigger the shutdown process but not wait for it
void shutdown()
{
	if (ggkGetServerRunState() > ERunning)
	{
		Logger::warn("Ignoring call to shutdown (we are already shutting down)");
		return;
	}

	// Our new state: shutting down
	setServerRunState(EStopping);

	// Stop our HciAdapter
	HciAdapter::getInstance().stop();

	// If we still have a main loop, ask it to quit
	if (nullptr != pMainLoop)
	{
		g_main_loop_quit(pMainLoop);
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------
//  ____           _           _ _        _   _
// |  _ \ ___ _ __(_) ___   __| (_) ___  | |_(_)_ __ ___   ___ _ __
// | |_) / _ \ '__| |/ _ \ / _` | |/ __| | __| | '_ ` _ \ / _ \ '__|
// |  __/  __/ |  | | (_) | (_| | | (__  | |_| | | | | | |  __/ |
// |_|   \___|_|  |_|\___/ \__,_|_|\___|  \__|_|_| |_| |_|\___|_|
//
// ---------------------------------------------------------------------------------------------------------------------------------

// Periodic timer handler
//
// A periodic timer is a timer fires every so often (see kPeriodicTimerFrequencySeconds.) This is used for our initialization
// failure retries, but custom code can also be added to a server description (see `onEvent()`)
gboolean onPeriodicTimer(gpointer pUserData)
{
	// If we're shutting down, don't do anything and stop the periodic timer
	if (ggkGetServerRunState() > ERunning)
	{
		return FALSE;
	}

	// Deal with retry timers
	if (0 != retryTimeStart)
	{
		Logger::debug(SSTR << "Ticking retry timer");

		// Has the retry time expired?
		int secondsRemaining = time(nullptr) - retryTimeStart - kRetryDelaySeconds;
		if (secondsRemaining >= 0)
		{
			retryTimeStart = 0;
			initializationStateProcessor();
		}
	}

	// If we're registered, then go ahead and emit signals
	if (bApplicationRegistered)
	{
		// Tick the object hierarchy
		//
		// The real goal here is to have the objects tick their interfaces (see `onEvent()` method when adding interfaces inside
		// 'Server::Server()'
		for (const DBusObject &object : TheServer->getObjects())
		{
			if (object.isPublished())
			{
				object.tickEvents(pBusConnection, pUserData);
			}
		}
	}

	return TRUE;
}

// ---------------------------------------------------------------------------------------------------------------------------------
//  _____                 _
// | ____|_   _____ _ __ | |_ ___
// |  _| \ \ / / _ \ '_ \| __/ __|
// | |___ \ V /  __/ | | | |_\__ )
// |_____| \_/ \___|_| |_|\__|___/
//
// Our event handlers. These are generic, as they parcel out the work to the appropriate server objects (see 'Server::Server()' for
// the code that manages event handlers.)
// ---------------------------------------------------------------------------------------------------------------------------------

// Handle D-Bus method calls
void onMethodCall
(
	GDBusConnection *pConnection,
	const gchar *pSender,
	const gchar *pObjectPath,
	const gchar *pInterfaceName,
	const gchar *pMethodName,
	GVariant *pParameters,
	GDBusMethodInvocation *pInvocation,
	gpointer pUserData
)
{
	// Convert our input path into our custom type for path management
	DBusObjectPath objectPath(pObjectPath);

	if (!TheServer->callMethod(objectPath, pInterfaceName, pMethodName, pConnection, pParameters, pInvocation, pUserData))
	{
		Logger::error(SSTR << " + Method not found: [" << pSender << "]:[" << objectPath << "]:[" << pInterfaceName << "]:[" << pMethodName << "]");
		g_dbus_method_invocation_return_dbus_error(pInvocation, kErrorNotImplemented.c_str(), "This method is not implemented");
		return;
	}

	return;
}

// Handle D-Bus requests to get a property
GVariant *onGetProperty
(
	GDBusConnection  *pConnection,
	const gchar      *pSender,
	const gchar      *pObjectPath,
	const gchar      *pInterfaceName,
	const gchar      *pPropertyName,
	GError           **ppError,
	gpointer         pUserData
)
{
	// Convert our input path into our custom type for path management
	DBusObjectPath objectPath(pObjectPath);

	const GattProperty *pProperty = TheServer->findProperty(objectPath, pInterfaceName, pPropertyName);

	std::string propertyPath = std::string("[") + pSender + "]:[" + objectPath.toString() + "]:[" + pInterfaceName + "]:[" + pPropertyName + "]";
	if (!pProperty)
	{
		Logger::error(SSTR << "Property(get) not found: " << propertyPath);
	    g_set_error(ppError, G_IO_ERROR, G_IO_ERROR_FAILED, ("Property(get) not found: " + propertyPath).c_str(), pSender);
		return nullptr;
	}

	if (!pProperty->getGetterFunc())
	{
		Logger::error(SSTR << "Property(get) func not found: " << propertyPath);
	    g_set_error(ppError, G_IO_ERROR, G_IO_ERROR_FAILED, ("Property(get) func not found: " + propertyPath).c_str(), pSender);
		return nullptr;
	}

	Logger::info(SSTR << "Calling property getter: " << propertyPath);
	GVariant *pResult = pProperty->getGetterFunc()(pConnection, pSender, objectPath.c_str(), pInterfaceName, pPropertyName, ppError, pUserData);

	if (nullptr == pResult)
	{
	    g_set_error(ppError, G_IO_ERROR, G_IO_ERROR_FAILED, ("Property(get) failed: " + propertyPath).c_str(), pSender);
	    return nullptr;
	}

	return pResult;
}

// Handle D-Bus requests to set a property
gboolean onSetProperty
(
	GDBusConnection  *pConnection,
	const gchar      *pSender,
	const gchar      *pObjectPath,
	const gchar      *pInterfaceName,
	const gchar      *pPropertyName,
	GVariant         *pValue,
	GError           **ppError,
	gpointer         pUserData
)
{
	// Convert our input path into our custom type for path management
	DBusObjectPath objectPath(pObjectPath);

	const GattProperty *pProperty = TheServer->findProperty(objectPath, pInterfaceName, pPropertyName);

	std::string propertyPath = std::string("[") + pSender + "]:[" + objectPath.toString() + "]:[" + pInterfaceName + "]:[" + pPropertyName + "]";
	if (!pProperty)
	{
		Logger::error(SSTR << "Property(set) not found: " << propertyPath);
	    g_set_error(ppError, G_IO_ERROR, G_IO_ERROR_FAILED, ("Property(set) not found: " + propertyPath).c_str(), pSender);
		return false;
	}

	if (!pProperty->getSetterFunc())
	{
		Logger::error(SSTR << "Property(set) func not found: " << propertyPath);
	    g_set_error(ppError, G_IO_ERROR, G_IO_ERROR_FAILED, ("Property(set) func not found: " + propertyPath).c_str(), pSender);
		return false;
	}

	Logger::info(SSTR << "Calling property getter: " << propertyPath);
	if (!pProperty->getSetterFunc()(pConnection, pSender, objectPath.c_str(), pInterfaceName, pPropertyName, pValue, ppError, pUserData))
	{
	    g_set_error(ppError, G_IO_ERROR, G_IO_ERROR_FAILED, ("Property(set) failed: " + propertyPath).c_str(), pSender);
	    return false;
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------------------
//  _____     _ _                                                                              _
// |  ___|_ _(_) |_   _ _ __ ___   _ __ ___   __ _ _ __   __ _  __ _  ___ _ __ ___   ___ _ __ | |_
// | |_ / _` | | | | | | '__/ _ \ | '_ ` _ \ / _` | '_ \ / _` |/ _` |/ _ \ '_ ` _ \ / _ \ '_ \| __|
// |  _| (_| | | | |_| | | |  __/ | | | | | | (_| | | | | (_| | (_| |  __/ | | | | |  __/ | | | |_
// |_|  \__,_|_|_|\__,_|_|  \___| |_| |_| |_|\__,_|_| |_|\__,_|\__, |\___|_| |_| |_|\___|_| |_|\__|
//                                                             |___/
//
// ---------------------------------------------------------------------------------------------------------------------------------

// Convenience method for setting a retry timer so that operations can be continuously retried until we eventually succeed
void setRetry()
{
	retryTimeStart = time(nullptr);
}

// Convenience method for setting a retry timer so that failures (related to initialization) can be continuously retried until we
// eventually succeed.
void setRetryFailure()
{
	setRetry();
	Logger::warn(SSTR << "  + Will retry the failed operation in about " << kRetryDelaySeconds << " seconds");
}

// ---------------------------------------------------------------------------------------------------------------------------------
//   ____    _  _____ _____                  _     _             _   _
//  / ___|  / \|_   _|_   _|  _ __ ___  __ _(_)___| |_ _ __ __ _| |_(_) ___  _ ___
// | |  _  / _ \ | |   | |   | '__/ _ \/ _` | / __| __| '__/ _` | __| |/ _ \| '_  |
// | |_| |/ ___ \| |   | |   | | |  __/ (_| | \__ \ |_| | | (_| | |_| | (_) | | | |
//  \____/_/   \_\_|   |_|   |_|  \___|\__, |_|___/\__|_|  \__,_|\__|_|\___/|_| |_|
//                                     |___/
//
// ---------------------------------------------------------------------------------------------------------------------------------

// Use the BlueZ GATT Manager proxy to register our GATT application with BlueZ
void doRegisterApplication()
{
	g_auto(GVariantBuilder) builder;
	g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
	GVariant *pParams = g_variant_new("(oa{sv})", "/", &builder);

	g_dbus_proxy_call
	(
		pBluezGattManagerProxy,         // GDBusProxy *proxy
		"RegisterApplication",          // const gchar *method_name   (ex: "GetManagedObjects")
		pParams,                        // GVariant *parameters
		G_DBUS_CALL_FLAGS_NONE,         // GDBusCallFlags flags
		-1,                             // gint timeout_msec
		nullptr,                        // GCancellable *cancellable

		// GAsyncReadyCallback callback
		[] (GObject * /*pSourceObject*/, GAsyncResult *pAsyncResult, gpointer /*pUserData*/)
		{
			GError *pError = nullptr;
			GVariant *pVariant = g_dbus_proxy_call_finish(pBluezGattManagerProxy, pAsyncResult, &pError);
			if (nullptr == pVariant)
			{
				Logger::error(SSTR << "Failed to register application: " << (nullptr == pError ? "Unknown" : pError->message));
				setRetryFailure();
			}
			else
			{
				g_variant_unref(pVariant);
				Logger::debug(SSTR << "GATT application registered with BlueZ");
				bApplicationRegistered = true;
			}

			// Keep going...
			initializationStateProcessor();
		},

		nullptr                         // gpointer user_data
	);
}

// ---------------------------------------------------------------------------------------------------------------------------------
//   ___  _     _           _                    _     _             _   _
//  / _ \| |__ (_) ___  ___| |_   _ __ ___  __ _(_)___| |_ _ __ __ _| |_(_) ___  _ ___
// | | | | '_ \| |/ _ \/ __| __| | '__/ _ \/ _` | / __| __| '__/ _` | __| |/ _ \| '_  |
// | |_| | |_) | |  __/ (__| |_  | | |  __/ (_| | \__ \ |_| | | (_| | |_| | (_) | | | |
//  \___/|_.__// |\___|\___|\__| |_|  \___|\__, |_|___/\__|_|  \__,_|\__|_|\___/|_| |_|
//           |__/                          |___/
//
// Before we can register our service(s) with BlueZ, we must first register ourselves with D-Bus. The easiest way to do this is to
// use an XML description of our D-Bus objects.
// ---------------------------------------------------------------------------------------------------------------------------------

void registerNodeHierarchy(GDBusNodeInfo *pNode, const DBusObjectPath &basePath = DBusObjectPath(), int depth = 1)
{
	std::string prefix;
	prefix.insert(0, depth * 2, ' ');

	static GDBusInterfaceVTable interfaceVtable;
	interfaceVtable.method_call = onMethodCall;
	interfaceVtable.get_property = onGetProperty;
	interfaceVtable.set_property = onSetProperty;

	GDBusInterfaceInfo **ppInterface = pNode->interfaces;

	Logger::debug(SSTR << prefix << "+ " << pNode->path);

	while(nullptr != *ppInterface)
	{
		GError *pError = nullptr;
		Logger::debug(SSTR << prefix << "    (iface: " << (*ppInterface)->name << ")");
		guint registeredObjectId = g_dbus_connection_register_object
		(
			pBusConnection,             // GDBusConnection *connection
			basePath.c_str(),           // const gchar *object_path
			*ppInterface,               // GDBusInterfaceInfo *interface_info
			&interfaceVtable,           // const GDBusInterfaceVTable *vtable
			nullptr,                    // gpointer user_data
			nullptr,                    // GDestroyNotify user_data_free_func
			&pError                     // GError **error
		);

		if (0 == registeredObjectId)
		{
			Logger::error(SSTR << "Failed to register object: " << (nullptr == pError ? "Unknown" : pError->message));

			// Cleanup and pretend like we were never here
			g_dbus_node_info_unref(pNode);
			registeredObjectIds.clear();

			// Try again later
			setRetryFailure();
			return;
		}

		// Save the registered object Id so we can clean it up later
		registeredObjectIds.push_back(registeredObjectId);

		++ppInterface;
	}

	GDBusNodeInfo **ppChild = pNode->nodes;
	while(nullptr != *ppChild)
	{
		registerNodeHierarchy(*ppChild, basePath + (*ppChild)->path, depth + 1);

		++ppChild;
	}
}

void registerObjects()
{
	// Parse each object into an XML interface tree
	for (const DBusObject &object : TheServer->getObjects())
	{
		GError *pError = nullptr;
		std::string xmlString = object.generateIntrospectionXML();
		GDBusNodeInfo *pNode = g_dbus_node_info_new_for_xml(xmlString.c_str(), &pError);
		if (nullptr == pNode)
		{
			Logger::error(SSTR << "Failed to introspect XML: " << (nullptr == pError ? "Unknown" : pError->message));
			setRetryFailure();
			return;
		}

		Logger::debug(SSTR << "Registering object hierarchy with D-Bus hierarchy");

		// Register the node hierarchy
		registerNodeHierarchy(pNode, DBusObjectPath(pNode->path));

		// Cleanup the node
		g_dbus_node_info_unref(pNode);
	}

	// Keep going
	initializationStateProcessor();
}

// ---------------------------------------------------------------------------------------------------------------------------------
//     _       _             _                               __ _                       _   _
//    / \   __| | __ _ _ __ | |_ ___ _ __    ___ ___  _ __  / _(_) __ _ _   _ _ __ __ _| |_(_) ___  _ ___
//   / _ \ / _` |/ _` | '_ \| __/ _ \ '__|  / __/ _ \| '_ \| |_| |/ _` | | | | '__/ _` | __| |/ _ \| '_  |
//  / ___ \ (_| | (_| | |_) | ||  __/ |    | (_| (_) | | | |  _| | (_| | |_| | | | (_| | |_| | (_) | | | |
// /_/   \_\__,_|\__,_| .__/ \__\___|_|     \___\___/|_| |_|_| |_|\__, |\__,_|_|  \__,_|\__|_|\___/|_| |_|
//                    |_|                                         |___/
//
// ---------------------------------------------------------------------------------------------------------------------------------

// Configure an adapter to ensure it is setup the way we need. We turn things on that we need and turn everything else off
// (to maximize security.)
//
// Note that this only works for the first adapter (index 0). Search for kControllerIndex for information.
//
// See also: https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/mgmt-api.txt
void configureAdapter()
{
	Mgmt mgmt;

	// Get our properly truncated advertising names
	std::string advertisingName = Mgmt::truncateName(TheServer->getAdvertisingName());
	std::string advertisingShortName = Mgmt::truncateShortName(TheServer->getAdvertisingShortName());

	// Find out what our current settings are
	HciAdapter::ControllerInformation info = HciAdapter::getInstance().getControllerInformation();

	// Are all of our settings the way we want them?
	bool pwFlag = info.currentSettings.isSet(HciAdapter::EHciPowered) == true;
	bool leFlag = info.currentSettings.isSet(HciAdapter::EHciLowEnergy) == true;
	bool brFlag = info.currentSettings.isSet(HciAdapter::EHciBasicRate_EnhancedDataRate) == TheServer->getEnableBREDR();
	bool scFlag = info.currentSettings.isSet(HciAdapter::EHciSecureConnections) == TheServer->getEnableSecureConnection();
	bool bnFlag = info.currentSettings.isSet(HciAdapter::EHciBondable) == TheServer->getEnableBondable();
	bool cnFlag = info.currentSettings.isSet(HciAdapter::EHciConnectable) == TheServer->getEnableConnectable();
	bool adFlag = info.currentSettings.isSet(HciAdapter::EHciAdvertising) == TheServer->getEnableAdvertising();
	bool anFlag = (advertisingName.length() == 0 || advertisingName == info.name) && (advertisingShortName.length() == 0 || advertisingShortName == info.shortName);

	// If everything is setup already, we're done
	if (!pwFlag || !leFlag || !brFlag || !scFlag || !bnFlag || !cnFlag || !adFlag || !anFlag)
	{
		// We need it off to start with
		if (pwFlag)
		{
			Logger::debug("Powering off");
			if (!mgmt.setPowered(false)) { setRetry(); return; }
		}

		// Enable the LE state (we always set this state if it's not set)
		if (!leFlag)
		{
			Logger::debug("Enabling LE");
			if (!mgmt.setLE(true)) { setRetry(); return; }
		}

		// Change the Br/Edr state?
		//
		// Note that enabling this requries LE to already be enabled or this command will receive a 'rejected' result
		if (!brFlag)
		{
			Logger::debug(SSTR << (TheServer->getEnableBREDR() ? "Enabling":"Disabling") << " BR/EDR");
			if (!mgmt.setBredr(TheServer->getEnableBREDR())) { setRetry(); return; }
		}

		// Change the Secure Connectinos state?
		if (!scFlag)
		{
			Logger::debug(SSTR << (TheServer->getEnableSecureConnection() ? "Enabling":"Disabling") << " Secure Connections");
			if (!mgmt.setSecureConnections(TheServer->getEnableSecureConnection() ? 1 : 0)) { setRetry(); return; }
		}

		// Change the Bondable state?
		if (!bnFlag)
		{
			Logger::debug(SSTR << (TheServer->getEnableBondable() ? "Enabling":"Disabling") << " Bondable");
			if (!mgmt.setBondable(TheServer->getEnableBondable())) { setRetry(); return; }
		}

		// Change the Connectable state?
		if (!cnFlag)
		{
			Logger::debug(SSTR << (TheServer->getEnableConnectable() ? "Enabling":"Disabling") << " Connectable");
			if (!mgmt.setConnectable(TheServer->getEnableConnectable())) { setRetry(); return; }
		}

		// Change the Advertising state?
		if (!adFlag)
		{
			Logger::debug(SSTR << (TheServer->getEnableAdvertising() ? "Enabling":"Disabling") << " Advertising");
			if (!mgmt.setAdvertising(TheServer->getEnableAdvertising() ? 1 : 0)) { setRetry(); return; }
		}

		// Set the name?
		if (!anFlag)
		{
			Logger::info(SSTR << "Setting advertising name to '" << advertisingName << "' (with short name: '" << advertisingShortName << "')");
			if (!mgmt.setName(advertisingName.c_str(), advertisingShortName.c_str())) { setRetry(); return; }
		}

		// Turn it back on
		Logger::debug("Powering on");
		if (!mgmt.setPowered(true)) { setRetry(); return; }
	}

	Logger::info("The Bluetooth adapter is fully configured");

	// We're all set, nothing to do!
	bAdapterConfigured = true;
	initializationStateProcessor();
}

// ---------------------------------------------------------------------------------------------------------------------------------
//     _       _             _
//    / \   __| | __ _ _ __ | |_ ___ _ __
//   / _ \ / _` |/ _` | '_ \| __/ _ \ '__|
//  / ___ \ (_| | (_| | |_) | ||  __/ |
// /_/   \_\__,_|\__,_| .__/ \__\___|_|
//                    |_|
//
// ---------------------------------------------------------------------------------------------------------------------------------

// Find the BlueZ's GATT Manager interface for the *first* Bluetooth adapter provided by BlueZ. We'll need this to register our
// GATT server with BlueZ.
void findAdapterInterface()
{
	// Get a list of the BlueZ's D-Bus objects
	GList *pObjects = g_dbus_object_manager_get_objects(pBluezObjectManager);
	if (nullptr == pObjects)
	{
		Logger::error(SSTR << "Unable to get ObjectManager objects");
		setRetryFailure();
		return;
	}

	// Scan the list of objects we find one with a GATT manager interface
	//
	// Note that if there are multiple interfaces, we will only find the first
	for (guint i = 0; i < g_list_length(pObjects) && bluezGattManagerInterfaceName.empty(); ++i)
	{
		// Current object in question
		pBluezAdapterObject = static_cast<GDBusObject *>(g_list_nth_data(pObjects, i));
		if (nullptr == pBluezAdapterObject) { continue; }

		// See if it has a GATT manager interface
		pBluezGattManagerProxy = reinterpret_cast<GDBusProxy *>(g_dbus_object_get_interface(pBluezAdapterObject, "org.bluez.GattManager1"));
		if (nullptr == pBluezGattManagerProxy) { continue; }

		// Get the interface proxy for this adapter - this will come in handy later
		pBluezAdapterInterfaceProxy = reinterpret_cast<GDBusProxy *>(g_dbus_object_get_interface(pBluezAdapterObject, "org.bluez.Adapter1"));
		if (nullptr == pBluezAdapterInterfaceProxy)
		{
			Logger::warn(SSTR << "Failed to get adapter proxy for interface 'org.bluez.Adapter1'");
			continue;
		}

		// Get the interface proxy for this adapter's properties - this will come in handy later
		pBluezAdapterPropertiesInterfaceProxy = reinterpret_cast<GDBusProxy *>(g_dbus_object_get_interface(pBluezAdapterObject, "org.freedesktop.DBus.Properties"));
		if (nullptr == pBluezAdapterPropertiesInterfaceProxy)
		{
			Logger::warn(SSTR << "Failed to get adapter properties proxy for interface 'org.freedesktop.DBus.Properties'");
			continue;
		}

		// Finally, save off the interface name, we're done!
		bluezGattManagerInterfaceName = g_dbus_proxy_get_object_path(pBluezGattManagerProxy);
		break;
	}

	// Get a fresh copy of our objects so we can release the entire list
	pBluezAdapterObject = g_dbus_object_manager_get_object(pBluezObjectManager, g_dbus_object_get_object_path(pBluezAdapterObject));

	// We'll need access to the device object so we can set properties on it
	pBluezDeviceObject = g_dbus_object_manager_get_object(pBluezObjectManager, g_dbus_object_get_object_path(pBluezAdapterObject));

	// Cleanup the list
	for (guint i = 0; i < g_list_length(pObjects) && bluezGattManagerInterfaceName.empty(); ++i)
	{
		g_object_unref(g_list_nth_data(pObjects, i));
	}

	g_list_free(pObjects);

	// If we didn't find the adapter object, reset things and we'll try again later
	if (nullptr == pBluezAdapterObject || nullptr == pBluezDeviceObject)
	{
		Logger::warn(SSTR << "Unable to find BlueZ objects outside of object list");
		bluezGattManagerInterfaceName.clear();
	}

	// If we never ended up with an interface name, bail now
	if (bluezGattManagerInterfaceName.empty())
	{
		Logger::error(SSTR << "Unable to find the adapter");
		setRetryFailure();
		return;
	}

	// Keep going
	initializationStateProcessor();
}

// ---------------------------------------------------------------------------------------------------------------------------------
//  ____  _            _____   ___  _     _           _   __  __
// | __ )| |_   _  ___|__  /  / _ \| |__ (_) ___  ___| |_|  \/  | __ _ _ __   __ _  __ _  ___ _ __
// |  _ \| | | | |/ _ \ / /  | | | | '_ \| |/ _ \/ __| __| |\/| |/ _` | '_ \ / _` |/ _` |/ _ \ '__|
// | |_) | | |_| |  __// /_  | |_| | |_) | |  __/ (__| |_| |  | | (_| | | | | (_| | (_| |  __/ |
// |____/|_|\__,_|\___/____|  \___/|_.__// |\___|\___|\__|_|  |_|\__,_|_| |_|\__,_|\__, |\___|_|
//                                     |__/                                        |___/
//
// ---------------------------------------------------------------------------------------------------------------------------------

// Get the D-Bus Object Manager client to the BlueZ ObjectManager object
//
// An ObjectManager allows us to find out what objects (and from those, interfaces, etc.) are available from an owned name. We'll
// use this to interrogate BlueZ's objects to find an adapter we can use, among other things.
void getBluezObjectManager()
{
	g_dbus_object_manager_client_new
	(
		pBusConnection,                             // GDBusConnection
		G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,    // GDBusObjectManagerClientFlags
		"org.bluez",                                // Owner name (or well-known name)
		"/",                                        // Object path
		nullptr,                                    // GDBusProxyTypeFunc get_proxy_type_func
		nullptr,                                    // gpointer get_proxy_type_user_data
		nullptr,                                    // GDestroyNotify get_proxy_type_destroy_notify
		nullptr,                                    // GCancellable *cancellable

		// GAsyncReadyCallback callback
		[] (GObject * /*pSourceObject*/, GAsyncResult *pAsyncResult, gpointer /*pUserData*/)
		{
			// Store BlueZ's ObjectManager
			GError *pError = nullptr;
			pBluezObjectManager = g_dbus_object_manager_client_new_finish(pAsyncResult, &pError);

			if (nullptr == pBluezObjectManager)
			{
				Logger::error(SSTR << "Failed to get an ObjectManager client: " << (nullptr == pError ? "Unknown" : pError->message));
				setRetryFailure();
				return;
			}

			// Keep going
			initializationStateProcessor();
		},

		nullptr                                     // gpointer user_data
	);
}

// ---------------------------------------------------------------------------------------------------------------------------------
//   ___                          _
//  / _ \__      ___ __   ___  __| |  _ __   __ _ _ __ ___   ___
// | | | \ \ /\ / / '_ \ / _ \/ _` | | '_ \ / _` | '_ ` _ \ / _ )
// | |_| |\ V  V /| | | |  __/ (_| | | | | | (_| | | | | | |  __/
//  \___/  \_/\_/ |_| |_|\___|\__,_| |_| |_|\__,_|_| |_| |_|\___|
//
// ---------------------------------------------------------------------------------------------------------------------------------

// Acquire an "owned name" with D-Bus. This name represents our server as a whole, identifying us on D-Bus and allowing others
// (BlueZ) to communicate back to us. All of the D-Bus objects (which represent our BlueZ services, characteristics, etc.) will all
// reside under this owned name.
//
// Note about error management: We don't yet hwave a timeout callback running for retries; errors are considered fatal
void doOwnedNameAcquire()
{
	// Our name is not presently lost
	bOwnedNameAcquired = false;

	ownedNameId = g_bus_own_name_on_connection
	(
		pBusConnection,                    // GDBusConnection *connection
		TheServer->getOwnedName().c_str(), // const gchar *name
		G_BUS_NAME_OWNER_FLAGS_NONE,       // GBusNameOwnerFlags flags

		// GBusNameAcquiredCallback name_acquired_handler
		[](GDBusConnection *, const gchar *, gpointer)
		{
			// Handy way to get periodic activity
			periodicTimeoutId = g_timeout_add_seconds(kPeriodicTimerFrequencySeconds, onPeriodicTimer, pBusConnection);
			if (periodicTimeoutId <= 0)
			{
				Logger::fatal(SSTR << "Failed to add a periodic timer");
				setServerHealth(EFailedInit);
				shutdown();
			}

			// Bus name acquired
			bOwnedNameAcquired = true;

			// Keep going...
			initializationStateProcessor();
		},

		// GBusNameLostCallback name_lost_handler
		[](GDBusConnection *, const gchar *, gpointer)
		{
			// Bus name lost
			bOwnedNameAcquired = false;

			// If we don't have a periodicTimeout (which we use for error recovery) then we're sunk
			if (0 == periodicTimeoutId)
			{
				Logger::fatal(SSTR << "Unable to acquire an owned name ('" << TheServer->getOwnedName() << "') on the bus");
				setServerHealth(EFailedInit);
				shutdown();
			}
			else
			{
				Logger::warn(SSTR << "Owned name ('" << TheServer->getOwnedName() << "') lost");
				setRetryFailure();
				return;
			}

			// Keep going...
			initializationStateProcessor();
		},

		nullptr,                          // gpointer user_data
		nullptr                           // GDestroyNotify user_data_free_func
	);
}

// ---------------------------------------------------------------------------------------------------------------------------------
//  ____
// | __ ) _   _ ___
// |  _ \| | | / __|
// | |_) | |_| \__ )
// |____/ \__,_|___/
//
// ---------------------------------------------------------------------------------------------------------------------------------

// Acquire a connection to the SYSTEM bus so we can communicate with BlueZ.
//
// Note about error management: We don't yet hwave a timeout callback running for retries; errors are considered fatal
void doBusAcquire()
{
	// Acquire a connection to the SYSTEM bus
	g_bus_get
	(
		G_BUS_TYPE_SYSTEM,      // GBusType bus_type
		nullptr,                // GCancellable *cancellable

		// GAsyncReadyCallback callback
		[] (GObject */*pSourceObject*/, GAsyncResult *pAsyncResult, gpointer /*pUserData*/)
		{
			GError *pError = nullptr;
			pBusConnection = g_bus_get_finish(pAsyncResult, &pError);

			if (nullptr == pBusConnection)
			{
				Logger::fatal(SSTR << "Failed to get bus connection: " << (nullptr == pError ? "Unknown" : pError->message));
				setServerHealth(EFailedInit);
				shutdown();
			}

			// Continue
			initializationStateProcessor();
		},

		nullptr                 // gpointer user_data
	);
}

// ---------------------------------------------------------------------------------------------------------------------------------
//  ____  _        _                                                                     _
// / ___|| |_ __ _| |_ ___   _ __ ___   __ _ _ __   __ _  __ _  ___ _ __ ___   ___ _ __ | |_
// \___ \| __/ _` | __/ _ \ | '_ ` _ \ / _` | '_ \ / _` |/ _` |/ _ \ '_ ` _ \ / _ \ '_ \| __|
//  ___) | || (_| | ||  __/ | | | | | | (_| | | | | (_| | (_| |  __/ | | | | |  __/ | | | |_
// |____/ \__\__,_|\__\___| |_| |_| |_|\__,_|_| |_|\__,_|\__, |\___|_| |_| |_|\___|_| |_|\__|
//                                                       |___/
//
// ---------------------------------------------------------------------------------------------------------------------------------

// Poor-man's state machine, which effectively ensures everything is initialized in order by verifying actual initialization state
// rather than stepping through a set of numeric states. This way, if something fails in an out-of-order sort of way, we can still
// handle it and recover nicely.
void initializationStateProcessor()
{
	// If we're in our end-of-life or waiting for a retry, don't process states
	if (ggkGetServerRunState() > ERunning || 0 != retryTimeStart)
	{
		return;
	}

	//
	// Get a bus connection
	//
	if (nullptr == pBusConnection)
	{
		Logger::debug(SSTR << "Acquiring bus connection");
		doBusAcquire();
		return;
	}

	//
	// Acquire an owned name on the bus
	//
	if (!bOwnedNameAcquired)
	{
		Logger::debug(SSTR << "Acquiring owned name: '" << TheServer->getOwnedName() << "'");
		doOwnedNameAcquire();
		return;
	}

	//
	// Get BlueZ's ObjectManager
	//
	if (nullptr == pBluezObjectManager)
	{
		Logger::debug(SSTR << "Getting BlueZ ObjectManager");
		getBluezObjectManager();
		return;
	}

	//
	// Find the adapter interface
	//
	if (bluezGattManagerInterfaceName.empty())
	{
		Logger::debug(SSTR << "Finding BlueZ GattManager1 interface");
		findAdapterInterface();
		return;
	}

	//
	// Find the adapter interface
	//
	if (!bAdapterConfigured)
	{
		Logger::debug(SSTR << "Configuring BlueZ adapter '" << bluezGattManagerInterfaceName << "'");
		configureAdapter();
		return;
	}

	//
	// Register our object with D-bus
	//
	if (registeredObjectIds.empty())
	{
		Logger::debug(SSTR << "Registering with D-Bus");
		registerObjects();
		return;
	}

	// Register our appliation with the BlueZ GATT manager
	if (!bApplicationRegistered)
	{
		Logger::debug(SSTR << "Registering application with BlueZ GATT manager");

		doRegisterApplication();
		return;
	}

	// At this point, we should be fully initialized
	//
	// It shouldn't ever happen, but just in case, let's double-check that we're healthy and if not, shutdown immediately
	if (ggkGetServerHealth() != EOk)
	{
		shutdown();
		return;
	}

	// Successful initialization - switch to running state
	setServerRunState(ERunning);
}

// ---------------------------------------------------------------------------------------------------------------------------------
//  ____                                                                  _
// |  _ \ _   _ _ __     ___  ___ _ ____   _____ _ __    _ __ _   _ _ __ | |
// | |_) | | | | '_ \   / __|/ _ \ '__\ \ / / _ \ '__|  | '__| | | | '_ \| |
// |  _ <| |_| | | | |  \__ \  __/ |   \ V /  __/ | _   | |  | |_| | | | |_|
// |_| \_\\__,_|_| |_|  |___/\___|_|    \_/ \___|_|( )  |_|   \__,_|_| |_(_)
//                                                 |/
//
// ---------------------------------------------------------------------------------------------------------------------------------

// Entry point for the asynchronous server thread
//
// This method should not be called directly, instead, direct your attention over to `ggkStart()`
void runServerThread()
{
	// Set the initialization state
	setServerRunState(EInitializing);

	// Start our state processor, which is really just a simplified state machine that steps us through an asynchronous
	// initialization process.
	//
	// In case you're wondering if these really need to be async, the answer is yes. For one, it's the right way to do it. But
	// the more practical response is that the main loop must be running (see below) in order for us to receive and respond to
	// events from BlueZ. Well, one of the calls we need to make during initialization is 'RegisterApplication'. This method will
	// will require that we respond the 'GetNamedObjects' method before it returns. If we were to call the synchronous version of
	// 'RegisterApplication', then we've effectively created a deadlock.
	//
	// There are alternatives, but using async methods is the recommended way.
	initializationStateProcessor();

	Logger::debug(SSTR << "Creating GLib main loop");
	pMainLoop = g_main_loop_new(NULL, FALSE);

	// Add the idle function
	//
	// Note that we actually run the idle function from a lambda. This allows us to manage the inter-idle sleep so we don't
	// soak up 100% of our CPU.
	guint res = g_idle_add
	(
		[](gpointer pUserData) -> gboolean
		{
			// Try to process some data and if no data is processed, sleep for the requested frequency
			if (!idleFunc(pUserData))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(kIdleFrequencyMS));
			}

			// Always return TRUE so our idle remains in tact
			return TRUE;
		},
		nullptr
	);

	if (res == 0)
	{
		Logger::error(SSTR << "Unable to add idle to main loop");
	}

	Logger::trace(SSTR << "Starting GLib main loop");
	g_main_loop_run(pMainLoop);

	// We have stopped
	setServerRunState(EStopped);
	Logger::info("GGK server stopped");

	// Cleanup
	uninit();
}

}; // namespace ggk
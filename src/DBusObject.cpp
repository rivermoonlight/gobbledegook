// Copyright 2017 Paul Nettle.
//
// This file is part of Gobbledegook.
//
// Gobbledegook is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Gobbledegook is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Gobbledegook.  If not, see <http://www.gnu.org/licenses/>.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// >>
// >>>  INSIDE THIS FILE
// >>
//
// This is an abstraction of a D-Bus object.
//
// >>
// >>>  DISCUSSION
// >>
//
// A D-Bus object is a container for any number of functional interfaces to expose on the bus. Objects are referred to by their
// path ("/com/acme/widgets"). Here is a simple example of how D-Bus objects relate to Bluetooth services:
//
// Object (path)                               Interface (name)
//
// /com/acme/widget                            org.bluez.GattService1
// /com/acme/widget/manufacturer_name          org.bluez.GattCharacteristic1
// /com/acme/widget/serial_number              org.bluez.GattCharacteristic1
//
// In English, this would be read as "The Acme company has a widget, which has two characteristics defining the manufacturer name
// and serial number for the widget."
//
// Finally, we'll include a published flag. Here's what that's all about:
//
// BlueZ uses the GetManagedObjects method (from the org.freedesktop.DBus.ObjectManager interface) to interrogate our
// service(s). Our Server, however, includes all objects and interfaces, including the GetManagedObjects as well as the various
// interfaces we expose over Bluetooth. Therefore, we'll need a way to know which ones to expose over Bluetooth (which is, in
// general, everything EXCEPT the object containing the org.freedesktop.DBus.ObjectManager interface.) Since we manage our
// objects in a hierarchy, only the root object's publish flag matters.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "GattProperty.h"
#include "DBusInterface.h"
#include "GattService.h"
#include "DBusObject.h"
#include "Utils.h"
#include "GattUuid.h"
#include "Logger.h"

namespace ggk {

// Construct a root object with no parent
//
// We'll include a publish flag since only root objects can be published
DBusObject::DBusObject(const DBusObjectPath &path, bool publish)
: publish(publish), path(path), pParent(nullptr)
{
}

// Construct a node object
//
// Nodes inherit their parent's publish path
DBusObject::DBusObject(DBusObject *pParent, const DBusObjectPath &pathElement)
: publish(pParent->publish), path(pathElement), pParent(pParent)
{
}

//
// Accessors
//

// Returns the `publish` flag
bool DBusObject::isPublished() const
{
	return publish;
}

// Returns the path node for this object within the hierarchy
//
// This method only returns the node. To get the full path, use `getPath()`
const DBusObjectPath &DBusObject::getPathNode() const
{
	return path;
}

// Returns the full path for this object within the hierarchy
//
// This method returns the full path. To get the current node, use `getPathNode()`
DBusObjectPath DBusObject::getPath() const
{
	DBusObjectPath path = getPathNode();
	const DBusObject *pCurrent = pParent;

	// Traverse up my chain, adding nodes to the path until we have the full thing
	while(nullptr != pCurrent)
	{
		path = pCurrent->getPathNode() + path;
		pCurrent = pCurrent->pParent;
	}

	return path;
}

// Returns the parent object in the hierarchy
DBusObject &DBusObject::getParent()
{
	return *pParent;
}

// Returns the list of children objects
const std::list<DBusObject> &DBusObject::getChildren() const
{
	return children;
}

// Add a child to this object
DBusObject &DBusObject::addChild(const DBusObjectPath &pathElement)
{
	children.push_back(DBusObject(this, pathElement));
	return children.back();
}

// Returns a list of interfaces for this object
const DBusObject::InterfaceList &DBusObject::getInterfaces() const
{
	return interfaces;
}

// Convenience functions to add a GATT service to the hierarchy
//
// We simply add a new child at the given path and add an interface configured as a GATT service to it using the given UUID.
GattService &DBusObject::gattServiceBegin(const std::string &pathElement, const GattUuid &uuid)
{
	DBusObject &child = addChild(DBusObjectPath(pathElement));
	GattService &service = *child.addInterface(std::make_shared<GattService>(child, "org.bluez.GattService1"));
	service.addProperty<GattService>("UUID", uuid);
	service.addProperty<GattService>("Primary", true);
	return service;
}

//
// Helpful routines for searching objects
//

// Finds an interface by name within this D-Bus object
std::shared_ptr<const DBusInterface> DBusObject::findInterface(const DBusObjectPath &path, const std::string &interfaceName, const DBusObjectPath &basePath) const
{
	if ((basePath + getPathNode()) == path)
	{
		for (std::shared_ptr<const DBusInterface> interface : interfaces)
		{
			if (interfaceName == interface->getName())
			{
				return interface;
			}
		}
	}

	for (const DBusObject &child : getChildren())
	{
		std::shared_ptr<const DBusInterface> pInterface = child.findInterface(path, interfaceName, basePath + getPathNode());
		if (nullptr != pInterface)
		{
			return pInterface;
		}
	}

	return nullptr;
}

// Finds a BlueZ method by name within the specified D-Bus interface
bool DBusObject::callMethod(const DBusObjectPath &path, const std::string &interfaceName, const std::string &methodName, GDBusConnection *pConnection, GVariant *pParameters, GDBusMethodInvocation *pInvocation, gpointer pUserData, const DBusObjectPath &basePath) const
{
	if ((basePath + getPathNode()) == path)
	{
		for (std::shared_ptr<const DBusInterface> interface : interfaces)
		{
			if (interfaceName == interface->getName())
			{
				if (interface->callMethod(methodName, pConnection, pParameters, pInvocation, pUserData))
				{
					return true;
				}
			}
		}
	}

	for (const DBusObject &child : getChildren())
	{
		if (child.callMethod(path, interfaceName, methodName, pConnection, pParameters, pInvocation, pUserData, basePath + getPathNode()))
		{
			return true;
		}
	}

	return false;
}

// Periodic timer tick propagation
void DBusObject::tickEvents(GDBusConnection *pConnection, void *pUserData) const
{
	for (std::shared_ptr<const DBusInterface> interface : interfaces)
	{
		interface->tickEvents(pConnection, pUserData);
	}

	for (const DBusObject &child : getChildren())
	{
		child.tickEvents(pConnection, pUserData);
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------
// XML generation for a D-Bus introspection
// ---------------------------------------------------------------------------------------------------------------------------------

// Internal method used to generate introspection XML used to describe our services on D-Bus
std::string DBusObject::generateIntrospectionXML(int depth) const
{
	std::string prefix;
	prefix.insert(0, depth * 2, ' ');

	std::string xml = std::string();

	if (depth == 0)
	{
		xml += "<?xml version='1.0'?>\n";
		xml += "<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' 'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>\n";
	}

	xml += prefix + "<node name='" + getPathNode().toString() + "'>\n";
	xml += prefix + "  <annotation name='" + TheServer->getServiceName() + ".DBusObject.path' value='" + getPath().toString() + "' />\n";

	for (std::shared_ptr<const DBusInterface> interface : interfaces)
	{
		xml += interface->generateIntrospectionXML(depth + 1);
	}

	for (DBusObject child : getChildren())
	{
		xml += child.generateIntrospectionXML(depth + 1);
	}

	xml += prefix + "</node>\n";

	if (depth == 0)
	{
		Logger::debug("Generated XML:");
		Logger::debug(xml);
	}

	return xml;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// D-Bus signals
// ---------------------------------------------------------------------------------------------------------------------------------

// Emits a signal on the bus from the given path, interface name and signal name, containing a GVariant set of parameters
void DBusObject::emitSignal(GDBusConnection *pBusConnection, const std::string &interfaceName, const std::string &signalName, GVariant *pParameters)
{
	GError *pError = nullptr;
	gboolean result = g_dbus_connection_emit_signal
	(
		pBusConnection,          // GDBusConnection *connection
		NULL,                    // const gchar *destination_bus_name
		getPath().c_str(),       // const gchar *object_path
		interfaceName.c_str(),   // const gchar *interface_name
		signalName.c_str(),      // const gchar *signal_name
		pParameters,             // GVariant *parameters
		&pError                  // GError **error
	);

	if (0 == result)
	{
		Logger::error(SSTR << "Failed to emit signal named '" << signalName << "': " << (nullptr == pError ? "Unknown" : pError->message));
	}
}


}; // namespace ggk
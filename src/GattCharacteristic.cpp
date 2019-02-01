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
// This is our representation of a GATT Characteristic which is intended to be used in our server description
//
// >>
// >>>  DISCUSSION
// >>
//
// A GATT characteristic is the component within the Bluetooth LE standard that holds and serves data over Bluetooth. This class
// is intended to be used within the server description. For an explanation of how this class is used, see the detailed discussion
// in Server.cpp.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "GattCharacteristic.h"
#include "GattDescriptor.h"
#include "GattProperty.h"
#include "GattUuid.h"
#include "DBusObject.h"
#include "GattService.h"
#include "Utils.h"
#include "Logger.h"

namespace ggk {

//
// Standard constructor
//

// Construct a GattCharacteristic
//
// Genreally speaking, these objects should not be constructed directly. Rather, use the `gattCharacteristicBegin()` method
// in `GattService`.
GattCharacteristic::GattCharacteristic(DBusObject &owner, GattService &service, const std::string &name)
: GattInterface(owner, name), service(service), pOnUpdatedValueFunc(nullptr)
{
}

// Returning the owner pops us one level up the hierarchy
//
// This method compliments `GattService::gattCharacteristicBegin()`
GattService &GattCharacteristic::gattCharacteristicEnd()
{
	return service;
}

// Locates a D-Bus method within this D-Bus interface and invokes the method
bool GattCharacteristic::callMethod(const std::string &methodName, GDBusConnection *pConnection, GVariant *pParameters, GDBusMethodInvocation *pInvocation, gpointer pUserData) const
{
	for (const DBusMethod &method : methods)
	{
		if (methodName == method.getName())
		{
			method.call<GattCharacteristic>(pConnection, getPath(), getName(), methodName, pParameters, pInvocation, pUserData);
			return true;
		}
	}

	return false;
}

// Adds an event to the characteristic and returns a refereence to 'this` to enable method chaining in the server description
//
// NOTE: We specifically overload this method in order to accept our custom EventCallback type and transform it into a
// TickEvent::Callback type. We also return our own type. This simplifies the server description by allowing call to chain.
GattCharacteristic &GattCharacteristic::onEvent(int tickFrequency, void *pUserData, EventCallback callback)
{
	events.push_back(TickEvent(this, tickFrequency, reinterpret_cast<TickEvent::Callback>(callback), pUserData));
	return *this;
}

// Ticks events within this characteristic
//
// Note: we specifically override this method in order to translate the generic TickEvent::Callback into our own EventCallback
void GattCharacteristic::tickEvents(GDBusConnection *pConnection, void *pUserData) const
{
	for (const TickEvent &event : events)
	{
		event.tick<GattCharacteristic>(getPath(), pConnection, pUserData);
	}
}

// Specialized support for ReadlValue method
//
// Defined as: array{byte} ReadValue(dict options)
//
// D-Bus breakdown:
//
//     Input args:  options - "a{sv}"
//     Output args: value   - "ay"
GattCharacteristic &GattCharacteristic::onReadValue(MethodCallback callback)
{
	// array{byte} ReadValue(dict options)
	static const char *inArgs[] = {"a{sv}", nullptr};
	addMethod("ReadValue", inArgs, "ay", reinterpret_cast<DBusMethod::Callback>(callback));
	return *this;
}

// Specialized support for WriteValue method
//
// Defined as: void WriteValue(array{byte} value, dict options)
//
// D-Bus breakdown:
//
//     Input args:  value   - "ay"
//                  options - "a{sv}"
//     Output args: void
GattCharacteristic &GattCharacteristic::onWriteValue(MethodCallback callback)
{
	static const char *inArgs[] = {"ay", "a{sv}", nullptr};
	addMethod("WriteValue", inArgs, nullptr, reinterpret_cast<DBusMethod::Callback>(callback));
	return *this;
}

// Custom support for handling updates to our characteristic's value
//
// Defined as: (NOT defined by Bluetooth or BlueZ - this method is internal only)
//
// This method is called by our framework whenever a characteristic's value is updated. If you need to perform any actions
// when a value is updatd, this is a good place to do that work.
//
// If you need to perform the same action(s) when a value is updated from the client (via `onWriteValue`) or from this server,
// then it may be beneficial to call this method from within your onWriteValue callback to reduce duplicated code. See
// `callOnUpdatedValue` for more information.
GattCharacteristic &GattCharacteristic::onUpdatedValue(UpdatedValueCallback callback)
{
	pOnUpdatedValueFunc = callback;
	return *this;
}

// Calls the onUpdatedValue method, if one was set.
//
// Returns false if there was no method set, otherwise, returns the boolean result of the method call.
//
// If you need to perform the same action(s) when a value is updated from the client (via onWriteValue) or from this server,
// then it may be beneficial to place those actions in the `onUpdatedValue` method and call it from from within your
// `onWriteValue` callback to reduce duplicated code. To call the `onUpdatedValue` method from within your `onWriteValue`, you
// can use this pattern:
//
//      .onWriteValue(CHARACTERISTIC_UPDATED_VALUE_CALLBACK_LAMBDA
//      {
//          // Update your value
//          ...
//
//          // Call the onUpdateValue method that was set in the same Characteristic
//          self.callOnUpdatedValue(pConnection, pUserData);
//      })
bool GattCharacteristic::callOnUpdatedValue(GDBusConnection *pConnection, void *pUserData) const
{
	if (nullptr == pOnUpdatedValueFunc)
	{
		return false;
	}

	Logger::debug(SSTR << "Calling OnUpdatedValue function for interface at path '" << getPath() << "'");
	return pOnUpdatedValueFunc(*this, pConnection, pUserData);
}

// Convenience functions to add a GATT descriptor to the hierarchy
//
// We simply add a new child at the given path and add an interface configured as a GATT descriptor to it. The
// new descriptor is declared with a UUID and a variable argument list of flags (in string form.) For a complete and
// up-to-date list of flag values, see: https://git.kernel.org/pub/scm/bluetooth/bluez.git/plain/doc/gatt-api.txt
//
// At the time of this writing, the list of flags is as follows:
//
//             "read"
//             "write"
//             "encrypt-read"
//             "encrypt-write"
//             "encrypt-authenticated-read"
//             "encrypt-authenticated-write"
//             "secure-read" (Server Only)
//             "secure-write" (Server Only)
//
//
// To end a descriptor, call `GattDescriptor::gattDescriptorEnd()`
GattDescriptor &GattCharacteristic::gattDescriptorBegin(const std::string &pathElement, const GattUuid &uuid, const std::vector<const char *> &flags)
{
	DBusObject &child = owner.addChild(DBusObjectPath(pathElement));
	GattDescriptor &descriptor = *child.addInterface(std::make_shared<GattDescriptor>(child, *this, "org.bluez.GattDescriptor1"));
	descriptor.addProperty<GattDescriptor>("UUID", uuid);
	descriptor.addProperty<GattDescriptor>("Characteristic", getPath());
	descriptor.addProperty<GattDescriptor>("Flags", flags);
	return descriptor;
}

// Sends a change notification to subscribers to this characteristic
//
// This is a generalized method that accepts a `GVariant *`. A templated version is available that supports common types called
// `sendChangeNotificationValue()`.
//
// The caller may choose to consult HciAdapter::getInstance().getActiveConnectionCount() in order to determine if there are any
// active connections before sending a change notification.
void GattCharacteristic::sendChangeNotificationVariant(GDBusConnection *pBusConnection, GVariant *pNewValue) const
{
	g_auto(GVariantBuilder) builder;
	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
	g_variant_builder_add(&builder, "{sv}", "Value", pNewValue);
	GVariant *pSasv = g_variant_new("(sa{sv})", "org.bluez.GattCharacteristic1", &builder);
	owner.emitSignal(pBusConnection, "org.freedesktop.DBus.Properties", "PropertiesChanged", pSasv);
}

}; // namespace ggk
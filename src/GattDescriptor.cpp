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
// A GATT descriptor is the component within the Bluetooth LE standard that holds and serves metadata about a Characteristic over
// Bluetooth. This class is intended to be used within the server description. For an explanation of how this class is used, see the
// detailed discussion in Server.cpp.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "GattDescriptor.h"
#include "GattProperty.h"
#include "DBusObject.h"
#include "Utils.h"
#include "Logger.h"

namespace ggk {

//
// Standard constructor
//

// Construct a GattDescriptor
//
// Genreally speaking, these objects should not be constructed directly. Rather, use the `gattDescriptorBegin()` method
// in `GattCharacteristic`.
GattDescriptor::GattDescriptor(DBusObject &owner, GattCharacteristic &characteristic, const std::string &name)
: GattInterface(owner, name), characteristic(characteristic), pOnUpdatedValueFunc(nullptr)
{
}

// Returning the owner pops us one level up the hierarchy
//
// This method compliments `GattCharacteristic::gattDescriptorBegin()`
GattCharacteristic &GattDescriptor::gattDescriptorEnd()
{
	return characteristic;
}

//
// D-Bus interface methods
//

// Locates a D-Bus method within this D-Bus interface
bool GattDescriptor::callMethod(const std::string &methodName, GDBusConnection *pConnection, GVariant *pParameters, GDBusMethodInvocation *pInvocation, gpointer pUserData) const
{
	for (const DBusMethod &method : methods)
	{
		if (methodName == method.getName())
		{
			method.call<GattDescriptor>(pConnection, getPath(), getName(), methodName, pParameters, pInvocation, pUserData);
			return true;
		}
	}

	return false;
}

// Adds an event to the descriptor and returns a refereence to 'this` to enable method chaining in the server description
//
// NOTE: We specifically overload this method in order to accept our custom EventCallback type and transform it into a
// TickEvent::Callback type. We also return our own type. This simplifies the server description by allowing call to chain.
GattDescriptor &GattDescriptor::onEvent(int tickFrequency, void *pUserData, EventCallback callback)
{
	events.push_back(TickEvent(this, tickFrequency, reinterpret_cast<TickEvent::Callback>(callback), pUserData));
	return *this;
}

// Ticks events within this descriptor
//
// Note: we specifically override this method in order to translate the generic TickEvent::Callback into our own EventCallback
void GattDescriptor::tickEvents(GDBusConnection *pConnection, void *pUserData) const
{
	for (const TickEvent &event : events)
	{
		event.tick<GattDescriptor>(getPath(), pConnection, pUserData);
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
GattDescriptor &GattDescriptor::onReadValue(MethodCallback callback)
{
	// array{byte} ReadValue(dict options)
	const char *inArgs[] = {"a{sv}", nullptr};
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
GattDescriptor &GattDescriptor::onWriteValue(MethodCallback callback)
{
	const char *inArgs[] = {"ay", "a{sv}", nullptr};
	addMethod("WriteValue", inArgs, nullptr, reinterpret_cast<DBusMethod::Callback>(callback));
	return *this;
}

// Custom support for handling updates to our descriptor's value
//
// Defined as: (NOT defined by Bluetooth or BlueZ - this method is internal only)
//
// This method is called by our framework whenever a Descriptor's value is updated. If you need to perform any actions
// when a value is updatd, this is a good place to do that work.
//
// If you need to perform the same action(s) when a value is updated from the client (via `onWriteValue`) or from this server,
// then it may be beneficial to call this method from within your onWriteValue callback to reduce duplicated code. See
// `callOnUpdatedValue` for more information.
GattDescriptor &GattDescriptor::onUpdatedValue(UpdatedValueCallback callback)
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
//      .onUpdatedValue(DESCRIPTOR_UPDATED_VALUE_CALLBACK_LAMBDA
//      {
//          // Update your value
//          ...
//
//          // Call the onUpdateValue method that was set in the same Descriptor
//          self.callOnUpdatedValue(pConnection, pUserData);
//      })
bool GattDescriptor::callOnUpdatedValue(GDBusConnection *pConnection, void *pUserData) const
{
	if (nullptr == pOnUpdatedValueFunc)
	{
		return false;
	}

	Logger::debug(SSTR << "Calling OnUpdatedValue function for interface at path '" << getPath() << "'");
	return pOnUpdatedValueFunc(*this, pConnection, pUserData);
}

}; // namespace ggk
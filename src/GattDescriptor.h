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
// See the discussion at the top of GattDescriptor.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <gio/gio.h>
#include <string>
#include <list>

#include "TickEvent.h"
#include "Utils.h"
#include "GattInterface.h"

namespace ggk {

// ---------------------------------------------------------------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------------------------------------------------------------

struct GattCharacteristic;
struct GattDescriptor;
struct GattProperty;
struct DBusObject;

// ---------------------------------------------------------------------------------------------------------------------------------
// Useful Lambdas
// ---------------------------------------------------------------------------------------------------------------------------------

#define DESCRIPTOR_UPDATED_VALUE_CALLBACK_LAMBDA [] \
( \
	const GattDescriptor &self, \
	GDBusConnection *pConnection, \
	void *pUserData \
) -> bool

#define DESCRIPTOR_EVENT_CALLBACK_LAMBDA [] \
( \
	const GattDescriptor &self, \
	const TickEvent &event, \
	GDBusConnection *pConnection, \
	void *pUserData \
)

#define DESCRIPTOR_METHOD_CALLBACK_LAMBDA [] \
( \
       const GattDescriptor &self, \
       GDBusConnection *pConnection, \
       const std::string &methodName, \
       GVariant *pParameters, \
       GDBusMethodInvocation *pInvocation, \
       void *pUserData \
)

// ---------------------------------------------------------------------------------------------------------------------------------
// Representation of a Bluetooth GATT Descriptor
// ---------------------------------------------------------------------------------------------------------------------------------

struct GattDescriptor : GattInterface
{
	// Our interface type
	static constexpr const char *kInterfaceType = "GattDescriptor";

	typedef void (*MethodCallback)(const GattDescriptor &self, GDBusConnection *pConnection, const std::string &methodName, GVariant *pParameters, GDBusMethodInvocation *pInvocation, void *pUserData);
	typedef void (*EventCallback)(const GattDescriptor &self, const TickEvent &event, GDBusConnection *pConnection, void *pUserData);
	typedef bool (*UpdatedValueCallback)(const GattDescriptor &self, GDBusConnection *pConnection, void *pUserData);

	//
	// Standard constructor
	//

	// Construct a GattDescriptor
	//
	// Genreally speaking, these objects should not be constructed directly. Rather, use the `gattDescriptorBegin()` method
	// in `GattCharacteristic`.
	GattDescriptor(DBusObject &owner, GattCharacteristic &characteristic, const std::string &name);
	virtual ~GattDescriptor() {}

	// Returns a string identifying the type of interface
	virtual const std::string getInterfaceType() const { return GattDescriptor::kInterfaceType; }

	// Returning the owner pops us one level up the hierarchy
	//
	// This method compliments `GattCharacteristic::gattDescriptorBegin()`
	GattCharacteristic &gattDescriptorEnd();

	// Locates a D-Bus method within this D-Bus interface and invokes the method
	virtual bool callMethod(const std::string &methodName, GDBusConnection *pConnection, GVariant *pParameters, GDBusMethodInvocation *pInvocation, gpointer pUserData) const;

	// Adds an event to the descriptor and returns a refereence to 'this` to enable method chaining in the server description
	//
	// NOTE: We specifically overload this method in order to accept our custom EventCallback type and transform it into a
	// TickEvent::Callback type. We also return our own type. This simplifies the server description by allowing call to chain.
	GattDescriptor &onEvent(int tickFrequency, void *pUserData, EventCallback callback);

	// Ticks events within this descriptor
	//
	// Note: we specifically override this method in order to translate the generic TickEvent::Callback into our own EventCallback
	virtual void tickEvents(GDBusConnection *pConnection, void *pUserData) const;

	// Specialized support for Descriptor ReadlValue method
	//
	// Defined as: array{byte} ReadValue(dict options)
	//
	// D-Bus breakdown:
	//
	//     Input args:  options - "a{sv}"
	//     Output args: value   - "ay"
	GattDescriptor &onReadValue(MethodCallback callback);

	// Specialized support for Descriptor WriteValue method
	//
	// Defined as: void WriteValue(array{byte} value, dict options)
	//
	// D-Bus breakdown:
	//
	//     Input args:  value   - "ay"
	//                  options - "a{sv}"
	//     Output args: void
	GattDescriptor &onWriteValue(MethodCallback callback);

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
	GattDescriptor &onUpdatedValue(UpdatedValueCallback callback);

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
	bool callOnUpdatedValue(GDBusConnection *pConnection, void *pUserData) const;

protected:

	GattCharacteristic &characteristic;
	UpdatedValueCallback pOnUpdatedValueFunc;
};

}; // namespace ggk
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
// This is our abstraction layer for GATT interfaces, used by GattService, GattCharacteristic & GattDescriptor
//
// >>
// >>>  DISCUSSION
// >>
//
// See the discussion at the top of GattInterface.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <gio/gio.h>
#include <string>
#include <list>

#include "TickEvent.h"
#include "DBusInterface.h"
#include "GattProperty.h"
#include "GattUuid.h"
#include "Server.h"
#include "Utils.h"

namespace ggk {

// ---------------------------------------------------------------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------------------------------------------------------------

struct GattInterface;
struct DBusObject;

// ---------------------------------------------------------------------------------------------------------------------------------
// Pure virtual representation of a Bluetooth GATT Interface, the base class for Services, Characteristics and Descriptors
// ---------------------------------------------------------------------------------------------------------------------------------

struct GattInterface : DBusInterface
{
	// Standard constructor
	GattInterface(DBusObject &owner, const std::string &name);
	virtual ~GattInterface();

	// Returns a string identifying the type of interface
	virtual const std::string getInterfaceType() const = 0;

	//
	// GATT Characteristic properties
	//

	// Returns the list of GATT properties
	const std::list<GattProperty> &getProperties() const;

	// Add a `GattProperty` to the interface
	//
	// There are helper methods for adding properties for common types as well as a generalized helper method for adding a
	// `GattProperty` of a generic GVariant * type.
	template<typename T>
	T &addProperty(const GattProperty &property)
	{
		properties.push_back(property);
		return *static_cast<T *>(this);
	}

	// Add a named property with a GVariant *
	//
	// There are helper methods for common types (UUIDs, strings, boolean, etc.) Use this method when no helper method exists for
	// the type you want to use. There is also a helper method for adding a named property of a pre-built `GattProperty`.
	template<typename T>
	T &addProperty(const std::string &name, GVariant *pValue, GDBusInterfaceGetPropertyFunc getter = nullptr, GDBusInterfaceSetPropertyFunc setter = nullptr)
	{
		return addProperty<T>(GattProperty(name, pValue, getter, setter));
	}

	// Helper method for adding a named property with a `GattUuid`
	template<typename T>
	T &addProperty(const std::string &name, const GattUuid &uuid, GDBusInterfaceGetPropertyFunc getter = nullptr, GDBusInterfaceSetPropertyFunc setter = nullptr)
	{
		return addProperty<T>(GattProperty(name, Utils::gvariantFromString(uuid.toString128().c_str()), getter, setter));
	}

	// Helper method for adding a named property with a `DBusObjectPath`
	template<typename T>
	T &addProperty(const std::string &name, const DBusObjectPath &path, GDBusInterfaceGetPropertyFunc getter = nullptr, GDBusInterfaceSetPropertyFunc setter = nullptr)
	{
		return addProperty<T>(GattProperty(name, Utils::gvariantFromObject(path), getter, setter));
	}

	// Helper method for adding a named property with a std::strings
	template<typename T>
	T &addProperty(const std::string &name, const std::string &str, GDBusInterfaceGetPropertyFunc getter = nullptr, GDBusInterfaceSetPropertyFunc setter = nullptr)
	{
		return addProperty<T>(GattProperty(name, Utils::gvariantFromString(str), getter, setter));
	}

	// Helper method for adding a named property with an array of std::strings
	template<typename T>
	T &addProperty(const std::string &name, const std::vector<std::string> &arr, GDBusInterfaceGetPropertyFunc getter = nullptr, GDBusInterfaceSetPropertyFunc setter = nullptr)
	{
		return addProperty<T>(GattProperty(name, Utils::gvariantFromStringArray(arr), getter, setter));
	}

	// Helper method for adding a named property with an array of C strings
	template<typename T>
	T &addProperty(const std::string &name, const std::vector<const char *> &arr, GDBusInterfaceGetPropertyFunc getter = nullptr, GDBusInterfaceSetPropertyFunc setter = nullptr)
	{
		return addProperty<T>(GattProperty(name, Utils::gvariantFromStringArray(arr), getter, setter));
	}

	// Helper method for adding a named property with a given C string
	template<typename T>
	T &addProperty(const std::string &name, const char *pStr, GDBusInterfaceGetPropertyFunc getter = nullptr, GDBusInterfaceSetPropertyFunc setter = nullptr)
	{
		return addProperty<T>(GattProperty(name, Utils::gvariantFromString(pStr), getter, setter));
	}

	// Helper method for adding a named property with a given boolean value
	template<typename T>
	T &addProperty(const std::string &name, bool value, GDBusInterfaceGetPropertyFunc getter = nullptr, GDBusInterfaceSetPropertyFunc setter = nullptr)
	{
		return addProperty<T>(GattProperty(name, Utils::gvariantFromBoolean(value), getter, setter));
	}

	// Return a data value from the server's registered data getter (GGKServerDataGetter)
	//
	// This method is for use with non-pointer types. For pointer types, use `getDataPointer()` instead.
	//
	// This method is intended to be used in the server description. An example usage would be:
	//
	//     uint8_t batteryLevel = self.getDataValue<uint8_t>("battery/level", 0);
	template<typename T>
	T getDataValue(const char *pName, const T defaultValue) const
	{
		const void *pData = TheServer->getDataGetter()(pName);
		return nullptr == pData ? defaultValue : *static_cast<const T *>(pData);
	}

	// Return a data pointer from the server's registered data getter (GGKServerDataGetter)
	//
	// This method is for use with pointer types. For non-pointer types, use `getDataValue()` instead.
	//
	// This method is intended to be used in the server description. An example usage would be:
	//
	//     const char *pTextString = self.getDataPointer<const char *>("text/string", "");
	template<typename T>
	T getDataPointer(const char *pName, const T defaultValue) const
	{
		const void *pData = TheServer->getDataGetter()(pName);
		return nullptr == pData ? defaultValue : static_cast<const T>(pData);
	}

	// Sends a data value from the server back to the application through the server's registered data setter
	// (GGKServerDataSetter)
	//
	// This method is for use with non-pointer types. For pointer types, use `setDataPointer()` instead.
	//
	// This method is intended to be used in the server description. An example usage would be:
	//
	//     self.setDataValue("battery/level", batteryLevel);
	template<typename T>
	bool setDataValue(const char *pName, const T value) const
	{
		return TheServer->getDataSetter()(pName, static_cast<const void *>(&value)) != 0;
	}

	// Sends a data pointer from the server back to the application through the server's registered data setter
	// (GGKServerDataSetter)
	//
	// This method is for use with pointer types. For non-pointer types, use `setDataValue()` instead.
	//
	// This method is intended to be used in the server description. An example usage would be:
	//
	//     self.setDataPointer("text/string", stringFromGVariantByteArray(pAyBuffer).c_str());
	template<typename T>
	bool setDataPointer(const char *pName, const T pointer) const
	{
		return TheServer->getDataSetter()(pName, static_cast<const void *>(pointer)) != 0;
	}

	// When responding to a ReadValue method, we need to return a GVariant value in the form "(ay)" (a tuple containing an array of
	// bytes). This method will simplify this slightly by wrapping a GVariant of the type "ay" and wrapping it in a tuple before
	// sending it off as the method response.
	//
	// This is the generalized form that accepts a GVariant *. There is a templated helper method (`methodReturnValue()`) that accepts
	// common types.
	void methodReturnVariant(GDBusMethodInvocation *pInvocation, GVariant *pVariant, bool wrapInTuple = false) const;

	// When responding to a ReadValue method, we need to return a GVariant value in the form "(ay)" (a tuple containing an array of
	// bytes). This method will simplify this slightly by wrapping a GVariant of the type "ay" and wrapping it in a tuple before
	// sending it off as the method response.
	//
	// This is a templated helper method that only works with common types. For a more generic form which can be used for custom
	// types, see `methodReturnVariant()'.
	template<typename T>
	void methodReturnValue(GDBusMethodInvocation *pInvocation, T value, bool wrapInTuple = false) const
	{
		GVariant *pVariant = Utils::gvariantFromByteArray(value);
		methodReturnVariant(pInvocation, pVariant, wrapInTuple);
	}

	// Locates a `GattProperty` within the interface
	//
	// This method returns a pointer to the property or nullptr if not found
	const GattProperty *findProperty(const std::string &name) const;

	// Internal method used to generate introspection XML used to describe our services on D-Bus
	virtual std::string generateIntrospectionXML(int depth) const;

protected:

	std::list<GattProperty> properties;
};

}; // namespace ggk
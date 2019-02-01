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
// A GATT Property is simply a name/value pair.
//
// >>
// >>>  DISCUSSION
// >>
//
// See the discussion at the top of GattProperty.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <gio/gio.h>
#include <string>

namespace ggk {

struct DBusObjectPath;

// Representation of a GATT Property
struct GattProperty
{
	// Constructs a named property
	//
	// In general, properties should not be constructed directly as properties are typically instanticated by adding them to to an
	// interface using one of the the interface's `addProperty` methods.
	GattProperty(const std::string &name, GVariant *pValue, GDBusInterfaceGetPropertyFunc getter = nullptr, GDBusInterfaceSetPropertyFunc setter = nullptr);

	//
	// Name
	//

	// Returns the name of the property
	const std::string &getName() const;

	// Sets the name of the property
	//
	// In general, this method should not be called directly as properties are typically added to an interface using one of the the
	// interface's `addProperty` methods.
	GattProperty &setName(const std::string &name);

	//
	// Value
	//

	// Returns the property's value
	const GVariant *getValue() const;

	// Sets the property's value
	//
	// In general, this method should not be called directly as properties are typically added to an interface using one of the the
	// interface's `addProperty` methods.
	GattProperty &setValue(GVariant *pValue);

	//
	// Callbacks to get/set this property
	//

	// Internal use method to retrieve the getter delegate method used to return custom values for a property
	GDBusInterfaceGetPropertyFunc getGetterFunc() const;

	// Internal use method to set the getter delegate method used to return custom values for a property
	//
	// In general, this method should not be called directly as properties are typically added to an interface using one of the the
	// interface's `addProperty` methods.
	GattProperty &setGetterFunc(GDBusInterfaceGetPropertyFunc func);

	// Internal use method to retrieve the setter delegate method used to return custom values for a property
	GDBusInterfaceSetPropertyFunc getSetterFunc() const;

	// Internal use method to set the setter delegate method used to return custom values for a property
	//
	// In general, this method should not be called directly as properties are typically added to an interface using one of the the
	// interface's `addProperty` methods.
	GattProperty &setSetterFunc(GDBusInterfaceSetPropertyFunc func);

	// Internal method used to generate introspection XML used to describe our services on D-Bus
	std::string generateIntrospectionXML(int depth) const;

private:

	std::string name;
	GVariant *pValue;
	GDBusInterfaceGetPropertyFunc getterFunc;
	GDBusInterfaceSetPropertyFunc setterFunc;
};

}; // namespace ggk
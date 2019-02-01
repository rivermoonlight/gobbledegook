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
// This class is intended to be used within the server description. For an explanation of how this class is used, see the detailed
// description in Server.cpp.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gio/gio.h>
#include <string>

#include "Utils.h"
#include "GattProperty.h"

namespace ggk {

// Constructs a named property
//
// In general, properties should not be constructed directly as properties are typically instanticated by adding them to to an
// interface using one of the the interface's `addProperty` methods.
GattProperty::GattProperty(const std::string &name, GVariant *pValue, GDBusInterfaceGetPropertyFunc getter, GDBusInterfaceSetPropertyFunc setter)
: name(name), pValue(pValue), getterFunc(getter), setterFunc(setter)
{
}

//
// Name
//

// Returns the name of the property
const std::string &GattProperty::getName() const
{
	return name;
}

// Sets the name of the property
//
// In general, this method should not be called directly as properties are typically added to an interface using one of the the
// interface's `addProperty` methods.
GattProperty &GattProperty::setName(const std::string &name)
{
	this->name = name;
	return *this;
}

//
// Value
//

// Returns the property's value
const GVariant *GattProperty::getValue() const
{
	return pValue;
}

// Sets the property's value
//
// In general, this method should not be called directly as properties are typically added to an interface using one of the the
// interface's `addProperty` methods.
GattProperty &GattProperty::setValue(GVariant *pValue)
{
	this->pValue = pValue;
	return *this;
}

//
// Callbacks to get/set this property
//

// Internal use method to retrieve the getter delegate method used to return custom values for a property
GDBusInterfaceGetPropertyFunc GattProperty::getGetterFunc() const
{
	return getterFunc;
}

// Internal use method to set the getter delegate method used to return custom values for a property
//
// In general, this method should not be called directly as properties are typically added to an interface using one of the the
// interface's `addProperty` methods.
GattProperty &GattProperty::setGetterFunc(GDBusInterfaceGetPropertyFunc func)
{
	getterFunc = func;
	return *this;
}

// Internal use method to retrieve the setter delegate method used to return custom values for a property
GDBusInterfaceSetPropertyFunc GattProperty::getSetterFunc() const
{
	return setterFunc;
}

// Internal use method to set the setter delegate method used to return custom values for a property
//
// In general, this method should not be called directly as properties are typically added to an interface using one of the the
// interface's `addProperty` methods.
GattProperty &GattProperty::setSetterFunc(GDBusInterfaceSetPropertyFunc func)
{
	setterFunc = func;
	return *this;
}

// Internal method used to generate introspection XML used to describe our services on D-Bus
std::string GattProperty::generateIntrospectionXML(int depth) const
{
	std::string prefix;
	prefix.insert(0, depth * 2, ' ');

	std::string xml = std::string();

	GVariant *pValue = const_cast<GVariant *>(getValue());
	const gchar *pType = g_variant_get_type_string(pValue);
	xml += prefix + "<property name='" + getName() + "' type='" + pType + "' access='read'>\n";

	if (g_variant_is_of_type(pValue, G_VARIANT_TYPE_BOOLEAN))
	{
		xml += prefix + "  <annotation name='name' value='" + (g_variant_get_boolean(pValue) != 0 ? "true":"false") + "' />\n";
	}
	else if (g_variant_is_of_type(pValue, G_VARIANT_TYPE_INT16))
	{
		xml += prefix + "  <annotation name='name' value='" + std::to_string(g_variant_get_int16(pValue)) + "' />\n";
	}
	else if (g_variant_is_of_type(pValue, G_VARIANT_TYPE_UINT16))
	{
		xml += prefix + "  <annotation name='name' value='" + std::to_string(g_variant_get_uint16(pValue)) + "' />\n";
	}
	else if (g_variant_is_of_type(pValue, G_VARIANT_TYPE_INT32))
	{
		xml += prefix + "  <annotation name='name' value='" + std::to_string(g_variant_get_int32(pValue)) + "' />\n";
	}
	else if (g_variant_is_of_type(pValue, G_VARIANT_TYPE_UINT32))
	{
		xml += prefix + "  <annotation name='name' value='" + std::to_string(g_variant_get_uint32(pValue)) + "' />\n";
	}
	else if (g_variant_is_of_type(pValue, G_VARIANT_TYPE_INT64))
	{
		xml += prefix + "  <annotation name='name' value='" + std::to_string(g_variant_get_int64(pValue)) + "' />\n";
	}
	else if (g_variant_is_of_type(pValue, G_VARIANT_TYPE_UINT64))
	{
		xml += prefix + "  <annotation name='name' value='" + std::to_string(g_variant_get_uint64(pValue)) + "' />\n";
	}
	else if (g_variant_is_of_type(pValue, G_VARIANT_TYPE_DOUBLE))
	{
		xml += prefix + "  <annotation value='" + std::to_string(g_variant_get_double(pValue)) + "' />\n";
	}
	else if (g_variant_is_of_type(pValue, G_VARIANT_TYPE_STRING))
	{
		xml += prefix + "  <annotation name='name' value='" + g_variant_get_string(pValue, nullptr) + "' />\n";
	}
	else if (g_variant_is_of_type(pValue, G_VARIANT_TYPE_OBJECT_PATH))
	{
		xml += prefix + "  <annotation name='name' value='" + g_variant_get_string(pValue, nullptr) + "' />\n";
	}
	else if (g_variant_is_of_type(pValue, G_VARIANT_TYPE_BYTESTRING))
	{
		xml += prefix + "  <annotation name='name' value='" + g_variant_get_bytestring(pValue) + "' />\n";
	}

	xml += prefix + "</property>\n";

	return xml;
}

}; // namespace ggk
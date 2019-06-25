// Copyright 2017-2019 Paul Nettle
//
// This file is part of Gobbledegook.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file in the root of the source tree.

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
// This class is intended to be used within the server description. For an explanation of how this class is used, see the detailed
// description in Server.cpp.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "GattInterface.h"
#include "GattProperty.h"
#include "DBusObject.h"
#include "Logger.h"

namespace ggk {

//
// Standard constructor
//
GattInterface::GattInterface(DBusObject &owner, const std::string &name)
: DBusInterface(owner, name)
{
}

GattInterface::~GattInterface()
{
}

//
// GATT Characteristic properties
//

// Returns the list of GATT properties
const std::list<GattProperty> &GattInterface::getProperties() const
{
	return properties;
}

// When responding to a method, we need to return a GVariant value wrapped in a tuple. This method will simplify this slightly by
// wrapping a GVariant of the type "ay" and wrapping it in a tuple before sending it off as the method response.
//
// This is the generalized form that accepts a GVariant *. There is a templated helper method (`methodReturnValue()`) that accepts
// common types.
void GattInterface::methodReturnVariant(GDBusMethodInvocation *pInvocation, GVariant *pVariant, bool wrapInTuple) const
{
	if (wrapInTuple)
	{
		pVariant = g_variant_new_tuple(&pVariant, 1);
	}
	g_dbus_method_invocation_return_value(pInvocation, pVariant);
}

// Locates a `GattProperty` within the interface
//
// This method returns a pointer to the property or nullptr if not found
const GattProperty *GattInterface::findProperty(const std::string &name) const
{
	for (const GattProperty &property : properties)
	{
		if (property.getName() == name)
		{
			return &property;
		}
	}

	return nullptr;
}

// Internal method used to generate introspection XML used to describe our services on D-Bus
std::string GattInterface::generateIntrospectionXML(int depth) const
{
	std::string prefix;
	prefix.insert(0, depth * 2, ' ');

	std::string xml = std::string();

	if (methods.size() && getProperties().empty())
	{
		xml += prefix + "<interface name='" + getName() + "' />\n";
	}
	else
	{
		xml += prefix + "<interface name='" + getName() + "'>\n";

		// Describe our methods
		for (const DBusMethod &method : methods)
		{
			xml += method.generateIntrospectionXML(depth + 1);
		}

		// Describe our properties
		for (const GattProperty &property : getProperties())
		{
			xml += property.generateIntrospectionXML(depth + 1);
		}

		xml += prefix + "</interface>\n";
	}

	return xml;
}

}; // namespace ggk
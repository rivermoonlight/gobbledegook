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
// This is an abstraction layer for a D-Bus interface, the base class for all interfaces.
//
// >>
// >>>  DISCUSSION
// >>
//
// Not sure what a D-Bus Interface is? Well, chedk the Readme for resources, but here's the TL;DR:
//
// A D-Bus interface is a contract (similar to programming language interfaces.) An interface defines a set of methods and
// properties for others to use.
//
// Interfaces are identified by their name, such as "org.freedesktop.DBus.Properties". In fact, if an object on the bus is found
// to have that interface, then you know that it provides an interface to access its properties via the methods "Get", "GetAll" and
// "Set". To see the details for this interface (and all of the D-Bus defined interfaces), see:
//
//     https://dbus.freedesktop.org/doc/dbus-specification.html
//
// We're also interested in working with BlueZ which has their own set of interfaces. One example is "org.bluez.GattManager1" which
// is the interface used to create and register GATT services with BlueZ.
//
// Remember, interfaces are not implementations; they're just contracts to provide an implementation. That means some interfaces
// are intended for us to implement. One example is "org.bluez.GattService1" which defines the interface that we must conform to
// so that others (likely BlueZ) can access our GATT service(s). For more information on these, have a look at:
//
//     https://git.kernel.org/pub/scm/bluetooth/bluez.git/plain/doc/gatt-api.txt
//
// Our interfaces also store a collection of events. Here, an event is much like a timer in modern UIs, which repeatedly fires
// after a defined time. A practical example of an event would be a BLE server that provides a Battery service. By adding a timer
// to the interface for this service, the server could wake up every minute to check the battery level and if it has changed, send
// a notifications to clients over BLE with the new battery level. This saves a lot of additional code on the server's part.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "DBusInterface.h"
#include "GattProperty.h"
#include "DBusObject.h"
#include "Logger.h"

namespace ggk {

//
// Construction
//

DBusInterface::DBusInterface(DBusObject &owner, const std::string &name)
: owner(owner), name(name)
{
}

DBusInterface::~DBusInterface()
{
}

//
// Interface name
//

// Returns the name of this interface (ex: "org.freedesktop.DBus.Properties")
const std::string &DBusInterface::getName() const
{
	return name;
}

// Sets the name of the interface (ex: "org.freedesktop.DBus.Properties")
DBusInterface &DBusInterface::setName(const std::string &name)
{
	this->name = name;
	return *this;
}

//
// Owner information
//

// Returns the owner (DBusObject) of this interface
DBusObject &DBusInterface::getOwner() const
{
	return owner;
}

// Returns the path node of this interface's owner
DBusObjectPath DBusInterface::getPathNode() const
{
	return owner.getPathNode();
}

// Returns the full path of this interface's owner
DBusObjectPath DBusInterface::getPath() const
{
	return owner.getPath();
}

//
// D-Bus interface methods
//

// Add a named method to this interface
//
// This method returns a reference to `this` in order to enable chaining inside the server description.
DBusInterface &DBusInterface::addMethod(const std::string &name, const char *pInArgs[], const char *pOutArgs, DBusMethod::Callback callback)
{
	methods.push_back(DBusMethod(this, name, pInArgs, pOutArgs, callback));
	return *this;
}

// Calls a named method on this interface
//
// This method returns false if the method could not be found, otherwise it returns true. Note that the return value is not related
// to the result of the method call itself (methods do not return values.)
//
// NOTE: Subclasses are encouraged to override this method in order to support different callback types that are specific to
// their subclass type.
bool DBusInterface::callMethod(const std::string &methodName, GDBusConnection *pConnection, GVariant *pParameters, GDBusMethodInvocation *pInvocation, gpointer pUserData) const
{
	for (const DBusMethod &method : methods)
	{
		if (methodName == method.getName())
		{
			method.call<DBusInterface>(pConnection, getPath(), getName(), methodName, pParameters, pInvocation, pUserData);
			return true;
		}
	}

	return false;
}

// Add an event to this interface
//
// For details on events, see TickEvent.cpp.
//
// This method returns a reference to `this` in order to enable chaining inside the server description.
//
// NOTE: Subclasses are encouraged to overload this method in order to support different callback types that are specific to
// their subclass type. In addition, they should return their own type. This simplifies the server description by allowing
// calls to chain.
DBusInterface &DBusInterface::onEvent(int tickFrequency, void *pUserData, TickEvent::Callback callback)
{
	events.push_back(TickEvent(this, tickFrequency, callback, pUserData));
	return *this;
}

// Ticks each event within this interface
//
// For details on events, see TickEvent.cpp.
//
// NOTE: Subclasses are encouraged to override this method in order to support different callback types that are specific to
// their subclass type.
void DBusInterface::tickEvents(GDBusConnection *pConnection, void *pUserData) const
{
	for (const TickEvent &event : events)
	{
		event.tick<DBusInterface>(getPath(), pConnection, pUserData);
	}
}

// Internal method used to generate introspection XML used to describe our services on D-Bus
std::string DBusInterface::generateIntrospectionXML(int depth) const
{
	std::string prefix;
	prefix.insert(0, depth * 2, ' ');

	std::string xml = std::string();

	if (methods.empty())
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

		xml += prefix + "</interface>\n";
	}

	return xml;
}

}; // namespace ggk
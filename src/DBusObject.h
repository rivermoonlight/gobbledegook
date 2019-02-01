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
// See the discussino at the top of DBusObject.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <gio/gio.h>
#include <string>
#include <list>
#include <memory>

#include "DBusObjectPath.h"

namespace ggk {

struct GattProperty;
struct GattService;
struct GattUuid;
struct DBusInterface;

struct DBusObject
{
	// A convenience typedef for describing our list of interface
	typedef std::list<std::shared_ptr<DBusInterface> > InterfaceList;

	// Construct a root object with no parent
	//
	// We'll include a publish flag since only root objects can be published
	DBusObject(const DBusObjectPath &path, bool publish = true);

	// Construct a node object
	//
	// Nodes inherit their parent's publish path
	DBusObject(DBusObject *pParent, const DBusObjectPath &pathElement);

	//
	// Accessors
	//

	// Returns the `publish` flag
	bool isPublished() const;

	// Returns the path node for this object within the hierarchy
	//
	// This method only returns the node. To get the full path, use `getPath()`
	const DBusObjectPath &getPathNode() const;

	// Returns the full path for this object within the hierarchy
	//
	// This method returns the full path. To get the current node, use `getPathNode()`
	DBusObjectPath getPath() const;

	// Returns the parent object in the hierarchy
	DBusObject &getParent();

	// Returns the list of children objects
	const std::list<DBusObject> &getChildren() const;

	// Add a child to this object
	DBusObject &addChild(const DBusObjectPath &pathElement);

	// Returns a list of interfaces for this object
	const InterfaceList &getInterfaces() const;

	// Templated method for adding typed interfaces to the object
	template<typename T>
	std::shared_ptr<T> addInterface(std::shared_ptr<T> interface)
	{
		interfaces.push_back(interface);
		return std::static_pointer_cast<T>(interfaces.back());
	}

	// Internal method used to generate introspection XML used to describe our services on D-Bus
	std::string generateIntrospectionXML(int depth = 0) const;

	// Convenience functions to add a GATT service to the hierarchy
	//
	// We simply add a new child at the given path and add an interface configured as a GATT service to it using the given UUID.
	//
	// To end a service, call `gattServiceEnd()`
	GattService &gattServiceBegin(const std::string &pathElement, const GattUuid &uuid);

	//
	// Helpful routines for searching objects
	//

	// Finds an interface by name within this D-Bus object
	std::shared_ptr<const DBusInterface> findInterface(const DBusObjectPath &path, const std::string &interfaceName, const DBusObjectPath &basePath = DBusObjectPath()) const;

	// Finds a BlueZ method by name within the specified D-Bus interface
	bool callMethod(const DBusObjectPath &path, const std::string &interfaceName, const std::string &methodName, GDBusConnection *pConnection, GVariant *pParameters, GDBusMethodInvocation *pInvocation, gpointer pUserData, const DBusObjectPath &basePath = DBusObjectPath()) const;

	// Periodic timer tick propagation
	void tickEvents(GDBusConnection *pConnection, void *pUserData = nullptr) const;

	// -----------------------------------------------------------------------------------------------------------------------------
	// D-Bus signals
	// -----------------------------------------------------------------------------------------------------------------------------

	// Emits a signal on the bus from the given path, interface name and signal name, containing a GVariant set of parameters
	void emitSignal(GDBusConnection *pBusConnection, const std::string &interfaceName, const std::string &signalName, GVariant *pParameters);

private:
	bool publish;
	DBusObjectPath path;
	InterfaceList interfaces;
	std::list<DBusObject> children;
	DBusObject *pParent;
};

}; // namespace ggk
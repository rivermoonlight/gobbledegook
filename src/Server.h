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
// This is the top-level interface for the server. There is only one of these stored in the global `TheServer`. Use this object
// to configure your server's settings (there are surprisingly few of them.) It also contains the full server description and
// implementation.
//
// >>
// >>>  DISCUSSION
// >>
//
// See the discussion at the top of Server.cpp
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <string>
#include <list>
#include <memory>

#include "DBusInterface.h"
#include "DBusObject.h"

namespace ggk {

//
// Forward declarations
//

struct GattProperty;

//
// Implementation
//

struct Server
{
	//
	// Types
	//

	// Our server is a collection of D-Bus objects
	typedef std::list<DBusObject> Objects;

	//
	// Accessors
	//

	// Returns the set of objects that each represent the root of an object tree describing a group of services we are providing
	const Objects &getObjects() const { return objects; }

	// Returns the requested setting for BR/EDR (true = enabled, false = disabled)
	bool getEnableBREDR() const { return enableBREDR; }

	// Returns the requested setting for secure connections (true = enabled, false = disabled)
	bool getEnableSecureConnection() const { return enableSecureConnection; }

	// Returns the requested setting the connectable state (true = enabled, false = disabled)
	bool getEnableConnectable() const { return enableConnectable; }

	// Returns the requested setting the LE advertising state (true = enabled, false = disabled)
	bool getEnableAdvertising() const { return enableAdvertising; }

	// Returns the requested setting the bondable state (true = enabled, false = disabled)
	bool getEnableBondable() const { return enableBondable; }

	// Returns our registered data getter
	GGKServerDataGetter getDataGetter() const { return dataGetter; }

	// Returns our registered data setter
	GGKServerDataSetter getDataSetter() const { return dataSetter; }

	//
	// Initialization
	//

	// Our constructor builds our entire server description
	Server(GGKServerDataGetter getter, GGKServerDataSetter setter);

	//
	// Utilitarian
	//

	// Find and call a D-Bus method within the given D-Bus object on the given D-Bus interface
	//
	// If the method was called, this method returns true, otherwise false.  There is no result from the method call itself.
	std::shared_ptr<const DBusInterface> findInterface(const DBusObjectPath &objectPath, const std::string &interfaceName) const;

	// Find a D-Bus method within the given D-Bus object on the given D-Bus interface
	//
	// If the method was found, it is returned, otherwise nullptr is returned
	bool callMethod(const DBusObjectPath &objectPath, const std::string &interfaceName, const std::string &methodName, GDBusConnection *pConnection, GVariant *pParameters, GDBusMethodInvocation *pInvocation, gpointer pUserData) const;

	// Find a GATT Property within the given D-Bus object on the given D-Bus interface
	//
	// If the property was found, it is returned, otherwise nullptr is returned
	const GattProperty *findProperty(const DBusObjectPath &objectPath, const std::string &interfaceName, const std::string &propertyName) const;

private:

	// Our server's objects
	Objects objects;

	// BR/EDR requested state
	bool enableBREDR;

	// Secure connection requested state
	bool enableSecureConnection;

	// Connectable requested state
	bool enableConnectable;

	// LE advertising requested state
	bool enableAdvertising;

	// Bondable requested state
	bool enableBondable;

	// The getter callback that is responsible for returning current server data that is shared over Bluetooth
	GGKServerDataGetter dataGetter;

	// The setter callback that is responsible for storing current server data that is shared over Bluetooth
	GGKServerDataSetter dataSetter;
};

// Our one and only server. It's a global.
extern std::shared_ptr<Server> TheServer;

}; // namespace ggk

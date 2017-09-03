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

#include <gio/gio.h>
#include <string>
#include <vector>
#include <list>
#include <memory>

#include "../include/Gobbledegook.h"
#include "DBusObject.h"

namespace ggk {

//
// Forward declarations
//

struct GattProperty;
struct GattCharacteristic;
struct DBusInterface;
struct DBusObjectPath;

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

	// advertisingName: The name for this controller, as advertised over LE
	//
	// This is set from the constructor.
	//
	// IMPORTANT: Setting the advertisingName will change the system-wide name of the device. If that's not what you want, set
	// BOTH advertisingName and advertisingShortName to as empty string ("") to prevent setting the advertising
	// name.
	const std::string &getAdvertisingName() const { return advertisingName; }

	// advertisingShortName: The short name for this controller, as advertised over LE
	//
	// According to the spec, the short name is used in case the full name doesn't fit within Extended Inquiry Response (EIR) or
	// Advertising Data (AD).
	//
	// This is set from the constructor.
	//
	// IMPORTANT: Setting the advertisingName will change the system-wide name of the device. If that's not what you want, set
	// BOTH advertisingName and advertisingShortName to as empty string ("") to prevent setting the advertising
	// name.
	const std::string &getAdvertisingShortName() const { return advertisingShortName; }

	// serviceName: The name of our server (collectino of services)
	//
	// This is set from the constructor.
	//
	// This is used to build the path for our Bluetooth services (and we'll go ahead and use it as the owned name as well for
	// consistency.)
	const std::string &getServiceName() const { return serviceName; }

	// Our owned name
	//
	// D-Bus uses owned names to locate servers on the bus. Think of this as a namespace within D-Bus. We building this with the
	// server name to keep things simple.
	std::string getOwnedName() const { return std::string("com.") + getServiceName(); }

	//
	// Initialization
	//

	// Our constructor builds our entire server description
	//
	// serviceName: The name of our server (collectino of services)
	//
	//     This is used to build the path for our Bluetooth services. It also provides the base for the D-Bus owned name (see
	//     getOwnedName.)
	//
	//     This value will be stored as lower-case only.
	//
	//     Retrieve this value using the `getName()` method.
	//
	// advertisingName: The name for this controller, as advertised over LE
	//
	//     IMPORTANT: Setting the advertisingName will change the system-wide name of the device. If that's not what you want, set
	//     BOTH advertisingName and advertisingShortName to as empty string ("") to prevent setting the advertising
	//     name.
	//
	//     Retrieve this value using the `getAdvertisingName()` method.
	//
	// advertisingShortName: The short name for this controller, as advertised over LE
	//
	//     According to the spec, the short name is used in case the full name doesn't fit within Extended Inquiry Response (EIR) or
	//     Advertising Data (AD).
	//
	//     IMPORTANT: Setting the advertisingName will change the system-wide name of the device. If that's not what you want, set
	//     BOTH advertisingName and advertisingShortName to as empty string ("") to prevent setting the advertising
	//     name.
	//
	//     Retrieve this value using the `getAdvertisingShortName()` method.
	//
	Server(const std::string &serviceName, const std::string &advertisingName, const std::string &advertisingShortName, 
		GGKServerDataGetter getter, GGKServerDataSetter setter);

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

	// advertisingName: The name for this controller, as advertised over LE
	//
	// This is set from the constructor.
	//
	// IMPORTANT: Setting the advertisingName will change the system-wide name of the device. If that's not what you want, set
	// BOTH advertisingName and advertisingShortName to as empty string ("") to prevent setting the advertising
	// name.
	std::string advertisingName;

	// advertisingShortName: The short name for this controller, as advertised over LE
	//
	// According to the spec, the short name is used in case the full name doesn't fit within Extended Inquiry Response (EIR) or
	// Advertising Data (AD).
	//
	// This is set from the constructor.
	//
	// IMPORTANT: Setting the advertisingName will change the system-wide name of the device. If that's not what you want, set
	// BOTH advertisingName and advertisingShortName to as empty string ("") to prevent setting the advertising
	// name.
	std::string advertisingShortName;

	// serviceName: The name of our server (collectino of services)
	//
	// This is set from the constructor.
	//
	// This is used to build the path for our Bluetooth services (and we'll go ahead and use it as the owned name as well for
	// consistency.)
	std::string serviceName;
};

// Our one and only server. It's a global.
extern std::shared_ptr<Server> TheServer;

}; // namespace ggk

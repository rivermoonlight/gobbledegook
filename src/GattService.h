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
// A GATT Service, used to add services to a Bluetooth server
//
// >>
// >>>  DISCUSSION
// >>
//
// A GATT Service is really a collection of 
//
// This class is intended to be used within the server description. For an explanation of how this class is used, see the detailed
// description in Server.cpp.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <gio/gio.h>
#include <string>
#include <list>

#include "TickEvent.h"
#include "GattInterface.h"

namespace ggk {

// ---------------------------------------------------------------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------------------------------------------------------------

struct GattService;
struct GattCharacteristic;
struct GattProperty;
struct DBusObject;

// ---------------------------------------------------------------------------------------------------------------------------------
// Representation of a Bluetooth GATT Service
// ---------------------------------------------------------------------------------------------------------------------------------

struct GattService : GattInterface
{
	// Our interface type
	static constexpr const char *kInterfaceType = "GattService";

	// Standard constructor
	GattService(DBusObject &owner, const std::string &name);

	virtual ~GattService() {}

	// Returning the parent pops us one level up the hierarchy
	DBusObject &gattServiceEnd();

	// Convenience functions to add a GATT characteristic to the hierarchy
	//
	// We simply add a new child at the given path and add an interface configured as a GATT characteristic to it. The
	// new characteristic is declared with a UUID and a variable argument list of flags (in string form.) For a complete and
	// up-to-date list of flag values, see: https://git.kernel.org/pub/scm/bluetooth/bluez.git/plain/doc/gatt-api.txt
	//
	// At the time of this writing, the list of flags is as follows:
	//
	//     "broadcast"
	//     "read"
	//     "write-without-response"
	//     "write"
	//     "notify"
	//     "indicate"
	//     "authenticated-signed-writes"
	//     "reliable-write"
	//     "writable-auxiliaries"
	//     "encrypt-read"
	//     "encrypt-write"
	//     "encrypt-authenticated-read"
	//     "encrypt-authenticated-write"
	//     "secure-read" (Server only)
	//     "secure-write" (Server only)
	//
	GattCharacteristic &gattCharacteristicBegin(const std::string &pathElement, const GattUuid &uuid, const std::vector<const char *> &flags);

	// Returns a string identifying the type of interface
	virtual const std::string getInterfaceType() const { return GattService::kInterfaceType; }
};

}; // namespace ggk
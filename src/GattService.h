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
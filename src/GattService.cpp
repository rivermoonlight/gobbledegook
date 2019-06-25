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

#include <gio/gio.h>
#include <string>
#include <list>

#include "GattService.h"
#include "GattInterface.h"
#include "DBusObject.h"
#include "GattCharacteristic.h"

namespace ggk {

// ---------------------------------------------------------------------------------------------------------------------------------
// Representation of a Bluetooth GATT Service
// ---------------------------------------------------------------------------------------------------------------------------------

// Standard constructor
GattService::GattService(DBusObject &owner, const std::string &name)
: GattInterface(owner, name)
{
}

// Returning the parent pops us one level up the hierarchy
DBusObject &GattService::gattServiceEnd()
{
	return getOwner().getParent();
}

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
GattCharacteristic &GattService::gattCharacteristicBegin(const std::string &pathElement, const GattUuid &uuid, const std::vector<const char *> &flags)
{
	DBusObject &child = owner.addChild(DBusObjectPath(pathElement));
	GattCharacteristic &characteristic = *child.addInterface(std::make_shared<GattCharacteristic>(child, *this, "org.bluez.GattCharacteristic1"));
	characteristic.addProperty<GattCharacteristic>("UUID", uuid);
	characteristic.addProperty<GattCharacteristic>("Service", owner.getPath());
	characteristic.addProperty<GattCharacteristic>("Flags", flags);
	return characteristic;
}

}; // namespace ggk
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
// This file contains helper function declarations for our server's implementation. That is, methods that are used by the server
// itself (when responding to D-Bus or BlueZ requests.)
//
// >>
// >>>  DISCUSSION
// >>
//
// See ServerUtils.cpp for implementations
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <gio/gio.h>
#include <string>

namespace ggk {

struct ServerUtils
{
	// Builds the response to the method call `GetManagedObjects` from the D-Bus interface `org.freedesktop.DBus.ObjectManager`
	static void getManagedObjects(GDBusMethodInvocation *pInvocation);

	// WARNING: Hacky code - don't count on this working properly on all systems
	//
	// This routine will attempt to parse /proc/cpuinfo to return the CPU count/model. Results are cached on the first call, with
	// cached results returned on successive calls.
	//
	// If this routine fails, it will respond with something reasonable, if not _entirely_ accurate.
	static std::string getCpuInfo(int16_t &cpuCount);

	// Build a variant that meets the standard for the Current Time (0x2A2B) Bluetooth Characteristic standard
	//
	// See: https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.current_time.xml
	static GVariant *gvariantCurrentTime();

	// Build a variant that meets the standard for the Local Time Information (0x2A0F) Bluetooth Characteristic standard
	//
	// See: https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.local_time_information.xml
	static GVariant *gvariantLocalTime();
};

}; // namespace ggk
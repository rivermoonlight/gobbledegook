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
// System-wide globals.
//
// >>
// >>>  DISCUSSION
// >>
//
// The globals below define the name of the server, along with any name-based items (such as errors.)
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <string>

// The name for this controller, as advertised over LE
//
// IMPORTANT: Setting the advertisingName will change the system-wide name of the device. If that's not what you want, set BOTH
// kCustomGlobalAdvertisingName and kCustomGlobalAdvertisingShortName to as empty string ("") to prevent setting the advertising
// name.
#define kCustomGlobalAdvertisingName std::string("Gobbledegook")

// The short name for this controller, as advertised over LE
//
// According to the spec, the short name is used in case the full name doesn't fit within Extended Inquiry Response (EIR) or
// Advertising Data (AD).
//
// IMPORTANT: Setting the advertisingName will change the system-wide name of the device. If that's not what you want, set BOTH
// kCustomGlobalAdvertisingName and kCustomGlobalAdvertisingShortName to as empty string ("") to prevent setting the advertising
// name.
#define kCustomGlobalAdvertisingShortName std::string("Gobbledegook")

// The name of our server (collectino of services)
//
// This is used to build the path for our Bluetooth services (and we'll go ahead and use it as the owned name as well for
// consistency.)
#define kServerName std::string("gobbledegook")

// Our owned name
//
// D-Bus uses owned names to locate servers on the bus. Think of this as a namespace within D-Bus. Building this with the server
// name, though it's not necessary to do so. We can call this anything we want, really.
#define kServerOwnedName (std::string("com.") + kServerName)

//
// Custom defined errors
//
// In order to avoid confusion, we should use the owned name here, so errors are like extensions to that name. This way, if a
// client gets one of these errors, it'll be clear which server it came from.
#define kErrorNotImplemented (kServerOwnedName + ".NotImplemented")

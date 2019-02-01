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
// This file contains various general utilitarian functions used throught. It is in some ways, the 'junk drawer' of the appliation,
// though better organized than most physical junk drawers.
//
// >>
// >>>  DISCUSSION
// >>
//
// This file contains:
//
//     - String helper functions (trimming methods)
//     - Hexidecimal helper functions for
//       + Producing hex values of various types (8-bit, 16-bit, 32-bit)
//       + Standardied Hex/ASCII dumps to the log file of chunks of binary data
//       + Properly formatted Bluetooth addresses)
//     - GVariant helper funcions of various forms to convert values to/from GVariants
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <glib.h>
#include <gio/gio.h>
#include <vector>
#include <string>
#include <endian.h>

#include "DBusObjectPath.h"

namespace ggk {

struct Utils
{
	// -----------------------------------------------------------------------------------------------------------------------------
	// Handy string functions
	// -----------------------------------------------------------------------------------------------------------------------------

	// Trim from start (in place)
	static void trimBeginInPlace(std::string &str);

	// Trim from end (in place)
	static void trimEndInPlace(std::string &str);

	// Trim from both ends (in place)
	static void trimInPlace(std::string &str);

	// Trim from start (copying)
	static std::string trimBegin(const std::string &str);

	// Trim from end (copying)
	static std::string trimEnd(const std::string &str);

	// Trim from both ends (copying)
	static std::string trim(const std::string &str);

	// -----------------------------------------------------------------------------------------------------------------------------
	// Hex output functions
	// -----------------------------------------------------------------------------------------------------------------------------

	// Returns a zero-padded 8-bit hex value in the format: 0xA
	static std::string hex(uint8_t value);

	// Returns a zero-padded 8-bit hex value in the format: 0xAB
	static std::string hex(uint16_t value);

	// Returns a zero-padded 8-bit hex value in the format: 0xABCD
	static std::string hex(uint32_t value);

	// A full hex-dump of binary data (with accompanying ASCII output)
	static std::string hex(const uint8_t *pData, int count);

	// Returns a peoperly formatted Bluetooth address from a set of six octets stored at `pAddress`
	//
	// USE WITH CAUTION: It is expected that pAddress point to an array of 6 bytes. The length of the array cannot be validated and
	// incorrect lengths will produce undefined, likely unwanted and potentially fatal results. Or it will return the address of the
	// train at platform 9 3/4. You decide.
	//
	// This method returns a set of six zero-padded 8-bit hex values 8-bit in the format: 12:34:56:78:9A:BC
	static std::string bluetoothAddressString(uint8_t *pAddress);

	// -----------------------------------------------------------------------------------------------------------------------------
	// A small collection of helper functions for generating various types of GVariants, which are needed when responding to BlueZ
	// method/property messages. Real services will likley need more of these to support various types of data passed to/from BlueZ,
	// or feel free to do away with them and use GLib directly.
	// -----------------------------------------------------------------------------------------------------------------------------

	// Returns a GVariant containing a floating reference to a utf8 string
	static GVariant *gvariantFromString(const char *pStr);

	// Returns a GVariant containing a floating reference to a utf8 string
	static GVariant *gvariantFromString(const std::string &str);

	// Returns an array of strings ("as") with one string per variable argument.
	//
	// The array must be terminated with a nullptr.
	//
	// This is an extension method to the vararg version, which accepts pass-through variable arguments from other mthods.
	static GVariant *gvariantFromStringArray(const char *pStr, va_list args);

	// Returns an array of strings ("as") with one string per variable argument.
	//
	// The array must be terminated with a nullptr.
	static GVariant *gvariantFromStringArray(const char *pStr, ...);

	// Returns an array of strings ("as") from an array of strings
	static GVariant *gvariantFromStringArray(const std::vector<std::string> &arr);

	// Returns an array of strings ("as") from an array of C strings
	static GVariant *gvariantFromStringArray(const std::vector<const char *> &arr);

	// Returns an GVariant* containing an object path ("o") from an DBusObjectPath
	static GVariant *gvariantFromObject(const DBusObjectPath &path);

	// Returns an GVariant* containing a boolean
	static GVariant *gvariantFromBoolean(bool b);

	// Returns an GVariant* containing a 16-bit integer
	static GVariant *gvariantFromInt(gint16 value);

	// Returns an GVariant* containing a 32-bit integer
	static GVariant *gvariantFromInt(gint32 value);

	// Returns an array of bytes ("ay") with the contents of the input C string
	static GVariant *gvariantFromByteArray(const char *pStr);

	// Returns an array of bytes ("ay") with the contents of the input string
	static GVariant *gvariantFromByteArray(const std::string &str);

	// Returns an array of bytes ("ay") with the contents of the input array of unsigned 8-bit values
	static GVariant *gvariantFromByteArray(const guint8 *pBytes, int count);

	// Returns an array of bytes ("ay") with the contents of the input array of unsigned 8-bit values
	static GVariant *gvariantFromByteArray(const std::vector<guint8> bytes);

	// Returns an array of bytes ("ay") containing a single unsigned 8-bit value
	static GVariant *gvariantFromByteArray(const guint8 data);

	// Returns an array of bytes ("ay") containing a single signed 8-bit value
	static GVariant *gvariantFromByteArray(const gint8 data);

	// Returns an array of bytes ("ay") containing a single unsigned 16-bit value
	static GVariant *gvariantFromByteArray(const guint16 data);

	// Returns an array of bytes ("ay") containing a single signed 16-bit value
	static GVariant *gvariantFromByteArray(const gint16 data);

	// Returns an array of bytes ("ay") containing a single unsigned 32-bit value
	static GVariant *gvariantFromByteArray(const guint32 data);

	// Returns an array of bytes ("ay") containing a single signed 32-bit value
	static GVariant *gvariantFromByteArray(const gint32 data);

	// Returns an array of bytes ("ay") containing a single unsigned 64-bit value
	static GVariant *gvariantFromByteArray(const guint64 data);

	// Returns an array of bytes ("ay") containing a single signed 64-bit value
	static GVariant *gvariantFromByteArray(const gint64 data);

	// Extracts a string from an array of bytes ("ay")
	static std::string stringFromGVariantByteArray(const GVariant *pVariant);

	// -----------------------------------------------------------------------------------------------------------------------------
	// Endian conversion
	//
	// The Bluetooth Management API defines itself has using little-endian byte order. In the methods below, 'Hci' refers to this
	// format, while 'Host' refers to the endianness of the hardware we are running on.
	//
	// The `Utils::endianToHost()` method overloads perform endian byte-ordering conversions from the HCI to our endian format
	// The `Utils::endianToHci()` method overloads perform endian byte-ordering conversions from our endian format to that of the HCI
	// -----------------------------------------------------------------------------------------------------------------------------

	// Convert a byte from HCI format to host format
	//
	// Since bytes are endian agnostic, this function simply returns the input value
	static uint8_t endianToHost(uint8_t value) {return value;}

	// Convert a byte from host format to HCI format
	//
	// Since bytes are endian agnostic, this function simply returns the input value
	static uint8_t endianToHci(uint8_t value) {return value;}

	// Convert a 16-bit value from HCI format to host format
	static uint16_t endianToHost(uint16_t value) {return le16toh(value);}

	// Convert a 16-bit value from host format to HCI format
	static uint16_t endianToHci(uint16_t value) {return htole16(value);}

	// Convert a 32-bit value from HCI format to host format
	static uint32_t endianToHost(uint32_t value) {return le32toh(value);}

	// Convert a 32-bit value from host format to HCI format
	static uint32_t endianToHci(uint32_t value) {return htole32(value);}
};

}; // namespace ggk
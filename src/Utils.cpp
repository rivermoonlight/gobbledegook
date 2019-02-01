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

#include <algorithm>
#include <string.h>

#include "Utils.h"

namespace ggk {

// ---------------------------------------------------------------------------------------------------------------------------------
// Handy string functions
// ---------------------------------------------------------------------------------------------------------------------------------

// Trim from start (in place)
void Utils::trimBeginInPlace(std::string &str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(),
    [](int ch)
    {
        return !std::isspace(ch);
    }));
}

// Trim from end (in place)
void Utils::trimEndInPlace(std::string &str)
{
    str.erase(std::find_if(str.rbegin(), str.rend(),
    [](int ch)
    {
        return !std::isspace(ch);
    }).base(), str.end());
}

// Trim from both ends (in place)
void Utils::trimInPlace(std::string &str)
{
    trimBeginInPlace(str);
    trimEndInPlace(str);
}

// Trim from start (copying)
std::string Utils::trimBegin(const std::string &str)
{
	std::string out = str;
    trimBeginInPlace(out);
    return out;
}

// Trim from end (copying)
std::string Utils::trimEnd(const std::string &str)
{
	std::string out = str;
    trimEndInPlace(out);
    return out;
}

// Trim from both ends (copying)
std::string Utils::trim(const std::string &str)
{
	std::string out = str;
    trimInPlace(out);
    return out;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// Hex output functions
// ---------------------------------------------------------------------------------------------------------------------------------

// Returns a zero-padded 8-bit hex value in the format: 0xA
std::string Utils::hex(uint8_t value)
{
	char hex[16];
	sprintf(hex, "0x%02X", value);
	return hex;
}

// Returns a zero-padded 8-bit hex value in the format: 0xAB
std::string Utils::hex(uint16_t value)
{
	char hex[16];
	sprintf(hex, "0x%04X", value);
	return hex;
}

// Returns a zero-padded 8-bit hex value in the format: 0xABCD
std::string Utils::hex(uint32_t value)
{
	char hex[16];
	sprintf(hex, "0x%08X", value);
	return hex;
}

// A full hex-dump of binary data (with accompanying ASCII output)
std::string Utils::hex(const uint8_t *pData, int count)
{
	char hex[16];

	// Hex data output
	std::string line;
	std::vector<std::string> hexData;
	for (int i = 0; i < count; ++i)
	{
		sprintf(hex, "%02X ", pData[i]);
		line += hex;

		if (line.length() >= 16 * 3)
		{
			hexData.push_back(line);
			line = "";
		}
	}

	if (!line.empty())
	{
		hexData.push_back(line);
		line = "";
	}

	// ASCII data output
	std::vector<std::string> asciiData;
	for (int i = 0; i < count; ++i)
	{
		// Unprintable?
		if (pData[i] < 0x20 || pData[i] > 0x7e)
		{
			line += ".";
		}
		else
		{
			line += pData[i];
		}

		if (line.length() >= 16)
		{
			asciiData.push_back(line);
			line = "";
		}
	}

	if (!line.empty())
	{
		asciiData.push_back(line);
	}

	std::string result = "";
	size_t dataSize = hexData.size();
	for (size_t i = 0; i < dataSize; ++i)
	{
		std::string hexPart = hexData[i];
		hexPart.insert(hexPart.length(), 48-hexPart.length(), ' ');

		std::string asciiPart = asciiData[i];
		asciiPart.insert(asciiPart.length(), 16-asciiPart.length(), ' ');

		result += std::string("    > ") + hexPart + "   [" + asciiPart + "]";

		if (i < dataSize - 1) { result += "\n"; }
	}

	return result;
}

// Returns a peoperly formatted Bluetooth address from a set of six octets stored at `pAddress`
//
// USE WITH CAUTION: It is expected that pAddress point to an array of 6 bytes. The length of the array cannot be validated and
// incorrect lengths will produce undefined, likely unwanted and potentially fatal results. Or it will return the address of the
// train at platform 9 3/4. You decide.
//
// This method returns a set of six zero-padded 8-bit hex values 8-bit in the format: 12:34:56:78:9A:BC
std::string Utils::bluetoothAddressString(uint8_t *pAddress)
{
	char hex[32];
	snprintf(hex, sizeof(hex), "%02X:%02X:%02X:%02X:%02X:%02X", 
		pAddress[0], pAddress[1], pAddress[2], pAddress[3], pAddress[4], pAddress[5]);
	return hex;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// GVariant helper functions
// ---------------------------------------------------------------------------------------------------------------------------------

// Returns a GVariant containing a floating reference to a utf8 string
GVariant *Utils::gvariantFromString(const char *pStr)
{
	return g_variant_new_string(pStr);
}

// Returns a GVariant containing a floating reference to a utf8 string
GVariant *Utils::gvariantFromString(const std::string &str)
{
	return g_variant_new_string(str.c_str());
}

// Returns an array of strings ("as") with one string per variable argument.
//
// The array must be terminated with a nullptr.
//
// This is an extension method to the vararg version, which accepts pass-through variable arguments from other mthods.
GVariant *Utils::gvariantFromStringArray(const char *pStr, va_list args)
{
	// Deal with empty arrays
	if (pStr == 0)
	{
		return g_variant_new("as", nullptr);
	}

	g_auto(GVariantBuilder) builder;
	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

	while(nullptr != pStr)
	{
		g_variant_builder_add(&builder, "s", pStr);
		pStr = va_arg(args, const char *);
	}

	return g_variant_builder_end(&builder);
}

// Returns an array of strings ("as") with one string per variable argument.
//
// The array must be terminated with a nullptr.
GVariant *Utils::gvariantFromStringArray(const char *pStr, ...)
{
	// Deal with empty arrays
	if (pStr == 0)
	{
		return g_variant_new("as", nullptr);
	}

	g_auto(GVariantBuilder) builder;
	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

	va_list args;
	va_start(args, pStr);

	GVariant *pResult = gvariantFromStringArray(pStr, args);

	va_end(args);

	return pResult;
}

// Returns an array of strings ("as") from an array of strings
GVariant *Utils::gvariantFromStringArray(const std::vector<std::string> &arr)
{
	// Deal with empty arrays
	if (arr.empty())
	{
		return g_variant_new("as", nullptr);
	}

	g_auto(GVariantBuilder) builder;
	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

	for (std::string str : arr)
	{
		g_variant_builder_add(&builder, "s", str.c_str());
	}

	return g_variant_builder_end(&builder);
}

// Returns an array of strings ("as") from an array of C strings
GVariant *Utils::gvariantFromStringArray(const std::vector<const char *> &arr)
{
	// Deal with empty arrays
	if (arr.empty())
	{
		return g_variant_new("as", nullptr);
	}

	g_auto(GVariantBuilder) builder;
	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

	for (const char *pStr : arr)
	{
		g_variant_builder_add(&builder, "s", pStr);
	}

	return g_variant_builder_end(&builder);
}

// Returns an GVariant* containing an object path ("o") from an DBusObjectPath
GVariant *Utils::gvariantFromObject(const DBusObjectPath &path)
{
	return g_variant_new_object_path(path.c_str());
}

// Returns an GVariant* containing a boolean
GVariant *Utils::gvariantFromBoolean(bool b)
{
	return g_variant_new_boolean(b);
}

// Returns an GVariant* containing a 16-bit integer
GVariant *Utils::gvariantFromInt(gint16 value)
{
	return g_variant_new_int16(value);
}

// Returns an GVariant* containing a 32-bit integer
GVariant *Utils::gvariantFromInt(gint32 value)
{
	return g_variant_new_int32(value);
}

// Returns an array of bytes ("ay") with the contents of the input C string
GVariant *Utils::gvariantFromByteArray(const char *pStr)
{
	// Deal with empty arrays
	if (*pStr == 0)
	{
		return g_variant_new("ay", nullptr);
	}

	return gvariantFromByteArray(reinterpret_cast<const guint8 *>(pStr), strlen(pStr));
}

// Returns an array of bytes ("ay") with the contents of the input string
GVariant *Utils::gvariantFromByteArray(const std::string &str)
{
	return gvariantFromByteArray(reinterpret_cast<const guint8 *>(str.c_str()), str.length());
}

// Returns an array of bytes ("ay") with the contents of the input array of unsigned 8-bit values
GVariant *Utils::gvariantFromByteArray(const guint8 *pBytes, int count)
{
	GBytes *pGbytes = g_bytes_new(pBytes, count);
	return g_variant_new_from_bytes(G_VARIANT_TYPE_BYTESTRING, pGbytes, count);
}

// Returns an array of bytes ("ay") with the contents of the input array of unsigned 8-bit values
GVariant *Utils::gvariantFromByteArray(const std::vector<guint8> bytes)
{
	GBytes *pGbytes = g_bytes_new(bytes.data(), bytes.size());
	return g_variant_new_from_bytes(G_VARIANT_TYPE_BYTESTRING, pGbytes, bytes.size());
}

// Returns an array of bytes ("ay") containing a single unsigned 8-bit value
GVariant *Utils::gvariantFromByteArray(const guint8 data)
{
	return gvariantFromByteArray((const guint8 *) &data, sizeof(data));
}

// Returns an array of bytes ("ay") containing a single signed 8-bit value
GVariant *Utils::gvariantFromByteArray(const gint8 data)
{
	return gvariantFromByteArray((const guint8 *) &data, sizeof(data));
}

// Returns an array of bytes ("ay") containing a single unsigned 16-bit value
GVariant *Utils::gvariantFromByteArray(const guint16 data)
{
	return gvariantFromByteArray((const guint8 *) &data, sizeof(data));
}

// Returns an array of bytes ("ay") containing a single signed 16-bit value
GVariant *Utils::gvariantFromByteArray(const gint16 data)
{
	return gvariantFromByteArray((const guint8 *) &data, sizeof(data));
}

// Returns an array of bytes ("ay") containing a single unsigned 32-bit value
GVariant *Utils::gvariantFromByteArray(const guint32 data)
{
	return gvariantFromByteArray((const guint8 *) &data, sizeof(data));
}

// Returns an array of bytes ("ay") containing a single signed 32-bit value
GVariant *Utils::gvariantFromByteArray(const gint32 data)
{
	return gvariantFromByteArray((const guint8 *) &data, sizeof(data));
}

// Returns an array of bytes ("ay") containing a single unsigned 64-bit value
GVariant *Utils::gvariantFromByteArray(const guint64 data)
{
	return gvariantFromByteArray((const guint8 *) &data, sizeof(data));
}

// Returns an array of bytes ("ay") containing a single signed 64-bit value
GVariant *Utils::gvariantFromByteArray(const gint64 data)
{
	return gvariantFromByteArray((const guint8 *) &data, sizeof(data));
}

// Extracts a string from an array of bytes ("ay")
std::string Utils::stringFromGVariantByteArray(const GVariant *pVariant)
{
	gsize size;
	gconstpointer pPtr = g_variant_get_fixed_array(const_cast<GVariant *>(pVariant), &size, 1);
	std::vector<gchar> array(size + 1, 0);
	memcpy(array.data(), pPtr, size);
	return array.data();
}

}; // namespace ggk
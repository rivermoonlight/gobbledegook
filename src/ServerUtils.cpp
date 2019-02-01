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
// This file contains helper functions for our server's implementation. That is, methods that are used by the server itself (when
// responding to D-Bus or BlueZ requests.)
//
// >>
// >>>  DISCUSSION
// >>
//
// Generally speaking, these are blocks of code that are too big to comfortably fit as lambdas within the `Server::Server()`
// constructor are here.
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <glib.h>
#include <string>
#include <fstream>
#include <regex>

#include "ServerUtils.h"
#include "DBusObject.h"
#include "DBusInterface.h"
#include "GattProperty.h"
#include "GattService.h"
#include "GattCharacteristic.h"
#include "GattDescriptor.h"
#include "Server.h"
#include "Logger.h"
#include "Utils.h"

namespace ggk {

// Adds an object to the tree of managed objects as returned from the `GetManagedObjects` method call from the D-Bus interface
// `org.freedesktop.DBus.ObjectManager`.
//
// According to the spec:
//
//     The return value of this method is a dict whose keys are object paths.
//     All returned object paths are children of the object path implementing this interface,
//     i.e. their object paths start with the ObjectManager's object path plus '/'.
//
//     Each value is a dict whose keys are interfaces names. Each value in this inner dict
//     is the same dict that would be returned by the org.freedesktop.DBus.Properties.GetAll()
//     method for that combination of object path and interface. If an interface has no properties,
//     the empty dict is returned.
//
//     (a{oa{sa{sv}}})
static void addManagedObjectsNode(const DBusObject &object, const DBusObjectPath &basePath, GVariantBuilder *pObjectArray)
{
	if (!object.isPublished())
	{
		return;
	}

	if (!object.getInterfaces().empty())
	{
		DBusObjectPath path = basePath + object.getPathNode();
		Logger::debug(SSTR << "  Object: " << path);

		GVariantBuilder *pInterfaceArray = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
		for (std::shared_ptr<const DBusInterface> pInterface : object.getInterfaces())
		{
			Logger::debug(SSTR << "  + Interface (type: " << pInterface->getInterfaceType() << ")");

			if (std::shared_ptr<const GattService> pService = TRY_GET_CONST_INTERFACE_OF_TYPE(pInterface, GattService))
			{
				if (!pService->getProperties().empty())
				{
					Logger::debug(SSTR << "    GATT Service interface: " << pService->getName());

					GVariantBuilder *pPropertyArray = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
					for (const GattProperty &property : pService->getProperties())
					{
						Logger::debug(SSTR << "      Property " << property.getName());
						g_variant_builder_add
						(
							pPropertyArray,
							"{sv}",
							property.getName().c_str(),
							property.getValue()
						);
					}

					g_variant_builder_add
					(
						pInterfaceArray,
						"{sa{sv}}",
						pService->getName().c_str(),
						pPropertyArray
					);
				}
			}
			else if (std::shared_ptr<const GattCharacteristic> pCharacteristic = TRY_GET_CONST_INTERFACE_OF_TYPE(pInterface, GattCharacteristic))
			{
				if (!pCharacteristic->getProperties().empty())
				{
					Logger::debug(SSTR << "    GATT Characteristic interface: " << pCharacteristic->getName());

					GVariantBuilder *pPropertyArray = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
					for (const GattProperty &property : pCharacteristic->getProperties())
					{
						Logger::debug(SSTR << "      Property " << property.getName());
						g_variant_builder_add
						(
							pPropertyArray,
							"{sv}",
							property.getName().c_str(),
							property.getValue()
						);
					}

					g_variant_builder_add
					(
						pInterfaceArray,
						"{sa{sv}}",
						pCharacteristic->getName().c_str(),
						pPropertyArray
					);
				}
			}
			else if (std::shared_ptr<const GattDescriptor> pDescriptor = TRY_GET_CONST_INTERFACE_OF_TYPE(pInterface, GattDescriptor))
			{
				if (!pDescriptor->getProperties().empty())
				{
					Logger::debug(SSTR << "    GATT Descriptor interface: " << pDescriptor->getName());

					GVariantBuilder *pPropertyArray = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
					for (const GattProperty &property : pDescriptor->getProperties())
					{
						Logger::debug(SSTR << "      Property " << property.getName());
						g_variant_builder_add
						(
							pPropertyArray,
							"{sv}",
							property.getName().c_str(),
							property.getValue()
						);
					}

					g_variant_builder_add
					(
						pInterfaceArray,
						"{sa{sv}}",
						pDescriptor->getName().c_str(),
						pPropertyArray
					);
				}
			}
			else
			{
				Logger::error(SSTR << "    Unknown interface type");
				return;
			}
		}

		g_variant_builder_add
		(
			pObjectArray,
			"{oa{sa{sv}}}",
			path.c_str(),
			pInterfaceArray
		);
	}

	for (const DBusObject &child : object.getChildren())
	{
		addManagedObjectsNode(child, basePath + object.getPathNode(), pObjectArray);
	}
}

// Builds the response to the method call `GetManagedObjects` from the D-Bus interface `org.freedesktop.DBus.ObjectManager`
void ServerUtils::getManagedObjects(GDBusMethodInvocation *pInvocation)
{
	Logger::debug(SSTR << "Reporting managed objects");

	GVariantBuilder *pObjectArray = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
	for (const DBusObject &object : TheServer->getObjects())
	{
		addManagedObjectsNode(object, DBusObjectPath(""), pObjectArray);
	}

	GVariant *pParams = g_variant_new("(a{oa{sa{sv}}})", pObjectArray);
	g_dbus_method_invocation_return_value(pInvocation, pParams);
}

// WARNING: Hacky code - don't count on this working properly on all systems
//
// This routine will attempt to parse /proc/cpuinfo to return the CPU count/model. Results are cached on the first call, with
// cached results returned on successive calls.
//
// If this routine fails, it will respond with something reasonable, if not _entirely_ accurate.
std::string ServerUtils::getCpuInfo(int16_t &cpuCount)
{
	static int16_t cachedCount = -1;
	static std::string cachedModel;
	static const std::string kCpuInfoFile = "/proc/cpuinfo";

	// If we haven't cached a result, let's go get one
	if (cachedCount == -1)
	{
		// Reset our counter
		cachedCount = 0;

		// Open the cpuinfo file
		std::ifstream cpuInfo(kCpuInfoFile);
		if (cpuInfo.is_open())
		{
			std::string line;
			while(getline(cpuInfo, line))
			{
				// Count the processors
				std::regex processorRegex("^processor.*: [0-9].*$", std::regex::ECMAScript);
				std::smatch processorMatch;

				if (std::regex_search(line, processorMatch, processorRegex))
				{
					cachedCount++;
				}

				// Extract the first model name we find
				if (cachedModel.empty())
				{
					std::regex modelRegex("^model name.*: (.*)$", std::regex::ECMAScript);
					std::smatch modelMatch;
					if (std::regex_search(line, modelMatch, modelRegex))
					{
						if (modelMatch.size() == 2)
						{
							cachedModel = Utils::trim(modelMatch[1].str());
						}
					}
				}
			}

			cpuInfo.close();
		}

		// If we never found one, provide a reasonable default
		if (cachedModel.empty()) { cachedModel = "Gooberfest Cyclemaster 3000 (v8)"; }
		if (cachedCount == 0) { cachedCount = 42; }
	}

	cpuCount = cachedCount;
	return cachedModel;
}

// Build a variant that meets the standard for the Current Time (0x2A2B) Bluetooth Characteristic standard
//
// See: https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.current_time.xml
GVariant *ServerUtils::gvariantCurrentTime()
{
	time_t timeValue = time(nullptr);
	struct tm *pTimeStruct = localtime(&timeValue);
	guint16 year = pTimeStruct->tm_year + 1900;
	guint8 wday = guint8(pTimeStruct->tm_wday == 0 ? 7 : pTimeStruct->tm_wday);

	g_auto(GVariantBuilder) builder;
	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

	g_variant_builder_add(&builder, "y", (year >> 0) & 0xff);
	g_variant_builder_add(&builder, "y", (year >> 8) & 0xff);
	g_variant_builder_add(&builder, "y", guint8(pTimeStruct->tm_mon+1));  // month (1-12)
	g_variant_builder_add(&builder, "y", guint8(pTimeStruct->tm_mday));   // day (1-31)
	g_variant_builder_add(&builder, "y", guint8(pTimeStruct->tm_hour));   // hour (0-23)
	g_variant_builder_add(&builder, "y", guint8(pTimeStruct->tm_min));    // minute (0-59)
	g_variant_builder_add(&builder, "y", guint8(pTimeStruct->tm_sec));    // seconds (0-59)
	g_variant_builder_add(&builder, "y", wday);                           // weekday (1-7 where 1=Monday)
	g_variant_builder_add(&builder, "y", guint8(0));                      // Fractions (1/256th of second)
	g_variant_builder_add(&builder, "y", guint8(0));                      // Adjust reason bitmask (0 for testing)

	GVariant * const pVariant = g_variant_builder_end(&builder);
	return pVariant;
}

// Build a variant that meets the standard for the Local Time Information (0x2A0F) Bluetooth Characteristic standard
//
// See: https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.local_time_information.xml
GVariant *ServerUtils::gvariantLocalTime()
{
	tzset();
	time_t timeValue = time(nullptr);
	struct tm *pTimeStruct = localtime(&timeValue);

	gint8 utcOffset = -gint8(timezone / 60 / 15); // UTC time (uses 15-minute increments, 0 = UTC time)
	guint8 dstOffset = pTimeStruct->tm_isdst == 0 ? 0 : 4;  // 0 = no DST offset, 4 = +1 hour for DST

	g_auto(GVariantBuilder) builder;
	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

	g_variant_builder_add(&builder, "y", utcOffset);
	g_variant_builder_add(&builder, "y", dstOffset);

	GVariant * const pVariant = g_variant_builder_end(&builder);
	return pVariant;
}

}; // namespace ggk
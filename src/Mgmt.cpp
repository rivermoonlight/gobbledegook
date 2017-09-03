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
// This file contains various functions for interacting with Bluetooth Management interface, which provides adapter configuration.
//
// >>
// >>>  DISCUSSION
// >>
//
// We only cover the basics here. If there are configuration features you need that aren't supported (such as configuring BR/EDR),
// then this would be a good place for them.
//
// Note that this class relies on the `HciAdapter`, which is a very primitive implementation. Use with caution.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <string.h>

#include "Mgmt.h"
#include "Logger.h"
#include "Utils.h"

namespace ggk {

// Construct the Mgmt device
//
// Set `controllerIndex` to the zero-based index of the device as recognized by the OS. If this parameter is omitted, the index
// of the first device (0) will be used.
Mgmt::Mgmt(uint16_t controllerIndex)
: controllerIndex(controllerIndex)
{
}

// Returns the version information:
//
//    bits 0-15 = revision
//    bits 16-23 = version
//
//    ... or -1 on error
int Mgmt::getVersion()
{
	struct SResponse : HciAdapter::ResponseEvent
	{
		uint8_t version;
		uint16_t revision;

		void toHost()
		{
			revision = Utils::endianToHost(revision);
		}
	} __attribute__((packed));

	HciAdapter::Header request;
	request.code = 1;
	request.controllerId = kNonController;
	request.dataSize = 0;

	SResponse response;
	if (!hciAdapter.sendCommand(request, response, sizeof(response)))
	{
		Logger::warn(SSTR << "  + Failed to get version information");
		return -1;
	}

	response.toHost();

	Logger::debug(SSTR << "  + Version response has version=" << Utils::hex(response.version) << " and revision=" << Utils::hex(response.revision));
	return (response.version << 16) | response.revision;
}

// Returns information about the controller (address, name, current settings, etc.)
//
// See the definition for MgmtControllerInformation for details.
//
//    ... or nullptr on error
Mgmt::ControllerInformation *Mgmt::getControllerInformation()
{
	struct SResponse : HciAdapter::ResponseEvent
	{
		ControllerInformation info;

		void toHost()
		{
			info.toHost();
		}
	} __attribute__((packed));

	HciAdapter::Header request;
	request.code = 4; // Controller Information Command
	request.controllerId = controllerIndex;
	request.dataSize = 0;

	Logger::debug("Dumping device information after configuration...");

	SResponse response;
	if (!hciAdapter.sendCommand(request, response, sizeof(response)))
	{
		Logger::warn(SSTR << "  + Failed to get current settings");
		return nullptr;
	}

	response.toHost();

	// Copy it to our local
	controllerInfo = response.info;

	Logger::debug("  + Controller information");
	Logger::debug(SSTR << "    + Current settings   : " << Utils::hex(controllerInfo.currentSettings));
	Logger::debug(SSTR << "    + Address            : " << Utils::bluetoothAddressString(controllerInfo.address));
	Logger::debug(SSTR << "    + BT Version         : " << controllerInfo.bluetoothVersion);
	Logger::debug(SSTR << "    + Manufacturer       : " << Utils::hex(controllerInfo.manufacturer));
	Logger::debug(SSTR << "    + Supported settings : " << controllerSettingsString(controllerInfo.supportedSettings));
	Logger::debug(SSTR << "    + Current settings   : " << controllerSettingsString(controllerInfo.currentSettings));
	Logger::debug(SSTR << "    + Name               : " << controllerInfo.name);
	Logger::debug(SSTR << "    + Short name         : " << controllerInfo.shortName);

	return &controllerInfo;
}

// Set the adapter name and short name
//
// The inputs `name` and `shortName` may be truncated prior to setting them on the adapter. To ensure that `name` and
// `shortName` conform to length specifications prior to calling this method, see the constants `kMaxAdvertisingNameLength` and
// `kMaxAdvertisingShortNameLength`. In addition, the static methods `truncateName()` and `truncateShortName()` may be helpful.
//
// Returns true on success, otherwise false
bool Mgmt::setName(std::string name, std::string shortName)
{
	// Ensure their lengths are okay
	name = truncateName(name);
	shortName = truncateShortName(shortName);

	struct SRequest : HciAdapter::Header
	{
		char name[249];
		char shortName[11];
	} __attribute__((packed));

	struct SResponse : HciAdapter::ResponseEvent
	{
		char name[249];
		char shortName[11];
	} __attribute__((packed));

	SRequest request;
	request.code = 0x000F;
	request.controllerId = controllerIndex;
	request.dataSize = sizeof(SRequest) - sizeof(HciAdapter::Header);

	memset(request.name, 0, sizeof(request.name));
	snprintf(request.name, sizeof(request.name), "%s", name.c_str());

	memset(request.shortName, 0, sizeof(request.shortName));
	snprintf(request.shortName, sizeof(request.shortName), "%s", shortName.c_str());

	SResponse response;
	if (!hciAdapter.sendCommand(request, response, sizeof(response)))
	{
		Logger::warn(SSTR << "  + Failed to set name");
		return false;
	}

	Logger::info(SSTR << "  + Name set to '" << request.name << "', short name set to '" << request.shortName << "'");
	return true;
}

// Set a setting state to 'newState'
//
// Many settings are set the same way, this is just a convenience routine to handle them all
//
// Returns true on success, otherwise false
bool Mgmt::setState(const char *pSettingName, uint16_t commandCode, uint16_t controllerId, uint8_t newState)
{
	struct SRequest : HciAdapter::Header
	{
		uint8_t state;
	} __attribute__((packed));

	struct SResponse : HciAdapter::ResponseEvent
	{
		uint32_t currentSettings;

		void toHost()
		{
			currentSettings = Utils::endianToHost(currentSettings);
		}
	} __attribute__((packed));

	SRequest request;
	request.code = commandCode;
	request.controllerId = controllerId;
	request.dataSize = sizeof(SRequest) - sizeof(HciAdapter::Header);
	request.state = newState;

	SResponse response;
	if (!hciAdapter.sendCommand(request, response, sizeof(response)))
	{
		Logger::warn(SSTR << "  + Failed to set " << pSettingName << " state to: " << static_cast<int>(newState));
		return false;
	}

	response.toHost();

	Logger::debug(SSTR << "  + " << pSettingName << " set to " << static_cast<int>(newState) << ": " << controllerSettingsString(response.currentSettings));
	return true;
}

// Set the powered state to `newState` (true = powered on, false = powered off)
//
// Returns true on success, otherwise false
bool Mgmt::setPowered(bool newState)
{
	return setState("Powered", 0x0005, controllerIndex, newState ? 1 : 0);
}

// Set the BR/EDR state to `newState` (true = enabled, false = disabled)
//
// Returns true on success, otherwise false
bool Mgmt::setBredr(bool newState)
{
	return setState("BR/EDR", 0x002A, controllerIndex, newState ? 1 : 0);
}

// Set the Secure Connection state (0 = disabled, 1 = enabled, 2 = secure connections only mode)
//
// Returns true on success, otherwise false
bool Mgmt::setSecureConnections(uint8_t newState)
{
	return setState("SecureConnections", 0x002D, controllerIndex, newState);
}

// Set the bondable state to `newState` (true = enabled, false = disabled)
//
// Returns true on success, otherwise false
bool Mgmt::setBondable(bool newState)
{
	return setState("SecureConnections", 0x0009, controllerIndex, newState ? 1 : 0);
}

// Set the connectable state to `newState` (true = enabled, false = disabled)
//
// Returns true on success, otherwise false
bool Mgmt::setConnectable(bool newState)
{
	return setState("Connectable", 0x0007, controllerIndex, newState ? 1 : 0);
}

// Set the LE state to `newState` (true = enabled, false = disabled)
//
// Returns true on success, otherwise false
bool Mgmt::setLE(bool newState)
{
	return setState("LowEnergy", 0x000D, controllerIndex, newState ? 1 : 0);
}

// Set the advertising state to `newState` (0 = disabled, 1 = enabled (with consideration towards the connectable setting),
// 2 = enabled in connectable mode).
//
// Returns true on success, otherwise false
bool Mgmt::setAdvertising(uint8_t newState)
{
	return setState("Advertising", 0x0029, controllerIndex, newState);
}

// ---------------------------------------------------------------------------------------------------------------------------------
// Utilitarian
// ---------------------------------------------------------------------------------------------------------------------------------

// Transforms a "Current_Settings" value (4 octets as defined by the Bluetooth Management API specification) into a human-readable
// string of flags and settings.
std::string Mgmt::controllerSettingsString(uint32_t bits)
{
	std::string result = "";

	if ((bits & EHciPowered) != 0) { result += "Powered, "; }
	if ((bits & EHciConnectable) != 0) { result += "Connectable, "; }
	if ((bits & EHciFastConnectable) != 0) { result += "FC, "; }
	if ((bits & EHciDiscoverable) != 0) { result += "Discov, "; }
	if ((bits & EHciBondable) != 0) { result += "Bondable, "; }
	if ((bits & EHciLinkLevelSecurity) != 0) { result += "LLS, "; }
	if ((bits & EHciSecureSimplePairing) != 0) { result += "SSP, "; }
	if ((bits & EHciBasicRate_EnhancedDataRate) != 0) { result += "BR/EDR, "; }
	if ((bits & EHciHighSpeed) != 0) { result += "HS, "; }
	if ((bits & EHciLowEnergy) != 0) { result += "LE, "; }
	if ((bits & EHciAdvertising) != 0) { result += "Adv, "; }
	if ((bits & EHciSecureConnections) != 0) { result += "SC, "; }
	if ((bits & EHciDebugKeys) != 0) { result += "DebugKeys, "; }
	if ((bits & EHciPrivacy) != 0) { result += "Privacy, "; }
	if ((bits & EHciControllerConfiguration) != 0) { result += "ControllerConfig, "; }
	if ((bits & EHciStaticAddress) != 0) { result += "StaticAddr, "; }

	if (result.length() != 0)
	{
		result = result.substr(0, result.length() - 2);
	}

	return result;
}

// Truncates the string `name` to the maximum allowed length for an adapter name. If `name` needs no truncation, a copy of
// `name` is returned.
std::string Mgmt::truncateName(const std::string &name)
{
	if (name.length() <= kMaxAdvertisingNameLength)
	{
		return name;
	}

	return name.substr(0, kMaxAdvertisingNameLength);
}

// Truncates the string `name` to the maximum allowed length for an adapter short-name. If `name` needs no truncation, a copy
// of `name` is returned.
std::string Mgmt::truncateShortName(const std::string &name)
{
	if (name.length() <= kMaxAdvertisingShortNameLength)
	{
		return name;
	}

	return name.substr(0, kMaxAdvertisingShortNameLength);
}

}; // namespace ggk
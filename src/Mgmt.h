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

#pragma once

#include <stdint.h>
#include <string>

#include "HciAdapter.h"
#include "Utils.h"

namespace ggk {

struct Mgmt
{
	//
	// Constants
	//

	// The length of the controller's name (not including null terminator)
	static const int kMaxAdvertisingNameLength = 248;

	// The length of the controller's short name (not including null terminator)
	static const int kMaxAdvertisingShortNameLength = 10;

	//
	// Types
	//

	// HCI Controller Settings
	enum EHciControllerSettings
	{
		EHciPowered = (1<<0),
		EHciConnectable = (1<<1),
		EHciFastConnectable = (1<<2),
		EHciDiscoverable = (1<<3),
		EHciBondable = (1<<4),
		EHciLinkLevelSecurity = (1<<5),
		EHciSecureSimplePairing = (1<<6),
		EHciBasicRate_EnhancedDataRate = (1<<7),
		EHciHighSpeed = (1<<8),
		EHciLowEnergy = (1<<9),
		EHciAdvertising = (1<<10),
		EHciSecureConnections = (1<<11),
		EHciDebugKeys = (1<<12),
		EHciPrivacy = (1<<13),
		EHciControllerConfiguration = (1<<14),
		EHciStaticAddress = (1<<15)
	};

	// The comments documenting these fields are very high level. There is a lot of detailed information not present, for example
	// some values are not available at all times. This is fully documented in:
	//
	//     https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/mgmt-api.txt
	struct ControllerInformation
	{
		uint8_t address[6];         // The Bluetooth address
		uint8_t bluetoothVersion;   // Bluetooth version
		uint16_t manufacturer;      // The manufacturer
		uint32_t supportedSettings; // Bits for various supported settings (see EHciControllerSettings)
		uint32_t currentSettings;   // Bits for various currently configured settings (see EHciControllerSettings)
		uint8_t classOfDevice[3];   // Um, yeah. That.
		char name[249];             // Null terminated name
		char shortName[11];         // Null terminated short name

		void toHost()
		{
			manufacturer = Utils::endianToHost(manufacturer);
			supportedSettings = Utils::endianToHost(supportedSettings);
			currentSettings = Utils::endianToHost(currentSettings);
		}
	} __attribute__((packed));

	// Construct the Mgmt device
	//
	// Set `controllerIndex` to the zero-based index of the device as recognized by the OS. If this parameter is omitted, the index
	// of the first device (0) will be used.
	Mgmt(uint16_t controllerIndex = kDefaultControllerIndex);

	// Returns the version information:
	//
	//    bits 0-15 = revision
	//    bits 16-23 = version
	//
	//    ... or -1 on error
	int getVersion();

	// Returns information about the controller (address, name, current settings, etc.)
	//
	// See the definition for MgmtControllerInformation for details.
	//
	//    ... or nullptr on error
	ControllerInformation *getControllerInformation();

	// Set the adapter name and short name
	//
	// The inputs `name` and `shortName` may be truncated prior to setting them on the adapter. To ensure that `name` and
	// `shortName` conform to length specifications prior to calling this method, see the constants `kMaxAdvertisingNameLength` and
	// `kMaxAdvertisingShortNameLength`. In addition, the static methods `truncateName()` and `truncateShortName()` may be helpful.
	//
	// Returns true on success, otherwise false
	bool setName(std::string name, std::string shortName);

	// Set a setting state to 'newState'
	//
	// Many settings are set the same way, this is just a convenience routine to handle them all
	//
	// Returns true on success, otherwise false
	bool setState(const char *pSettingName, uint16_t commandCode, uint16_t controllerId, uint8_t newState);

	// Set the powered state to `newState` (true = powered on, false = powered off)
	//
	// Returns true on success, otherwise false
	bool setPowered(bool newState);

	// Set the BR/EDR state to `newState` (true = enabled, false = disabled)
	//
	// Returns true on success, otherwise false
	bool setBredr(bool newState);

	// Set the Secure Connection state (0 = disabled, 1 = enabled, 2 = secure connections only mode)
	//
	// Returns true on success, otherwise false
	bool setSecureConnections(uint8_t newState);

	// Set the bondable state to `newState` (true = enabled, false = disabled)
	//
	// Returns true on success, otherwise false
	bool setBondable(bool newState);

	// Set the connectable state to `newState` (true = enabled, false = disabled)
	//
	// Returns true on success, otherwise false
	bool setConnectable(bool newState);

	// Set the LE state to `newState` (true = enabled, false = disabled)
	//
	// Returns true on success, otherwise false
	bool setLE(bool newState);

	// Set the advertising state to `newState` (0 = disabled, 1 = enabled (with consideration towards the connectable setting),
	// 2 = enabled in connectable mode).
	//
	// Returns true on success, otherwise false
	bool setAdvertising(uint8_t newState);

	//
	// Utilitarian
	//

	// Transforms a "Current_Settings" value (4 octets as defined by the Bluetooth Management API specification) into a human-
	// readable string of flags and settings.
	static std::string controllerSettingsString(uint32_t bits);

	// Truncates the string `name` to the maximum allowed length for an adapter name. If `name` needs no truncation, a copy of
	// `name` is returned.
	static std::string truncateName(const std::string &name);

	// Truncates the string `name` to the maximum allowed length for an adapter short-name. If `name` needs no truncation, a copy
	// of `name` is returned.
	static std::string truncateShortName(const std::string &name);

private:

	//
	// Data members
	//

	// Our adapter - we use this to connect to the HCI socket to talk to the adapter
	HciAdapter hciAdapter;

	// Our controller information, updated each time the controller information is received
	ControllerInformation controllerInfo;

	// The default controller index (the first device)
	uint16_t controllerIndex;

	// Default controller index
	static const uint16_t kDefaultControllerIndex = 0;

	// A constant referring to a 'non-controller' (for commands that do not require a controller index)
	static const uint16_t kNonController = 0xffff;
};

}; // namespace ggk
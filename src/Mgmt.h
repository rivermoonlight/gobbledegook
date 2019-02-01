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

	// These indices should match those in HciAdapter::kEventTypeNames
	enum EventTypes
	{
		EInvalidEvent                                         = 0x0000,
		ECommandCompleteEvent                                 = 0x0001,
		ECommandStatusEvent                                   = 0x0002,
		EControllerErrorEvent                                 = 0x0003,
		EIndexAddedEvent                                      = 0x0004,
		EIndexRemovedEvent                                    = 0x0005,
		ENewSettingsEvent                                     = 0x0006,
		EClassOfDeviceChangedEvent                            = 0x0007,
		ELocalNameChangedEvent                                = 0x0008,
		ENewLinkKeyEvent                                      = 0x0009,
		ENewLongTermKeyEvent                                  = 0x000A,
		EDeviceConnectedEvent                                 = 0x000B,
		EDeviceDisconnectedEvent                              = 0x000C,
		EConnectFailedEvent                                   = 0x000D,
		EPINCodeRequestEvent                                  = 0x000E,
		EUserConfirmationRequestEvent                         = 0x000F,
		EUserPasskeyRequestEvent                              = 0x0010,
		EAuthenticationFailedEvent                            = 0x0011,
		EDeviceFoundEvent                                     = 0x0012,
		EDiscoveringEvent                                     = 0x0013,
		EDeviceBlockedEvent                                   = 0x0014,
		EDeviceUnblockedEvent                                 = 0x0015,
		EDeviceUnpairedEvent                                  = 0x0016,
		EPasskeyNotifyEvent                                   = 0x0017,
		ENewIdentityResolvingKeyEvent                         = 0x0018,
		ENewSignatureResolvingKeyEvent                        = 0x0019,
		EDeviceAddedEvent                                     = 0x001a,
		EDeviceRemovedEvent                                   = 0x001b,
		ENewConnectionParameterEvent                          = 0x001c,
		EUnconfiguredIndexAddedEvent                          = 0x001d,
		EUnconfiguredIndexRemovedEvent                        = 0x001e,
		ENewConfigurationOptionsEvent                         = 0x001f,
		EExtendedIndexAddedEvent                              = 0x0020,
		EExtendedIndexRemovedEvent                            = 0x0021,
		ELocalOutOfBandExtendedDataUpdatedEvent               = 0x0022,
		EAdvertisingAddedEvent                                = 0x0023,
		EAdvertisingRemovedEvent                              = 0x0024,
		EExtendedControllerInformationChangedEvent            = 0x0025
	};

	// These indices should match those in HciAdapter::kCommandCodeNames
	enum CommandCodes
	{
		EInvalidCommand                                       = 0x0000,
		EReadVersionInformationCommand                        = 0x0001,
		EReadSupportedCommandsCommand                         = 0x0002,
		EReadControllerIndexListCommand                       = 0x0003,
		EReadControllerInformationCommand                     = 0x0004,
		ESetPoweredCommand                                    = 0x0005,
		ESetDiscoverableCommand                               = 0x0006,
		ESetConnectableCommand                                = 0x0007,
		ESetFastConnectableCommand                            = 0x0008,
		ESetBondableCommand                                   = 0x0009,
		ESetLinkSecurityCommand                               = 0x000A,
		ESetSecureSimplePairingCommand                        = 0x000B,
		ESetHighSpeedCommand                                  = 0x000C,
		ESetLowEnergyCommand                                  = 0x000D,
		ESetDeviceClass                                       = 0x000E,
		ESetLocalNameCommand                                  = 0x000F,
		EAddUUIDCommand                                       = 0x0010,
		ERemoveUUIDCommand                                    = 0x0011,
		ELoadLinkKeysCommand                                  = 0x0012,
		ELoadLongTermKeysCommand                              = 0x0013,
		EDisconnectCommand                                    = 0x0014,
		EGetConnectionsCommand                                = 0x0015,
		EPINCodeReplyCommand                                  = 0x0016,
		EPINCodeNegativeReplyCommand                          = 0x0017,
		ESetIOCapabilityCommand                               = 0x0018,
		EPairDeviceCommand                                    = 0x0019,
		ECancelPairDeviceCommand                              = 0x001A,
		EUnpairDeviceCommand                                  = 0x001B,
		EUserConfirmationReplyCommand                         = 0x001C,
		EUserConfirmationNegativeReplyCommand                 = 0x001D,
		EUserPasskeyReplyCommand                              = 0x001E,
		EUserPasskeyNegativeReplyCommand                      = 0x001F,
		EReadLocalOutOfBandDataCommand                        = 0x0020,
		EAddRemoteOutOfBandDataCommand                        = 0x0021,
		ERemoveRemoteOutOfBandDataCommand                     = 0x0022,
		EStartDiscoveryCommand                                = 0x0023,
		EStopDiscoveryCommand                                 = 0x0024,
		EConfirmNameCommand                                   = 0x0025,
		EBlockDeviceCommand                                   = 0x0026,
		EUnblockDeviceCommand                                 = 0x0027,
		ESetDeviceIDCommand                                   = 0x0028,
		ESetAdvertisingCommand                                = 0x0029,
		ESetBREDRCommand                                      = 0x002A,
		ESetStaticAddressCommand                              = 0x002B,
		ESetScanParametersCommand                             = 0x002C,
		ESetSecureConnectionsCommand                          = 0x002D,
		ESetDebugKeysCommand                                  = 0x002E,
		ESetPrivacyCommand                                    = 0x002F,
		ELoadIdentityResolvingKeysCommand                     = 0x0030,
		EGetConnectionInformationCommand                      = 0x0031,
		EGetClockInformationCommand                           = 0x0032,
		EAddDeviceCommand                                     = 0x0033,
		ERemoveDeviceCommand                                  = 0x0034,
		ELoadConnectionParametersCommand                      = 0x0035,
		EReadUnconfiguredControllerIndexListCommand           = 0x0036,
		EReadControllerConfigurationInformationCommand        = 0x0037,
		ESetExternalConfigurationCommand                      = 0x0038,
		ESetPublicAddressCommand                              = 0x0039,
		EStartServiceDiscoveryCommand                         = 0x003a,
		EReadLocalOutOfBandExtendedDataCommand                = 0x003b,
		EReadExtendedControllerIndexListCommand               = 0x003c,
		EReadAdvertisingFeaturesCommand                       = 0x003d,
		EAddAdvertisingCommand                                = 0x003e,
		ERemoveAdvertisingCommand                             = 0x003f,
		EGetAdvertisingSizeInformationCommand                 = 0x0040,
		EStartLimitedDiscoveryCommand                         = 0x0041,
		EReadExtendedControllerInformationCommand             = 0x0042,
		ESetAppearanceCommand                                 = 0x0043
	};

	// Construct the Mgmt device
	//
	// Set `controllerIndex` to the zero-based index of the device as recognized by the OS. If this parameter is omitted, the index
	// of the first device (0) will be used.
	Mgmt(uint16_t controllerIndex = kDefaultControllerIndex);

	// Set the adapter name and short name
	//
	// The inputs `name` and `shortName` may be truncated prior to setting them on the adapter. To ensure that `name` and
	// `shortName` conform to length specifications prior to calling this method, see the constants `kMaxAdvertisingNameLength` and
	// `kMaxAdvertisingShortNameLength`. In addition, the static methods `truncateName()` and `truncateShortName()` may be helpful.
	//
	// Returns true on success, otherwise false
	bool setName(std::string name, std::string shortName);

	// Sets discoverable mode
	// 0x00 disables discoverable
	// 0x01 enables general discoverable
	// 0x02 enables limited discoverable
	// Timeout is the time in seconds. For 0x02, the timeout value is required.
	bool setDiscoverable(uint8_t disc, uint16_t timeout);

	// Set a setting state to 'newState'
	//
	// Many settings are set the same way, this is just a convenience routine to handle them all
	//
	// Returns true on success, otherwise false
	bool setState(uint16_t commandCode, uint16_t controllerId, uint8_t newState);

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

	// The default controller index (the first device)
	uint16_t controllerIndex;

	// Default controller index
	static const uint16_t kDefaultControllerIndex = 0;
};

}; // namespace ggk

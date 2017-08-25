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
// Protocol-level code for the Bluetooth Management API, which is used to configure the Bluetooth adapter
//
// >>
// >>>  DISCUSSION
// >>
//
// This class is intended for use by `Mgmt` (see Mgmt.cpp).
//
// The Bluetooth Management API, is used to configure the Bluetooth adapter (such as enabling LE, setting the device name, etc.)
// This class uses HciSocket (HciSocket.h) for the raw communications.
//
// The information for this implementation (as well as HciSocket.h) came from:
//
//     https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/mgmt-api.txt
//
// KNOWN LIMITATIONS:
//
// This is far from a complete implementation. I'm not even sure how reliable of an implementation this is. However, I can say with
// _complete_confidence_ that it works on my machine after numerous minutes of testing.
//
// One notable limitation is that this code doesn't work with the Bluetooth Management API in the way it was intended. The Bluetooth
// Management API treats the sending and receiving of data differently. It receives commands on the HCI socket and acts upon them.
// It also sends events on the same socket. It is important to note that there is not necessarily a 1:1 correlation from commands
// received to events generated. For example, an event can be generated when a bluetooth client disconnects, even though no command
// was sent for which that event is associated.
//
// However, for initialization, it seems to be generally safe to treat them as "nearly 1:1". The solution below is to consume all
// events and look for the event that we're waiting on. This seems to work in my environment (Raspberry Pi) fairly well, but please
// do use this with caution.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <string.h>

#include "HciAdapter.h"
#include "HciSocket.h"
#include "Utils.h"
#include "Logger.h"

static const int kMinCommandCode = 0x0001;
static const int kMaxCommandCode = 0x0043;
static const char *kCommandCodeNames[kMaxCommandCode] =
{
	"Read Management Version Information Command", // 0x0001
	"Read Management Supported Commands Command", // 0x0002
	"Read Controller Index List Command", // 0x0003
	"Read Controller Information Command", // 0x0004
	"Set Powered Command", // 0x0005
	"Set Discoverable Command", // 0x0006
	"Set Connectable Command", // 0x0007
	"Set Fast Connectable Command", // 0x0008
	"Set Bondable Command", // 0x0009
	"Set Link Security Command", // 0x000A
	"Set Secure Simple Pairing Command", // 0x000B
	"Set High Speed Command", // 0x000C
	"Set Low Energy Command", // 0x000D
	"Set Device Class", // 0x000E
	"Set Local Name Command", // 0x000F
	"Add UUID Command", // 0x0010
	"Remove UUID Command", // 0x0011
	"Load Link Keys Command", // 0x0012
	"Load Long Term Keys Command", // 0x0013
	"Disconnect Command", // 0x0014
	"Get Connections Command", // 0x0015
	"PIN Code Reply Command", // 0x0016
	"PIN Code Negative Reply Command", // 0x0017
	"Set IO Capability Command", // 0x0018
	"Pair Device Command", // 0x0019
	"Cancel Pair Device Command", // 0x001A
	"Unpair Device Command", // 0x001B
	"User Confirmation Reply Command", // 0x001C
	"User Confirmation Negative Reply Command", // 0x001D
	"User Passkey Reply Command", // 0x001E
	"User Passkey Negative Reply Command", // 0x001F
	"Read Local Out Of Band Data Command", // 0x0020
	"Add Remote Out Of Band Data Command", // 0x0021
	"Remove Remote Out Of Band Data Command", // 0x0022
	"Start Discovery Command", // 0x0023
	"Stop Discovery Command", // 0x0024
	"Confirm Name Command", // 0x0025
	"Block Device Command", // 0x0026
	"Unblock Device Command", // 0x0027
	"Set Device ID Command", // 0x0028
	"Set Advertising Command", // 0x0029
	"Set BR/EDR Command", // 0x002A
	"Set Static Address Command", // 0x002B
	"Set Scan Parameters Command", // 0x002C
	"Set Secure Connections Command", // 0x002D
	"Set Debug Keys Command", // 0x002E
	"Set Privacy Command", // 0x002F
	"Load Identity Resolving Keys Command", // 0x0030
	"Get Connection Information Command", // 0x0031
	"Get Clock Information Command", // 0x0032
	"Add Device Command", // 0x0033
	"Remove Device Command", // 0x0034
	"Load Connection Parameters Command", // 0x0035
	"Read Unconfigured Controller Index List Command", // 0x0036
	"Read Controller Configuration Information Command", // 0x0037
	"Set External Configuration Command", // 0x0038
	"Set Public Address Command", // 0x0039
	"Start Service Discovery Command", // 0x003a
	"Read Local Out Of Band Extended Data Command", // 0x003b
	"Read Extended Controller Index List Command", // 0x003c
	"Read Advertising Features Command", // 0x003d
	"Add Advertising Command", // 0x003e
	"Remove Advertising Command", // 0x003f
	"Get Advertising Size Information Command", // 0x0040
	"Start Limited Discovery Command", // 0x0041
	"Read Extended Controller Information Command", // 0x0042
	// NOTE: The documentation at https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/mgmt-api.txt) states that the command
	// code for "Set Appearance Command" is 0x0042. It also says this about the previous command in the list ("Read Extended
	// Controller Information Command".) This is likely an error, so I'm following the order of the commands as they appear in the
	// documentation. This makes "Set Appearance Code" have a command code of 0x0043.
	"Set Appearance Command", // 0x0043
};

static const int kMinEventType = 0x0001;
static const int kMaxEventType = 0x0025;
static const char *kEventTypeNames[kMaxEventType] =
{
	"Command Complete Event", // 0x0001
	"Command Status Event", // 0x0002
	"Controller Error Event", // 0x0003
	"Index Added Event", // 0x0004
	"Index Removed Event", // 0x0005
	"New Settings Event", // 0x0006
	"Class Of Device Changed Event", // 0x0007
	"Local Name Changed Event", // 0x0008
	"New Link Key Event", // 0x0009
	"New Long Term Key Event", // 0x000A
	"Device Connected Event", // 0x000B
	"Device Disconnected Event", // 0x000C
	"Connect Failed Event", // 0x000D
	"PIN Code Request Event", // 0x000E
	"User Confirmation Request Event", // 0x000F
	"User Passkey Request Event", // 0x0010
	"Authentication Failed Event", // 0x0011
	"Device Found Event", // 0x0012
	"Discovering Event", // 0x0013
	"Device Blocked Event", // 0x0014
	"Device Unblocked Event", // 0x0015
	"Device Unpaired Event", // 0x0016
	"Passkey Notify Event", // 0x0017
	"New Identity Resolving Key Event", // 0x0018
	"New Signature Resolving Key Event", // 0x0019
	"Device Added Event", // 0x001a
	"Device Removed Event", // 0x001b
	"New Connection Parameter Event", // 0x001c
	"Unconfigured Index Added Event", // 0x001d
	"Unconfigured Index Removed Event", // 0x001e
	"New Configuration Options Event", // 0x001f
	"Extended Index Added Event", // 0x0020
	"Extended Index Removed Event", // 0x0021
	"Local Out Of Band Extended Data Updated Event", // 0x0022
	"Advertising Added Event", // 0x0023
	"Advertising Removed Event", // 0x0024
	"Extended Controller Information Changed Event" // 0x0025
};

// Connects the HCI socket if a connection does not already exist
//
// If a connection already exists, this method will do nothing and return true.
//
// Note that it shouldn't be necessary to connect manually; any action requiring a connection will automatically connect
//
// Returns true if the HCI socket is connected (either via a new connection or an existing one), otherwise false
bool HciAdapter::connect()
{
	// Connect if we aren't already connected
	if (!isConnected() && !hciSocket.connect())
	{
		disconnect();
		return false;
	}

	return true;
}

// Returns true if connected to the HCI socket, otherwise false
//
// Note that it shouldn't be necessary to connect manually; any action requiring a connection will automatically connect
bool HciAdapter::isConnected() const
{
	return hciSocket.isConnected();
}

// Disconnects from the HCI Socket
//
// If the connection is not connected, this method will do nothing.
//
// It isn't necessary to disconnect manually; the HCI socket will get disocnnected automatically upon destruction
void HciAdapter::disconnect()
{
	if (isConnected())
	{
		hciSocket.disconnect();
	}
}

// Sends a command over the HCI socket
//
// If the HCI socket is not connected, it will auto-connect prior to sending the command. In the case of a failed auto-connect,
// a failure is returned.
//
// Returns true on success, otherwise false
bool HciAdapter::sendCommand(Header &request, ResponseEvent &response, int responseLen)
{
	// Auto-connect
	if (!connect()) { return false; }

	Logger::debug("  + Request header");
	Logger::debug(SSTR << "    + Event code         : " << Utils::hex(request.code));
	Logger::debug(SSTR << "    + Controller Id      : " << Utils::hex(request.controllerId));
	Logger::debug(SSTR << "    + Data size          : " << request.dataSize << " bytes");

	request.toNetwork();
	uint8_t *pRequest = reinterpret_cast<uint8_t *>(&request);
	std::vector<uint8_t> requestPacket = std::vector<uint8_t>(pRequest, pRequest + sizeof(request) + request.dataSize);
	if (!hciSocket.write(requestPacket))
	{
		return false;
	}

	// Read the response for this particular command
	return readResponse(request.code, response, responseLen);
}

// Reads a response from the HCI socket
//
// Responses are generally triggered by sending commands (see `sendCommand`) but not always. In HCI parlance, a response is
// actually an event. Performing commands triggers events. There is not always a 1:1 ratio betwee command and event, and a
// command may trigger different events based on the result of the command.
//
// Unlike the other methods in this class, this method does not auto-connect, as this method is private and can only be called
// from methods that have alreay auto-connected.
bool HciAdapter::readResponse(uint16_t commandCode, ResponseEvent &response, size_t responseLen) const
{
	if (!hciSocket.isConnected()) { return false; }

	std::vector<uint8_t> responsePacket = std::vector<uint8_t>();
	if (!hciSocket.read(responsePacket))
	{
		return false;
	}

	// Validate our events and remove everything that isn't a command complete or status event
	if (!filterAndValidateEvents(commandCode, responsePacket))
	{
		return false;
	}

	// Do we have enough to check the event code?
	if (responsePacket.size() < 2)
	{
		Logger::error(SSTR << "Command's response was invalid");
		return false;
	}

	// If this wasn't a Command Complete Event, just return an error
	uint16_t eventCode = Utils::endianToHost(*reinterpret_cast<uint16_t *>(responsePacket.data()));
	if (eventCode != 1)
	{
		return false;
	}

	// It's a Command Complete event, verify the size is what we expect
	if (responsePacket.size() != responseLen)
	{
		Logger::error(SSTR << "Command's response was " << responsePacket.size() << " bytes but was supposed to be " << responseLen << " bytes");
		return false;
	}

	// Copy the response data into the requested structure
	memcpy(&response, responsePacket.data(), responseLen);

	// Perform endian conversion
	response.toHost();

	// Log the header information
	Logger::debug("  + Response event");
	Logger::debug(SSTR << "    + Event code         : " << Utils::hex(response.header.code));
	Logger::debug(SSTR << "    + Controller Id      : " << Utils::hex(response.header.controllerId));
	Logger::debug(SSTR << "    + Data size          : " << response.header.dataSize << " bytes");
	Logger::debug(SSTR << "    + Command code       : " << response.commandCode);
	Logger::debug(SSTR << "    + Status             : " << Utils::hex(response.status));

	// One last check... let's verify that our data size is what it should be
	//
	// Note that we add 3 - this is because there are two fields that always follow the
	// data size (so they were included in our ResponseEvent structure) even
	// though they are not counted as part of the size, so we account for that here.
	int adjustedHeaderSize = sizeof(ResponseEvent) - 3;
	int expectedDataSize = responseLen - adjustedHeaderSize;
	if (response.header.dataSize != expectedDataSize)
	{
		Logger::error(SSTR << "The data size from the response (" << response.header.dataSize
		<< ") does not match the response structure (" << expectedDataSize
		<< ": " << responseLen << " - " << adjustedHeaderSize
		<< ") - this is likely a kernel version mismatch or bug in the code");
	}

	// Everything checks out - good to go!
	return true;
}

// Filter out events that we aren't interested in
//
// NOTE: We're just dipping our toe into the HCI stuff here, so we only care about command complete and status events. This
// isn't the most robust way to do things, but it is effective.
bool HciAdapter::filterAndValidateEvents(uint16_t commandCode, std::vector<uint8_t> &buffer) const
{
	// Chew through each event in the buffer, removing those that are not related to the requested commandCode
	std::vector<uint8_t> lastGoodEvent;
	while (!buffer.empty())
	{
		// Get an event pointer into the start of our buffer
		ResponseEvent *pEvent = reinterpret_cast<ResponseEvent *>(buffer.data());

		// Validate that there is enough space for this event according to its size
		size_t dataLength = sizeof(Header) + pEvent->header.dataSize;
		if (dataLength > buffer.size())
		{
			Logger::error(SSTR << "  + Not enough data for the current event");
			return false;
		}

		// Check the event type
		if (pEvent->header.code < kMinEventType || pEvent->header.code > kMaxEventType)
		{
			Logger::error(SSTR << "  + Unknown event type " << Utils::hex(pEvent->header.code) << " - ditching all response data to resynchronize");
			return false;
		}

		std::string eventTypeName = kEventTypeNames[pEvent->header.code - kMinEventType];
		std::string commandCodeName = pEvent->commandCode < kMinCommandCode || pEvent->commandCode > kMaxCommandCode ? "Unknown" : kCommandCodeNames[pEvent->commandCode - kMinCommandCode];

		Logger::info(SSTR << "  + Received event type " << Utils::hex(pEvent->header.code) << " (" << eventTypeName << ")");

		// Success event for our command?
		if (pEvent->header.code != 1 && pEvent->commandCode != commandCode)
		{
			Logger::warn(SSTR << "  + Skipping event type " << Utils::hex(pEvent->header.code) << " (" << eventTypeName << ")");
		}
		else
		{
			lastGoodEvent = std::vector<uint8_t>(buffer.begin(), buffer.begin() + dataLength);
		}

		// Remove the current event and move along
		buffer.erase(buffer.begin(), buffer.begin() + dataLength);
	}

	// If we don't have a last good event, return an error
	if (lastGoodEvent.empty())
	{
		return false;
	}

	// Return the last good event
	buffer = lastGoodEvent;
	return true;
}

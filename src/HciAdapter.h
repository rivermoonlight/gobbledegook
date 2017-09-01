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
// See the discussion at the top of HciAdapter.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <stdint.h>
#include <vector>

#include "HciSocket.h"
#include "Utils.h"

namespace ggk {

class HciAdapter
{
public:

	struct Header
	{
		uint16_t code;
		uint16_t controllerId;
		uint16_t dataSize;

		void toNetwork()
		{
			code = Utils::endianToHci(code);
			controllerId = Utils::endianToHci(controllerId);
			dataSize = Utils::endianToHci(dataSize);
		}

		void toHost()
		{
			code = Utils::endianToHost(code);
			controllerId = Utils::endianToHost(controllerId);
			dataSize = Utils::endianToHost(dataSize);
		}

	} __attribute__((packed));

	struct ResponseEvent
	{
		Header header;
		uint16_t commandCode;
		uint8_t status;

		void toNetwork()
		{
			header.toNetwork();
			commandCode = Utils::endianToHci(commandCode);
		}

		void toHost()
		{
			header.toHost();
			commandCode = Utils::endianToHost(commandCode);
		}

	} __attribute__((packed));

	// Connects the HCI socket if a connection does not already exist
	//
	// If a connection already exists, this method will do nothing and return true.
	//
	// Note that it shouldn't be necessary to connect manually; any action requiring a connection will automatically connect
	//
	// Returns true if the HCI socket is connected (either via a new connection or an existing one), otherwise false
	bool connect();

	// Returns true if connected to the HCI socket, otherwise false
	//
	// Note that it shouldn't be necessary to connect manually; any action requiring a connection will automatically connect
	bool isConnected() const;

	// Disconnects from the HCI Socket
	//
	// If the connection is not connected, this method will do nothing.
	//
	// It isn't necessary to disconnect manually; the HCI socket will get disocnnected automatically upon destruction
	void disconnect();

	// Sends a command over the HCI socket
	//
	// If the HCI socket is not connected, it will auto-connect prior to sending the command. In the case of a failed auto-connect,
	// a failure is returned.
	//
	// Returns true on success, otherwise false
	bool sendCommand(Header &request, ResponseEvent &response, int responseLen);

private:

	// Reads a response from the HCI socket
	//
	// Responses are generally triggered by sending commands (see `sendCommand`) but not always. In HCI parlance, a response is
	// actually an event. Performing commands triggers events. There is not always a 1:1 ratio betwee command and event, and a
	// command may trigger different events based on the result of the command.
	//
	// Unlike the other methods in this class, this method does not auto-connect, as this method is private and can only be called
	// from methods that have alreay auto-connected.
	bool readResponse(uint16_t commandCode, ResponseEvent &response, size_t responseLen) const;

	// Filter out events that we aren't interested in
	//
	// NOTE: We're just dipping our toe into the HCI stuff here, so we only care about command complete and status events. This
	// isn't the most robust way to do things, but it is effective.
	bool filterAndValidateEvents(uint16_t commandCode, std::vector<uint8_t> &buffer) const;

	// Our HCI Socket, which allows us to talk directly to the kernel
	HciSocket hciSocket;
};

}; // namespace ggk
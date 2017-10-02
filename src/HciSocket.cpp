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
// Low-level socket interface used to communicate with the Bluetooth Management API (see HciAdapter.cpp)
//
// >>
// >>>  DISCUSSION
// >>
//
// This class intended to support `HciAdapter` - see HciAdapter.cpp for more information.
//
// This file contains raw communication layer for the Bluetooth Management API, which is used to configure the Bluetooth adapter
// (such as enabling LE, setting the device name, etc.) This class is used by HciAdapter (HciAdapter.h) to perform higher-level
// functions.
//
// This code is for example purposes only. If you plan to use this in a production environment, I suggest rewriting it.
//
// The information for this implementation (as well as HciAdapter.h) came from:
//
//     https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/mgmt-api.txt
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <thread>
#include <fcntl.h>

#include "HciSocket.h"
#include "Logger.h"
#include "Utils.h"

namespace ggk {

// Initializes an unconnected socket
HciSocket::HciSocket()
: fdSocket(-1)
{
}

// Socket destructor
//
// This will automatically disconnect the socket if it is currently connected
HciSocket::~HciSocket()
{
	disconnect();
}

// Connects to an HCI socket using the Bluetooth Management API protocol
//
// Returns true on success, otherwise false
bool HciSocket::connect()
{
	disconnect();

	fdSocket = socket(PF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK, BTPROTO_HCI);
	if (fdSocket < 0)
	{
		logErrno("Connect(socket)");
		return false;
	}

	struct sockaddr_hci addr;
	memset(&addr, 0, sizeof(addr));
	addr.hci_family = AF_BLUETOOTH;
	addr.hci_dev = HCI_DEV_NONE;
	addr.hci_channel = HCI_CHANNEL_CONTROL;

	if (bind(fdSocket, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
	{
		logErrno("Connect(bind)");
		disconnect();
		return false;
	}

	Logger::debug(SSTR << "Connected to HCI control socket (fd = " << fdSocket << ")");

	return true;
}

// Returns true if the socket is currently connected, otherwise false
bool HciSocket::isConnected() const
{
	return fdSocket >= 0;
}

// Disconnects from the HCI socket
void HciSocket::disconnect()
{
	if (isConnected())
	{
		Logger::debug("HciSocket disconnecting");

		if (close(fdSocket) != 0)
		{
			logErrno("close(fdSocket)");
		}

		fdSocket = -1;
		Logger::trace("HciSocket closed");
	}
}

// Reads data from the HCI socket
//
// Raw data is read and returned in `response`.
//
// Returns true if data was read successfully, otherwise false is returned. A false return code does not necessarily depict
// an error, as this can arise from expected conditions (such as an interrupt.)
bool HciSocket::read(std::vector<uint8_t> &response) const
{
	// Fill our response with empty data
	response.resize(kResponseMaxSize, 0);

	// Wait for data or a cancellation
	if (!waitForDataOrShutdown())
	{
		return false;
	}

	// Block until we receive data, a disconnect, or a signal
	ssize_t bytesRead = ::recv(fdSocket, &response[0], kResponseMaxSize, MSG_WAITALL);

	// If there was an error, wipe the data and return an error condition
	if (bytesRead < 0)
	{
		if (errno == EINTR)
		{
			Logger::debug("HciSocket receive interrupted");
		}
		else
		{
			logErrno("recv");
		}
		response.resize(0);
		return false;
	}
	else if (bytesRead == 0)
	{
		Logger::error("Peer closed the socket");
		response.resize(0);
		return false;
	}

	// We have data
	response.resize(bytesRead);

	std::string dump = "";
	dump += "  > Read " + std::to_string(response.size()) + " bytes\n";
	dump += Utils::hex(response.data(), response.size());
	Logger::debug(dump);

	return true;
}

// Writes the array of bytes of a given count
//
// This method returns true if the bytes were written successfully, otherwise false
bool HciSocket::write(std::vector<uint8_t> buffer) const
{
	return write(buffer.data(), buffer.size());
}

// Writes the array of bytes of a given count
//
// This method returns true if the bytes were written successfully, otherwise false
bool HciSocket::write(const uint8_t *pBuffer, size_t count) const
{
	std::string dump = "";
	dump += "  > Writing " + std::to_string(count) + " bytes\n";
	dump += Utils::hex(pBuffer, count);
	Logger::debug(dump);

	size_t len = ::write(fdSocket, pBuffer, count);

	if (len != count)
	{
		logErrno("write");
		return false;
	}

	return true;
}

// Wait for data to arrive, or for a shutdown event
//
// Returns true if data is available, false if we are shutting down
bool HciSocket::waitForDataOrShutdown() const
{
	while(ggkIsServerRunning())
	{
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(fdSocket, &rfds);

		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = kDataWaitTimeMS * 1000;

		int retval = select(fdSocket+1, &rfds, NULL, NULL, &tv);

		// Do we have data?
		if (retval > 0) { return true; }

		// We have an error
		if (retval < 0) { return false; }

		// No data; keep waiting
		continue;
	}

	return false;
}

// Utilitarian function for logging errors for the given operation
void HciSocket::logErrno(const char *pOperation) const
{
	std::string errorDetail(strerror(errno));
	if (errno == EAGAIN)
	{
		errorDetail += " or not enough permission for this operation";
	}

	Logger::error(SSTR << "Error on Bluetooth management socket during " << pOperation << " operation. Error code " << errno << ": " << errorDetail);
}

}; // namespace ggk

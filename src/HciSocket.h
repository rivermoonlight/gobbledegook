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
// Low-level socket interface used to communicate with the Bluetooth Management API (see HciAdapter.cpp)
//
// >>
// >>>  DISCUSSION
// >>
//
// See the discussion at the top of HciSocket.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <unistd.h>
#include <stdint.h>
#include <vector>

namespace ggk {

class HciSocket
{
public:
	// Initializes an unconnected socket
	HciSocket();

	// Socket destructor
	//
	// This will automatically disconnect the socket if it is currently connected
	~HciSocket();

	// Connects to an HCI socket using the Bluetooth Management API protocol
	//
	// Returns true on success, otherwise false
	bool connect();

	// Returns true if the socket is currently connected, otherwise false
	bool isConnected() const;

	// Disconnects from the HCI socket
	void disconnect();

	// Reads data from the HCI socket
	//
	// Raw data is read until no more data is available. If no data is available when this method initially starts to read, it will
	// retry for a maximum timeout defined by `kMaxRetryTimeMS`.
	//
	// Returns true if any data was read successfully, otherwise false is returned in the case of an error or a timeout.
	bool read(std::vector<uint8_t> &response) const;

	// Writes the array of bytes of a given count
	//
	// This method returns true if the bytes were written successfully, otherwise false
	bool write(std::vector<uint8_t> buffer) const;

	// Writes the array of bytes of a given count
	//
	// This method returns true if the bytes were written successfully, otherwise false
	bool write(const uint8_t *pBuffer, size_t count) const;

private:

	// Wait for data to arrive, or for a shutdown event
	//
	// Returns true if data is available, false if we are shutting down
	bool waitForDataOrShutdown() const;

	// Utilitarian function for logging errors for the given operation
	void logErrno(const char *pOperation) const;

	int	fdSocket;

	const size_t kResponseMaxSize = 64 * 1024;
	const int kDataWaitTimeMS = 10;
};

}; // namespace ggk
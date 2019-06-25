// Copyright 2017-2019 Paul Nettle
//
// This file is part of Gobbledegook.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file in the root of the source tree.

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
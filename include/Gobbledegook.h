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
// This file represents the complete interface to Gobbledegook from a stand-alone application.
//
// >>
// >>>  DISCUSSION
// >>
//
// The interface to Gobbledegook is rether simple. It consists of the following categories of functionality:
//
//     * Logging
//
//       The server defers all logging to the application. The application registers a set of logging delegates (one for each
//       log level) so it can manage the logs however it wants (syslog, console, file, an external logging service, etc.)
//
//     * Managing updates to server data
//
//       The application is required to implement two delegates (`GGKServerDataGetter` and `GGKServerDataSetter`) for sharing data
//       with the server. See standalone.cpp for an example of how this is done.
//
//       In addition, the server provides a thread-safe queue for notifications of data updates to the server. Generally, the only
//       methods an application will need to call are `ggkNofifyUpdatedCharacteristic` and `ggkNofifyUpdatedDescriptor`. The other
//       methods are provided in case an application requies extended functionality.
//
//     * Server control
//
//       A small set of methods for starting and stopping the server.
//
//     * Server state
//
//       These routines allow the application to query the server's current state. The server runs through these states during its
//       lifecycle:
//
//           EUninitialized -> EInitializing -> ERunning -> EStopping -> EStopped
//
//     * Server health
//
//       The server maintains its own health information. The states are:
//
//           EOk         - the server is A-OK
//           EFailedInit - the server had a failure prior to the ERunning state
//           EFailedRun  - the server had a failure during the ERunning state
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

	// -----------------------------------------------------------------------------------------------------------------------------
	// LOGGING
	// -----------------------------------------------------------------------------------------------------------------------------

	// Type definition for callback delegates that receive log messages
	typedef void (*GGKLogReceiver)(const char *pMessage);

	// Each of these methods registers a log receiver method. Receivers are set when registered. To unregister a log receiver,
	// simply register with `nullptr`.
	void ggkLogRegisterDebug(GGKLogReceiver receiver);
	void ggkLogRegisterInfo(GGKLogReceiver receiver);
	void ggkLogRegisterStatus(GGKLogReceiver receiver);
	void ggkLogRegisterWarn(GGKLogReceiver receiver);
	void ggkLogRegisterError(GGKLogReceiver receiver);
	void ggkLogRegisterFatal(GGKLogReceiver receiver);
	void ggkLogRegisterAlways(GGKLogReceiver receiver);
	void ggkLogRegisterTrace(GGKLogReceiver receiver);

	// -----------------------------------------------------------------------------------------------------------------------------
	// SERVER DATA
	// -----------------------------------------------------------------------------------------------------------------------------

	// Type definition for a delegate that the server will use when it needs to receive data from the host application
	//
	// IMPORTANT:
	//
	// This will be called from the server's thread. Be careful to ensure your implementation is thread safe.
	//
	// Similarly, the pointer to data returned to the server should point to non-volatile memory so that the server can use it
	// safely for an indefinite period of time.
	typedef const void *(*GGKServerDataGetter)(const char *pName);

	// Type definition for a delegate that the server will use when it needs to notify the host application that data has changed
	//
	// IMPORTANT:
	//
	// This will be called from the server's thread. Be careful to ensure your implementation is thread safe.
	//
	// The data setter uses void* types to allow receipt of unknown data types from the server. Ensure that you do not store these
	// pointers. Copy the data before returning from your getter delegate.
	//
	// This method returns a non-zero value on success or 0 on failure.
	//
	// Possible failures:
	//
	//   * pName is null
	//   * pData is null
	//   * pName is not a supported value to store
	//   * Any other failure, as deemed by the delegate handler
	typedef int (*GGKServerDataSetter)(const char *pName, const void *pData);

	// -----------------------------------------------------------------------------------------------------------------------------
	// SERVER DATA UPDATE MANAGEMENT
	// -----------------------------------------------------------------------------------------------------------------------------

	// Adds an update to the front of the queue for a characteristic at the given object path
	//
	// Returns non-zero value on success or 0 on failure.
	int ggkNofifyUpdatedCharacteristic(const char *pObjectPath);

	// Adds an update to the front of the queue for a descriptor at the given object path
	//
	// Returns non-zero value on success or 0 on failure.
	int ggkNofifyUpdatedDescriptor(const char *pObjectPath);

	// Adds a named update to the front of the queue. Generally, this routine should not be used directly. Instead, use the
	// `ggkNofifyUpdatedCharacteristic()` instead.
	//
	// Returns non-zero value on success or 0 on failure.
	int ggkPushUpdateQueue(const char *pObjectPath, const char *pInterfaceName);

	// Get the next update from the back of the queue and returns the element in `element` as a string in the format:
	//
	//     "com/object/path|com.interface.name"
	//
	// If the queue is empty, this method returns `0` and does nothing.
	//
	// `elementLen` is the size of the `element` buffer in bytes. If the resulting string (including the null terminator) will not
	// fit within `elementLen` bytes, the method returns `-1` and does nothing.
	//
	// If `keep` is set to non-zero, the entry is not removed and will be retrieved again on the next call. Otherwise, the element
	// is removed.
	//
	// Returns 1 on success, 0 if the queue is empty, -1 on error (such as the length too small to store the element)
	int ggkPopUpdateQueue(char *pElement, int elementLen, int keep);

	// Returns 1 if the queue is empty, otherwise 0
	int ggkUpdateQueueIsEmpty();

	// Returns the number of entries waiting in the queue
	int ggkUpdateQueueSize();

	// Removes all entries from the queue
	void ggkUpdateQueueClear();

	// -----------------------------------------------------------------------------------------------------------------------------
	// SERVER CONTROL
	// -----------------------------------------------------------------------------------------------------------------------------

	// Set the server state to 'EInitializing' and then immediately create a server thread and initiate the server's async
	// processing on the server thread.
	//
	// At that point the current thread will block for maxAsyncInitTimeoutMS milliseconds or until initialization completes.
	//
	// If initialization was successful, the method will return a non-zero value with the server running on its own thread in
	// 'runServerThread'.
	//
	// If initialization was unsuccessful, this method will continue to block until the server has stopped. This method will then
	// return 0.
	//
	// IMPORTANT:
	//
	// The data setter uses void* types to allow receipt of unknown data types from the server. Ensure that you do not store these
	// pointers. Copy the data before returning from your getter delegate.
	//
	// Similarly, the pointer to data returned to the data getter should point to non-volatile memory so that the server can use it
	// safely for an indefinite period of time.
	//
	// serviceName: The name of our server (collectino of services)
	//
	//     !!!IMPORTANT!!!
	//
	//     This name must match tha name configured in the D-Bus permissions. See the Readme.md file for more information.
	//
	//     This is used to build the path for our Bluetooth services. It also provides the base for the D-Bus owned name (see
	//     getOwnedName.)
	//
	//     This value will be stored as lower-case only.
	//
	//     Retrieve this value using the `TheServer->getName()` method
	//
	// advertisingName: The name for this controller, as advertised over LE
	//
	//     IMPORTANT: Setting the advertisingName will change the system-wide name of the device. If that's not what you want, set
	//     BOTH advertisingName and advertisingShortName to as empty string ("") to prevent setting the advertising
	//     name.
	//
	//     Retrieve this value using the `getAdvertisingName()` method
	//
	// advertisingShortName: The short name for this controller, as advertised over LE
	//
	//     According to the spec, the short name is used in case the full name doesn't fit within Extended Inquiry Response (EIR) or
	//     Advertising Data (AD).
	//
	//     IMPORTANT: Setting the advertisingName will change the system-wide name of the device. If that's not what you want, set
	//     BOTH advertisingName and advertisingShortName to as empty string ("") to prevent setting the advertising
	//     name.
	//
	//     Retrieve this value using the `getAdvertisingShortName()` method
	//
	int ggkStart(const char *pServiceName, const char *pAdvertisingName, const char *pAdvertisingShortName, 
		GGKServerDataGetter getter, GGKServerDataSetter setter, int maxAsyncInitTimeoutMS);

	// Blocks for up to maxAsyncInitTimeoutMS milliseconds until the server shuts down.
	//
	// If shutdown is successful, this method will return a non-zero value. Otherwise, it will return 0.
	//
	// If the server fails to stop for some reason, the thread will be killed.
	//
	// Typically, a call to this method would follow `ggkTriggerShutdown()`.
	int ggkWait();

	// Tells the server to begin the shutdown process
	//
	// The shutdown process will interrupt any currently running asynchronous operation and prevent new operations from starting.
	// Once the server has stabilized, its event processing loop is terminated and the server is cleaned up.
	//
	// `ggkGetServerRunState` will return EStopped when shutdown is complete. To block until the shutdown is complete, see
	// `ggkWait()`.
	//
	// Alternatively, you can use `ggkShutdownAndWait()` to request the shutdown and block until the shutdown is complete.
	void ggkTriggerShutdown();

	// Convenience method to trigger a shutdown (via `ggkTriggerShutdown()`) and also waits for shutdown to complete (via
	// `ggkWait()`)
	int ggkShutdownAndWait();

	// -----------------------------------------------------------------------------------------------------------------------------
	// SERVER STATE
	// -----------------------------------------------------------------------------------------------------------------------------

	// Current state of the server
	//
	// States should progress through states in this order:
	//
	//     EUninitialized -> EInitializing -> ERunning -> EStopping -> EStopped
	//
	// Note that in some cases, a server may skip one or more states, as is the case of a failed initialization where the server
	// will progress from EInitializing directly to EStopped.
	//
	// Use `ggkGetServerRunState` to retrive the state and `ggkGetServerRunStateString` to convert a `GGKServerRunState` into a
	// human-readable string.
	enum GGKServerRunState
	{
		EUninitialized,
		EInitializing,
		ERunning,
		EStopping,
		EStopped
	};

	// Retrieve the current running state of the server
	//
	// See `GGKServerRunState` (enumeration) for more information.
	enum GGKServerRunState ggkGetServerRunState();

	// Convert a `GGKServerRunState` into a human-readable string
	const char *ggkGetServerRunStateString(enum GGKServerRunState state);

	// Convenience method to check ServerRunState for a running server
	int ggkIsServerRunning();

	// -----------------------------------------------------------------------------------------------------------------------------
	// SERVER HEALTH
	// -----------------------------------------------------------------------------------------------------------------------------

	// The current health of the server
	//
	// A running server's health will always be EOk, therefore it is only necessary to check the health status after the server
	// has shut down to determine if it was shut down due to an unhealthy condition.
	//
	// Use `ggkGetServerHealth` to retrieve the health and `ggkGetServerHealthString` to convert a `GGKServerHealth` into a
	// human-readable string.
	enum GGKServerHealth
	{
		EOk,
		EFailedInit,
		EFailedRun
	};

	// Retrieve the current health of the server
	//
	// See `GGKServerHealth` (enumeration) for more information.
	enum GGKServerHealth ggkGetServerHealth();

	// Convert a `GGKServerHealth` into a human-readable string
	const char *ggkGetServerHealthString(enum GGKServerHealth state);

#ifdef __cplusplus
}
#endif //__cplusplus

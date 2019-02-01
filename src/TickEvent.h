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
// A TIckEvent is an event that is triggered on a regular timer interval.
//
// >>
// >>>  DISCUSSION
// >>
//
// Tick events are used within the Server's description in Server.cpp. As you might expect, they are useful for updating data on a
// regular basis or performing other periodic tasks. One example usage might be checking the battery level every 60 seconds and if
// it has changed since the last update, send out a notification to subscribers.
//
// The frequency at which events fire is dependent upon two values: the driving timer's tick frequency multiplied by the tick
// freuency of the TickEvent itself.
//
// The tick event's frequency is set when a tick event is added via the `onEvent()` method to the server description.
//
// The driving timer's frequency is a one-second-resolution low-frequency timer with a default of one second. To modify this, see
// `kPeriodicTimerFrequencySeconds` at the top of Init.cpp. Note that the periodic timer (which drives tick events) is intentionally
// a low-frequency timer. Higher frequency timers would lend themselves to using more battery on both, the server and client.
//
// When using a TickEvent, be careful not to demand too much of your client. Notifiations that are too frequent may place undue
// stress on their battery to receive and process the updates.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <gio/gio.h>
#include <string>

#include "DBusObjectPath.h"
#include "Logger.h"

namespace ggk {

// ---------------------------------------------------------------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------------------------------------------------------------

struct DBusInterface;

// ---------------------------------------------------------------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------------------------------------------------------------

struct TickEvent
{
	//
	// Types
	//

	// A tick event callback, which is called whenever the TickEvent fires
	typedef void (*Callback)(const DBusInterface &self, const TickEvent &event, GDBusConnection *pConnection, void *pUserData);

	// Construct a TickEvent that will fire after a specified 'tickFrequency' number of ticks of the periodic timer.
	//
	// Note that the actual time between a callback's execution is the event's 'tickFrequency' multiplied by the time between each
	// periodic timer tick.
	TickEvent(const DBusInterface *pOwner, int tickFrequency, Callback callback, void *pUserData)
	: pOwner(pOwner), elapsedTicks(0), tickFrequency(tickFrequency), callback(callback), pUserData(pUserData)
	{
	}

	//
	// Accessors
	//

	// Returns the elapsed ticks since the last event firing
	int getElapsedTicks() const { return elapsedTicks; }

	// Sets the elapsed ticks since the last event firing
	void setElapsedTicks(int elapsed) { elapsedTicks = elapsed; }

	// Returns the tick frequency between schedule tick events
	int getTickFrequency() const { return tickFrequency; }

	// Sets the tick frequency between schedule tick events
	void setTickFrequency(int frequency) { tickFrequency = frequency; }

	// Returns the user data pointer associated to this TickEvent
	void *getUserData() { return pUserData; }

	// Sets the user data pointer associated to this TickEvent
	void setUserData(void *pUserData) { this->pUserData = pUserData; }

	// Gets the callback for the TickEvent
	Callback getCallback() const { return callback; }

	// Sets the callback for the TickEvent
	void setCallback(Callback callback) { this->callback = callback; }

	//
	// Tick management
	//

	// Perform a single tick of a TickEvent
	//
	// A TickEvent is ticked each time the periodic timer fires. The TickEvent only fires after `tickFrequency` ticks. As a result,
	// the `callback` is only called after a period of time equal to the time between firings of the periodic timer, multiplied by
	// `tickFrequency`.
	//
	// Returns true if event fires, false otherwise
	template<typename T>
	void tick(const DBusObjectPath &path, GDBusConnection *pConnection, void *pUserData) const
	{
		elapsedTicks += 1;
		if (elapsedTicks >= tickFrequency)
		{
			if (nullptr != callback)
			{
				Logger::debug(SSTR << "Ticking at path '" << path << "'");
				callback(*static_cast<const T *>(pOwner), *this, pConnection, pUserData);
			}

			elapsedTicks = 0;
		}
	}

private:

	//
	// Data members
	//

	const DBusInterface *pOwner;
	mutable int elapsedTicks;
	int tickFrequency;
	Callback callback;
	void *pUserData;
};

}; // namespace ggk
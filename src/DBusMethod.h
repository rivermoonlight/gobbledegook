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
// This is a representation of a D-Bus interface method.
//
// >>
// >>>  DISCUSSION
// >>
//
// See the discussion at the top of DBusMethod.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// This file contains a representation of a D-Bus interface method.
//
// Methods are identified by their name (such as "Get" or "Set"). They have argument definitions (defined as part of their
// interface) that describe the type of arguments passed into the method and returned from the method.
//
// In addition to the method itself, we also store a callback that can be called whenever the method is invoked.

#pragma once

#include <gio/gio.h>
#include <string>
#include <vector>

#include "Globals.h"
#include "DBusObjectPath.h"
#include "Logger.h"
#include "Server.h"

namespace ggk {

struct DBusInterface;

struct DBusMethod
{
	// A method callback delegate
	typedef void (*Callback)(const DBusInterface &self, GDBusConnection *pConnection, const std::string &methodName, GVariant *pParameters, GDBusMethodInvocation *pInvocation, void *pUserData);

	// Instantiate a named method on a given interface (pOwner) with a given set of arguments and a callback delegate
	DBusMethod(const DBusInterface *pOwner, const std::string &name, const char *pInArgs[], const char *pOutArgs, Callback callback);

	//
	// Accessors
	//

	// Returns the name of the method
	const std::string &getName() const { return name; }

	// Sets the name of the method
	//
	// This method should generally not be called directly. Rather, the name should be set by the constructor
	DBusMethod &setName(const std::string &name) { this->name = name; return *this; }

	// Get the input argument type string (a GVariant type string format)
	const std::vector<std::string> &getInArgs() const { return inArgs; }

	// Get the output argument type string (a GVariant type string format)
	const std::string &getOutArgs() const { return outArgs; }

	// Set the argument types for this method
	//
	// This method should generally not be called directly. Rather, the arguments should be set by the constructor
	DBusMethod &setArgs(const std::vector<std::string> &inArgs, const std::string &outArgs)
	{
		this->inArgs = inArgs;
		this->outArgs = outArgs;
		return *this;
	}

	//
	// Call the method
	//

	// Calls the method
	//
	// If a callback delegate has been set, then this method will call that delegate, otherwise this method will do nothing
	template<typename T>
	void call(GDBusConnection *pConnection, const DBusObjectPath &path, const std::string &interfaceName, const std::string &methodName, GVariant *pParameters, GDBusMethodInvocation *pInvocation, void *pUserData) const
	{
		// This should never happen, but technically possible if instantiated with a nullptr for `callback`
		if (!callback)
		{
			Logger::error(SSTR << "DBusMethod contains no callback: [" << path << "]:[" << interfaceName << "]:[" << methodName << "]");
			g_dbus_method_invocation_return_dbus_error(pInvocation, kErrorNotImplemented.c_str(), "This method is not implemented");
			return;
		}

		Logger::info(SSTR << "Calling method: [" << path << "]:[" << interfaceName << "]:[" << methodName << "]");
		callback(*static_cast<const T *>(pOwner), pConnection, methodName, pParameters, pInvocation, pUserData);
	}

	// Internal method used to generate introspection XML used to describe our services on D-Bus
	std::string generateIntrospectionXML(int depth) const;

private:
	const DBusInterface *pOwner;
	std::string name;
	std::vector<std::string> inArgs;
	std::string outArgs;
	Callback callback;
};

}; // namespace ggk
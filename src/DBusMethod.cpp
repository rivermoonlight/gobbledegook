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
// This is a representation of a D-Bus interface method.
//
// >>
// >>>  DISCUSSION
// >>
//
// Methods are identified by their name (such as "ReadValue" or "WriteValue"). They have argument definitions (defined as part of
// their interface) that describe the type of arguments passed into the method and returned from the method.
//
// In addition to the method itself, we also store a callback delegate that is responsible for performing the tasks for this method.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <gio/gio.h>
#include <string>
#include <vector>

#include "DBusMethod.h"

namespace ggk {

// Instantiate a named method on a given interface (pOwner) with a given set of arguments and a callback delegate
DBusMethod::DBusMethod(const DBusInterface *pOwner, const std::string &name, const char *pInArgs[], const char *pOutArgs, Callback callback)
: pOwner(pOwner), name(name), callback(callback)
{
	const char **ppInArg = pInArgs;
	while(*ppInArg)
	{
		this->inArgs.push_back(std::string(*ppInArg));
		ppInArg++;
	}

	if (nullptr != pOutArgs)
	{
		this->outArgs = pOutArgs;
	}
}

// Internal method used to generate introspection XML used to describe our services on D-Bus
std::string DBusMethod::generateIntrospectionXML(int depth) const
{
	std::string prefix;
	prefix.insert(0, depth * 2, ' ');

	std::string xml = std::string();

	xml += prefix + "<method name='" + getName() + "'>\n";

	// Add our input arguments
	for (const std::string &inArg : getInArgs())
	{
		xml += prefix + "  <arg type='" + inArg + "' direction='in'>\n";
		xml += prefix + "    <annotation name='org.gtk.GDBus.C.ForceGVariant' value='true' />\n";
		xml += prefix + "  </arg>\n";
	}

	const std::string &outArgs = getOutArgs();
	if (!outArgs.empty())
	{
		xml += prefix + "  <arg type='" + outArgs + "' direction='out'>\n";
		xml += prefix + "    <annotation name='org.gtk.GDBus.C.ForceGVariant' value='true' />\n";
		xml += prefix + "  </arg>\n";
	}

	xml += prefix + "</method>\n";

	return xml;
}

}; // namespace ggk
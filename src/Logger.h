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
// This is our Logger, which allows for applications to use their own logging mechanisms by registering log receivers for each of
// the logging categories.
//
// >>
// >>>  DISCUSSION
// >>
//
// See the discussion at the top of Logger.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <sstream>

#include "../include/Gobbledegook.h"

namespace ggk {

// Our handy stringstream macro
#define SSTR std::ostringstream().flush()

class Logger
{
public:

	//
	// Registration
	//

	// Register logging receiver for DEBUG logging.  To register a logging level, simply call with a delegate that performs the
	// appropriate logging action. To unregister, call with `nullptr`
	static void registerDebugReceiver(GGKLogReceiver receiver);

	// Register logging receiver for INFO logging.  To register a logging level, simply call with a delegate that performs the
	// appropriate logging action. To unregister, call with `nullptr`
	static void registerInfoReceiver(GGKLogReceiver receiver);

	// Register logging receiver for STATUS logging.  To register a logging level, simply call with a delegate that performs the
	// appropriate logging action. To unregister, call with `nullptr`
	static void registerStatusReceiver(GGKLogReceiver receiver);

	// Register logging receiver for WARN logging.  To register a logging level, simply call with a delegate that performs the
	// appropriate logging action. To unregister, call with `nullptr`
	static void registerWarnReceiver(GGKLogReceiver receiver);

	// Register logging receiver for ERROR logging.  To register a logging level, simply call with a delegate that performs the
	// appropriate logging action. To unregister, call with `nullptr`
	static void registerErrorReceiver(GGKLogReceiver receiver);

	// Register logging receiver for FATAL logging.  To register a logging level, simply call with a delegate that performs the
	// appropriate logging action. To unregister, call with `nullptr`
	static void registerFatalReceiver(GGKLogReceiver receiver);

	// Register logging receiver for ALWAYS logging.  To register a logging level, simply call with a delegate that performs the
	// appropriate logging action. To unregister, call with `nullptr`
	static void registerAlwaysReceiver(GGKLogReceiver receiver);

	// Register logging receiver for TRACE logging.  To register a logging level, simply call with a delegate that performs the
	// appropriate logging action. To unregister, call with `nullptr`
	static void registerTraceReceiver(GGKLogReceiver receiver);


	//
	// Logging actions
	//

	// Log a DEBUG entry with a C string
	static void debug(const char *pText);

	// Log a DEBUG entry with a string
	static void debug(const std::string &text);

	// Log a DEBUG entry using a stream
	static void debug(const std::ostream &text);

	// Log a INFO entry with a C string
	static void info(const char *pText);

	// Log a INFO entry with a string
	static void info(const std::string &text);

	// Log a INFO entry using a stream
	static void info(const std::ostream &text);

	// Log a STATUS entry with a C string
	static void status(const char *pText);

	// Log a STATUS entry with a string
	static void status(const std::string &text);

	// Log a STATUS entry using a stream
	static void status(const std::ostream &text);

	// Log a WARN entry with a C string
	static void warn(const char *pText);

	// Log a WARN entry with a string
	static void warn(const std::string &text);

	// Log a WARN entry using a stream
	static void warn(const std::ostream &text);

	// Log a ERROR entry with a C string
	static void error(const char *pText);

	// Log a ERROR entry with a string
	static void error(const std::string &text);

	// Log a ERROR entry using a stream
	static void error(const std::ostream &text);

	// Log a FATAL entry with a C string
	static void fatal(const char *pText);

	// Log a FATAL entry with a string
	static void fatal(const std::string &text);

	// Log a FATAL entry using a stream
	static void fatal(const std::ostream &text);

	// Log a ALWAYS entry with a C string
	static void always(const char *pText);

	// Log a ALWAYS entry with a string
	static void always(const std::string &text);

	// Log a ALWAYS entry using a stream
	static void always(const std::ostream &text);

	// Log a TRACE entry with a C string
	static void trace(const char *pText);

	// Log a TRACE entry with a string
	static void trace(const std::string &text);

	// Log a TRACE entry using a stream
	static void trace(const std::ostream &text);

private:

	// The registered log receiver for DEBUG logs - a nullptr will cause the logging for that receiver to be ignored
	static GGKLogReceiver logReceiverDebug;

	// The registered log receiver for INFO logs - a nullptr will cause the logging for that receiver to be ignored
	static GGKLogReceiver logReceiverInfo;

	// The registered log receiver for STATUS logs - a nullptr will cause the logging for that receiver to be ignored
	static GGKLogReceiver logReceiverStatus;

	// The registered log receiver for WARN logs - a nullptr will cause the logging for that receiver to be ignored
	static GGKLogReceiver logReceiverWarn;

	// The registered log receiver for ERROR logs - a nullptr will cause the logging for that receiver to be ignored
	static GGKLogReceiver logReceiverError;

	// The registered log receiver for FATAL logs - a nullptr will cause the logging for that receiver to be ignored
	static GGKLogReceiver logReceiverFatal;

	// The registered log receiver for ALWAYS logs - a nullptr will cause the logging for that receiver to be ignored
	static GGKLogReceiver logReceiverAlways;

	// The registered log receiver for TRACE logs - a nullptr will cause the logging for that receiver to be ignored
	static GGKLogReceiver logReceiverTrace;
};

}; // namespace ggk
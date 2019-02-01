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
// In order for logging to work, the application must register logging receivers with the `ggkLogRegister*` methods. At that point,
// it is up to the application to decide how to handle the incoming log requests. For example, it may filter Error and Fatal logs
// to stderr.
//
// The following is a breakdown of the intended use of each of the available categories. Application receivers can filter then
// (enable/ disable/redirect/etc.) in whatever way works best (via bitmasks, log levels, etc.)
//
//     Debug  - Lowest logging level; may contain log-spam
//     Info   - The logging equivalent to the junk drawer
//     Status - Current health/service availability info (ex: "Server starting", "Initialization complete", "Stop requested")
//     Warn   - Problems that may or may not affect functionality
//     Error  - Problems that will affect functionality, but do not result in a termination of the process
//     Fatal  - Issues that so severe as to require termination
//
// The following log categories should be considered very important (and possibly never filtered):
//
//     Always - Use when something absolutely, positively has to show up in the log output
//     Trace  - Useful for tracing through code (ex: "Before call to foo()", "Entering function bar") and other development use
//              only and should be removed before a release.
//
// Using the logger is simple:
//
//    Logger::error("Unable to locate configuration file (this is probably bad)");
//
// There is an additional macro (SSTR) which can simplify sending dynamic data to the logger via a string stream:
//
//    Logger::info(SSTR << "There were " << count << " entries in the list");
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "Logger.h"

namespace ggk {

//
// Log receiver delegates
//

// The registered log receiver for DEBUG logs - a nullptr will cause the logging for that receiver to be ignored
GGKLogReceiver Logger::logReceiverDebug = nullptr;

// The registered log receiver for INFO logs - a nullptr will cause the logging for that receiver to be ignored
GGKLogReceiver Logger::logReceiverInfo = nullptr;

// The registered log receiver for STATUS logs - a nullptr will cause the logging for that receiver to be ignored
GGKLogReceiver Logger::logReceiverStatus = nullptr;

// The registered log receiver for WARN logs - a nullptr will cause the logging for that receiver to be ignored
GGKLogReceiver Logger::logReceiverWarn = nullptr;

// The registered log receiver for ERROR logs - a nullptr will cause the logging for that receiver to be ignored
GGKLogReceiver Logger::logReceiverError = nullptr;

// The registered log receiver for FATAL logs - a nullptr will cause the logging for that receiver to be ignored
GGKLogReceiver Logger::logReceiverFatal = nullptr;

// The registered log receiver for ALWAYS logs - a nullptr will cause the logging for that receiver to be ignored
GGKLogReceiver Logger::logReceiverAlways = nullptr;

// The registered log receiver for TRACE logs - a nullptr will cause the logging for that receiver to be ignored
GGKLogReceiver Logger::logReceiverTrace = nullptr;

//
// Registration
//

// Register logging receiver for DEBUG logging.  To register a logging level, simply call with a delegate that performs the
// appropriate logging action. To unregister, call with `nullptr`
void Logger::registerDebugReceiver(GGKLogReceiver receiver) { Logger::logReceiverDebug = receiver; }

// Register logging receiver for INFO logging.  To register a logging level, simply call with a delegate that performs the
// appropriate logging action. To unregister, call with `nullptr`
void Logger::registerInfoReceiver(GGKLogReceiver receiver) { Logger::logReceiverInfo = receiver; }

// Register logging receiver for STATUS logging.  To register a logging level, simply call with a delegate that performs the
// appropriate logging action. To unregister, call with `nullptr`
void Logger::registerStatusReceiver(GGKLogReceiver receiver) { Logger::logReceiverStatus = receiver; }

// Register logging receiver for WARN logging.  To register a logging level, simply call with a delegate that performs the
// appropriate logging action. To unregister, call with `nullptr`
void Logger::registerWarnReceiver(GGKLogReceiver receiver) { Logger::logReceiverWarn = receiver; }

// Register logging receiver for ERROR logging.  To register a logging level, simply call with a delegate that performs the
// appropriate logging action. To unregister, call with `nullptr`
void Logger::registerErrorReceiver(GGKLogReceiver receiver) { Logger::logReceiverError = receiver; }

// Register logging receiver for FATAL logging.  To register a logging level, simply call with a delegate that performs the
// appropriate logging action. To unregister, call with `nullptr`
void Logger::registerFatalReceiver(GGKLogReceiver receiver) { Logger::logReceiverFatal = receiver; }

// Register logging receiver for ALWAYS logging.  To register a logging level, simply call with a delegate that performs the
// appropriate logging action. To unregister, call with `nullptr`
void Logger::registerAlwaysReceiver(GGKLogReceiver receiver) { Logger::logReceiverAlways = receiver; }

// Register logging receiver for TRACE logging.  To register a logging level, simply call with a delegate that performs the
// appropriate logging action. To unregister, call with `nullptr`
void Logger::registerTraceReceiver(GGKLogReceiver receiver) { Logger::logReceiverTrace = receiver; }

//
// Logging actions
//

// Log a DEBUG entry with a C string
void Logger::debug(const char *pText) { if (nullptr != Logger::logReceiverDebug) { Logger::logReceiverDebug(pText); } }

// Log a DEBUG entry with a string
void Logger::debug(const std::string &text) { if (nullptr != Logger::logReceiverDebug) { debug(text.c_str()); } }

// Log a DEBUG entry using a stream
void Logger::debug(const std::ostream &text) { if (nullptr != Logger::logReceiverDebug) { debug(static_cast<const std::ostringstream &>(text).str().c_str()); } }

// Log a INFO entry with a C string
void Logger::info(const char *pText) { if (nullptr != Logger::logReceiverInfo) { Logger::logReceiverInfo(pText); } }

// Log a INFO entry with a string
void Logger::info(const std::string &text) { if (nullptr != Logger::logReceiverInfo) { info(text.c_str()); } }

// Log a INFO entry using a stream
void Logger::info(const std::ostream &text) { if (nullptr != Logger::logReceiverInfo) { info(static_cast<const std::ostringstream &>(text).str().c_str()); } }

// Log a STATUS entry with a C string
void Logger::status(const char *pText) { if (nullptr != Logger::logReceiverStatus) { Logger::logReceiverStatus(pText); } }

// Log a STATUS entry with a string
void Logger::status(const std::string &text) { if (nullptr != Logger::logReceiverStatus) { status(text.c_str()); } }

// Log a STATUS entry using a stream
void Logger::status(const std::ostream &text) { if (nullptr != Logger::logReceiverStatus) { status(static_cast<const std::ostringstream &>(text).str().c_str()); } }

// Log a WARN entry with a C string
void Logger::warn(const char *pText) { if (nullptr != Logger::logReceiverWarn) { Logger::logReceiverWarn(pText); } }

// Log a WARN entry with a string
void Logger::warn(const std::string &text) { if (nullptr != Logger::logReceiverWarn) { warn(text.c_str()); } }

// Log a WARN entry using a stream
void Logger::warn(const std::ostream &text) { if (nullptr != Logger::logReceiverWarn) { warn(static_cast<const std::ostringstream &>(text).str().c_str()); } }

// Log a ERROR entry with a C string
void Logger::error(const char *pText) { if (nullptr != Logger::logReceiverError) { Logger::logReceiverError(pText); } }

// Log a ERROR entry with a string
void Logger::error(const std::string &text) { if (nullptr != Logger::logReceiverError) { error(text.c_str()); } }

// Log a ERROR entry using a stream
void Logger::error(const std::ostream &text) { if (nullptr != Logger::logReceiverError) { error(static_cast<const std::ostringstream &>(text).str().c_str()); } }

// Log a FATAL entry with a C string
void Logger::fatal(const char *pText) { if (nullptr != Logger::logReceiverFatal) { Logger::logReceiverFatal(pText); } }

// Log a FATAL entry with a string
void Logger::fatal(const std::string &text) { if (nullptr != Logger::logReceiverFatal) { fatal(text.c_str()); } }

// Log a FATAL entry using a stream
void Logger::fatal(const std::ostream &text) { if (nullptr != Logger::logReceiverFatal) { fatal(static_cast<const std::ostringstream &>(text).str().c_str()); } }

// Log a ALWAYS entry with a C string
void Logger::always(const char *pText) { if (nullptr != Logger::logReceiverAlways) { Logger::logReceiverAlways(pText); } }

// Log a ALWAYS entry with a string
void Logger::always(const std::string &text) { if (nullptr != Logger::logReceiverAlways) { always(text.c_str()); } }

// Log a ALWAYS entry using a stream
void Logger::always(const std::ostream &text) { if (nullptr != Logger::logReceiverAlways) { always(static_cast<const std::ostringstream &>(text).str().c_str()); } }

// Log a TRACE entry with a C string
void Logger::trace(const char *pText) { if (nullptr != Logger::logReceiverTrace) { Logger::logReceiverTrace(pText); } }

// Log a TRACE entry with a string
void Logger::trace(const std::string &text) { if (nullptr != Logger::logReceiverTrace) { trace(text.c_str()); } }

// Log a TRACE entry using a stream
void Logger::trace(const std::ostream &text) { if (nullptr != Logger::logReceiverTrace) { trace(static_cast<const std::ostringstream &>(text).str().c_str()); } }

}; // namespace ggk
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
// This represents a custom string type for a D-Bus object path.
//
// >>
// >>>  DISCUSSION
// >>
//
// A D-Bus object path is normal string in the form "/com/example/foo/bar". This class provides a set of methods for building
// these paths safely in such a way that they are guaranteed to always provide a valid path.
//
// In addition to this functionality, our DBusObjectPath is its own distinct type requiring explicit conversion, providing a level
// of protection against accidentally using an arbitrary string as an object path.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <string>
#include <ostream>

namespace ggk {

struct DBusObjectPath
{
	// Default constructor (creates a root path)
	inline DBusObjectPath() { path = "/"; }

	// Copy constructor
	inline DBusObjectPath(const DBusObjectPath &path) : path(path.path) {}

	// Constructor that accepts a C string
	//
	// Note: explicit because we don't want accidental conversion. Creating a DBusObjectPath must be intentional.
	inline explicit DBusObjectPath(const char *pPath) : path(pPath) {}

	// Constructor that accepts a std::string
	//
	// Note: explicit because we don't want accidental conversion. Creating a DBusObjectPath must be intentional.
	inline explicit DBusObjectPath(const std::string &path) : path(path) {}

	// Explicit conversion to std::string
	inline const std::string &toString() const { return path; }

	// Explicit conversion to a C string
	inline const char *c_str() const { return path.c_str(); }

	// Assignment
	inline DBusObjectPath &operator =(const DBusObjectPath &rhs)
	{
		if (this == &rhs) return *this;
		path = rhs.path;
		return *this;
	}

	// Concatenation
	inline const DBusObjectPath &append(const char *rhs)
	{
		if (nullptr == rhs || !*rhs) { return *this; }
		if (path.empty()) { path = rhs; return *this; }

		bool ls = path.back() == '/';
		bool rs = *rhs == '/';
		if (ls && rs) { path.erase(path.length()-1); }
		if (!ls && !rs) { path += "/"; }

		path += rhs;
		return *this;
	}

	// Adds a path node (in the form of an std::string) to the end of the path
	inline const DBusObjectPath &append(const std::string &rhs)
	{
		return append(rhs.c_str());
	}

	// Adds a path node (in the form of a DBusObjectPath) to the end of the path
	inline const DBusObjectPath &append(const DBusObjectPath &rhs)
	{
		return append(rhs.path.c_str());
	}

	// Adds a path node (in the form of a DBusObjectPath) to the end of the path
	inline void operator +=(const DBusObjectPath &rhs)
	{
		append(rhs);
	}

	// Adds a path node (in the form of a C string) to the end of the path
	inline void operator +=(const char *rhs)
	{
		append(rhs);
	}

	// Adds a path node (in the form of an std::string) to the end of the path
	inline void operator +=(const std::string &rhs)
	{
		append(rhs);
	}

	// Concats two DBusObjectPaths into one, returning the resulting path
	inline DBusObjectPath operator +(const DBusObjectPath &rhs) const
	{
		DBusObjectPath result(*this);
		result += rhs;
		return result;
	}

	// Concats a C string onto a DBusObjectPath, returning the resulting path
	inline DBusObjectPath operator +(const char *rhs) const
	{
		DBusObjectPath result(*this);
		result += rhs;
		return result;
	}

	// Concats a std::string onto a DBusObjectPath, returning the resulting path
	inline DBusObjectPath operator +(const std::string &rhs) const
	{
		DBusObjectPath result(*this);
		result += rhs;
		return result;
	}

	// Tests two DBusObjectPaths for equality, returning true of the two strings are identical
	inline bool operator ==(const DBusObjectPath &rhs) const
	{
		return path == rhs.path;
	}

private:

	std::string path;
};

// Mixed-mode override for adding a DBusObjectPath to a C string, returning a new DBusObjectPath result
inline DBusObjectPath operator +(const char *lhs, const DBusObjectPath &rhs) { return DBusObjectPath(lhs) + rhs; }

// Mixed-mode override for adding a DBusObjectPath to a std::string, returning a new DBusObjectPath result
inline DBusObjectPath operator +(const std::string &lhs, const DBusObjectPath &rhs) { return DBusObjectPath(lhs) + rhs; }

// Streaming support for our DBusObjectPath (useful for our logging mechanism)
inline std::ostream& operator<<(std::ostream &os, const DBusObjectPath &path)
{
    os << path.toString();
    return os;
}

// Streaming support for our DBusObjectPath (useful for our logging mechanism)
inline std::ostream& operator +(std::ostream &os, const DBusObjectPath &path)
{
    os << path.toString();
    return os;
}

}; // namespace ggk
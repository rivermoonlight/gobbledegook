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
// This is an abstraction layer for a D-Bus interface, the base class for all interfaces.
//
// >>
// >>>  DISCUSSION
// >>
//
// See the discussion in DBusInterface.cpp.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <gio/gio.h>
#include <string>
#include <list>

#include "TickEvent.h"
#include "DBusMethod.h"

namespace ggk {

// ---------------------------------------------------------------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------------------------------------------------------------

struct DBusInterface;
struct GattProperty;
struct DBusObject;
struct DBusObjectPath;

// ---------------------------------------------------------------------------------------------------------------------------------
// Useful Lambdas
// ---------------------------------------------------------------------------------------------------------------------------------

#define INTERFACE_METHOD_CALLBACK_LAMBDA [] \
( \
       const DBusInterface &self, \
       GDBusConnection *pConnection, \
       const std::string &methodName, \
       GVariant *pParameters, \
       GDBusMethodInvocation *pInvocation, \
       void *pUserData \
)

#define TRY_GET_INTERFACE_OF_TYPE(pInterface, type) \
	(pInterface->getInterfaceType() == type::kInterfaceType ? \
		std::static_pointer_cast<type>(pInterface) : \
		nullptr)

#define TRY_GET_CONST_INTERFACE_OF_TYPE(pInterface, type) \
	(pInterface->getInterfaceType() == type::kInterfaceType ? \
		std::static_pointer_cast<const type>(pInterface) : \
		nullptr)

// ---------------------------------------------------------------------------------------------------------------------------------
// Representation of a D-Bus interface
// ---------------------------------------------------------------------------------------------------------------------------------

struct DBusInterface
{
	// Our interface type
	static constexpr const char *kInterfaceType = "DBusInterface";

	typedef void (*MethodCallback)(const DBusInterface &self, GDBusConnection *pConnection, const std::string &methodName, GVariant *pParameters, GDBusMethodInvocation *pInvocation, void *pUserData);
	typedef void (*EventCallback)(const DBusInterface &self, const TickEvent &event, GDBusConnection *pConnection, void *pUserData);

	// Standard constructor
	DBusInterface(DBusObject &owner, const std::string &name);
	virtual ~DBusInterface();

	// Returns a string identifying the type of interface
	virtual const std::string getInterfaceType() const { return DBusInterface::kInterfaceType; }

	//
	// Interface name (ex: "org.freedesktop.DBus.Properties")
	//

	const std::string &getName() const;
	DBusInterface &setName(const std::string &name);

	//
	// Owner information
	//

	DBusObject &getOwner() const;
	DBusObjectPath getPathNode() const;
	DBusObjectPath getPath() const;

	//
	// D-Bus interface methods
	//

	DBusInterface &addMethod(const std::string &name, const char *pInArgs[], const char *pOutArgs, DBusMethod::Callback callback);

	// NOTE: Subclasses are encouraged to override this method in order to support different callback types that are specific to
	// their subclass type.
	virtual bool callMethod(const std::string &methodName, GDBusConnection *pConnection, GVariant *pParameters, GDBusMethodInvocation *pInvocation, gpointer pUserData) const;

	//
	// Interface events (our home-grown poor-mans's method of allowing interfaces to do things periodically)
	//

	// NOTE: Subclasses are encouraged to overload this method in order to support different callback types that are specific to
	// their subclass type. In addition, they should return their own type. This simplifies the server description by allowing
	// calls to chain.
	DBusInterface &onEvent(int tickFrequency, void *pUserData, TickEvent::Callback callback);

	// NOTE: Subclasses are encouraged to override this method in order to support different callback types that are specific to
	// their subclass type.
	virtual void tickEvents(GDBusConnection *pConnection, void *pUserData) const;

	// Internal method used to generate introspection XML used to describe our services on D-Bus
	virtual std::string generateIntrospectionXML(int depth) const;

protected:
	DBusObject &owner;
	std::string name;
	std::list<DBusMethod> methods;
	std::list<TickEvent> events;
};

}; // namespace ggk
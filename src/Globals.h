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
// System-wide globals.
//
// >>
// >>>  DISCUSSION
// >>
//
// The globals below define the name of the server, along with any name-based items (such as errors.)
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <string>

//
// Custom defined errors
//

// In order to avoid confusion, we should use the owned name here, so errors are like extensions to that name. This way, if a
// client gets one of these errors, it'll be clear which server it came from.
#define kErrorNotImplemented (TheServer->getOwnedName() + ".NotImplemented")

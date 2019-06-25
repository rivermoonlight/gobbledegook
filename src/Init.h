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
// Herein lies the code that manages the full initialization (including the running) of the server
//
// >>
// >>>  DISCUSSION
// >>
//
// See the discussion at the top of Init.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

namespace ggk {

// Trigger a graceful, asynchronous shutdown of the server
//
// This method is non-blocking and as such, will only trigger the shutdown process but not wait for it
void shutdown();

// Entry point for the asynchronous server thread
//
// This method should not be called directly, instead, direct your attention over to `ggkStart()`
void runServerThread();

}; // namespace ggk
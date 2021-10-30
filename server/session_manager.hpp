/*
 * Copyright (c) 2021, Kirill GPRB. All Rights Reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once
#include <shared/session.hpp>

namespace session_manager
{
void init();
Session *create();
Session *find(uint32_t session_id);
void destroy(Session *session);
void kick(Session *session, const std::string &reason);
void kickAll(const std::string &reason);
} // namespace session_manager

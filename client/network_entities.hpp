/*
 * Copyright (c) 2021, Kirill GPRB. All Rights Reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once
#include <common/math/types.hpp>
#include <entt/entt.hpp>

namespace network_entities
{
void clear();
entt::entity create(uint32_t network_id);
entt::entity find(uint32_t network_id);
void remove(uint32_t network_id);
} // namespace network_entities
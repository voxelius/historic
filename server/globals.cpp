/*
 * globals.cpp
 * Copyright (c) 2021, Kirill GPRB.
 * All Rights Reserved.
 */
#include <server/chunks.hpp>
#include <server/globals.hpp>
#include <shared/script/script_engine.hpp>
#include <shared/voxels.hpp>

bool sv_globals::running = false;

// Networking
ENetHost *sv_globals::host = nullptr;

// World
entt::registry sv_globals::registry;
ServerChunkManager sv_globals::chunks;
VoxelDef sv_globals::voxels;

// Scripting & configuration
ScriptEngine sv_globals::script;

// Stats
float sv_globals::curtime = 0.0f;
float sv_globals::ticktime = 0.0f;
uint64_t sv_globals::num_ticks = 0;

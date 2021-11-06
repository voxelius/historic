/*
 * proj_view.cpp
 * Copyright (c) 2021, Kirill GPRB.
 * All Rights Reserved.
 */
#include <client/components/local_player.hpp>
#include <client/systems/proj_view.hpp>
#include <client/globals.hpp>
#include <client/input.hpp>
#include <client/shadow_manager.hpp>
#include <client/screen.hpp>
#include <shared/components/creature.hpp>
#include <shared/components/head.hpp>
#include <shared/components/player.hpp>
#include <shared/cvar.hpp>
#include <shared/world.hpp>
#include <spdlog/spdlog.h>
#include <shared/session.hpp>

static float2 pv_angles = FLOAT2_ZERO;
static float3 pv_position = FLOAT3_ZERO;
static float4x4 pv_matrix = FLOAT4X4_IDENTITY;
static float4x4 pv_matrix_shadow = FLOAT4X4_IDENTITY;
static Frustum pv_frustum, pv_frustum_shadow;

static CVar r_fov("r.fov", "90.0");
static CVar r_znear("r.znear", "0.01");
static CVar r_zfar("r.zfar", "1024.0");

void proj_view::init()
{
    globals::cvars.insert(r_fov);
    globals::cvars.insert(r_znear);
    globals::cvars.insert(r_zfar);
}

void proj_view::update()
{
    pv_position = FLOAT3_ZERO;
    pv_matrix = glm::perspective(glm::radians(r_fov.getFloat()), screen::getAspectRatio(), r_znear.getFloat(), r_zfar.getFloat());

    const auto hg = globals::registry.group(entt::get<HeadComponent, PlayerComponent, LocalPlayerComponent>);
    for(const auto [entity, head, player] : hg.each()) {
        pv_angles = head.angles;
        pv_position = head.offset;
        if(CreatureComponent *creature = globals::registry.try_get<CreatureComponent>(entity))
            pv_position += creature->position;
        pv_matrix *= glm::lookAt(pv_position, pv_position + floatquat(float3(pv_angles.x, pv_angles.y, 0.0f)) * FLOAT3_FORWARD, FLOAT3_UP);
        break;
    }

    pv_frustum.update(pv_matrix);

    pv_matrix_shadow = shadow_manager::getProjView(pv_position);
    pv_frustum_shadow.update(pv_matrix_shadow);
}

const float2 &proj_view::angles()
{
    return pv_angles;
}

const float3 &proj_view::position()
{
    return pv_position;
}

const float4x4 &proj_view::matrix()
{
    return pv_matrix;
}

const float4x4 &proj_view::matrixShadow()
{
    return pv_matrix_shadow;
}

const Frustum &proj_view::frustum()
{
    return pv_frustum;
}

const Frustum &proj_view::frustumShadow()
{
    return pv_frustum_shadow;
}

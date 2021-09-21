/*
 * Copyright (c) 2021, Kirill GPRB. All Rights Reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <filesystem.hpp>
#include <client/comp/voxel_mesh.hpp>
#include <client/sys/proj_view.hpp>
#include <client/sys/voxel_renderer.hpp>
#include <client/globals.hpp>
#include <spdlog/spdlog.h>
#include <client/vertex.hpp>
#include <math/util.hpp>
#include <client/gl/pipeline.hpp>
#include <client/gl/sampler.hpp>
#include <client/gl/shader.hpp>
#include <client/atlas.hpp>
#include <shared/world.hpp>

constexpr static const char *VERT_NAME = "shaders/voxel.vert.glsl";
constexpr static const char *FRAG_NAME = "shaders/voxel.frag.glsl";

struct alignas(16) UBufferData final {
    float4x4_t projview;
    float3_t chunkpos;
};

static gl::Shader shaders[2];
static gl::Pipeline pipeline;
static gl::Sampler sampler;
static gl::Buffer ubuffer;

void voxel_renderer::init()
{
    std::string vsrc, fsrc;
    if(!fs::readText(VERT_NAME, vsrc) || !fs::readText(FRAG_NAME, fsrc))
        std::terminate();

    shaders[0].create();
    if(!shaders[0].glsl(GL_VERTEX_SHADER, vsrc))
        std::terminate();

    shaders[1].create();
    if(!shaders[1].glsl(GL_FRAGMENT_SHADER, fsrc))
        std::terminate();

    pipeline.create();
    pipeline.stage(shaders[0]);
    pipeline.stage(shaders[1]);

    sampler.create();
    sampler.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    sampler.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    ubuffer.create();
    ubuffer.resize(sizeof(UBufferData), nullptr, GL_DYNAMIC_DRAW);
}

void voxel_renderer::shutdown()
{
    ubuffer.destroy();
    sampler.destroy();
    pipeline.destroy();
    shaders[1].destroy();
    shaders[0].destroy();
}

// UNDONE: move this somewhere else
static inline bool isInFrustum(const Frustum &frustum, const float3_t &view, const chunkpos_t &cp)
{
    const float3_t wp = toWorldPos(cp);
    if(math::isInBB(view, wp, wp + float3_t(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE)))
        return true;
    if(frustum.point(wp + float3_t(0.0f, 0.0f, 0.0f)))
        return true;
    if(frustum.point(wp + float3_t(0.0f, 0.0f, CHUNK_SIZE)))
        return true;
    if(frustum.point(wp + float3_t(0.0f, CHUNK_SIZE, 0.0f)))
        return true;
    if(frustum.point(wp + float3_t(0.0f, CHUNK_SIZE, CHUNK_SIZE)))
        return true;
    if(frustum.point(wp + float3_t(CHUNK_SIZE, 0.0f, 0.0f)))
        return true;
    if(frustum.point(wp + float3_t(CHUNK_SIZE, 0.0f, CHUNK_SIZE)))
        return true;
    if(frustum.point(wp + float3_t(CHUNK_SIZE, CHUNK_SIZE, 0.0f)))
        return true;
    if(frustum.point(wp + float3_t(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE)))
        return true;
    return false;
}

void voxel_renderer::update()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    UBufferData ubuffer_data = {};
    ubuffer_data.projview = proj_view::matrix();
    ubuffer.write(offsetof(UBufferData, projview), sizeof(UBufferData::projview), &ubuffer_data.projview);

    pipeline.bind();
    sampler.bind(0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubuffer.get());
    cl_globals::solid_textures.getTexture().bind(0);

    const float3_t &view = proj_view::position();
    const Frustum &frustum = proj_view::frustum();

    auto group = cl_globals::registry.group(entt::get<VoxelMeshComponent, chunkpos_t>);
    for(const auto [entity, mesh, chunkpos] : group.each()) {
        if(isInFrustum(frustum, view, chunkpos)) {
            ubuffer_data.chunkpos = toWorldPos(chunkpos);
            ubuffer.write(offsetof(UBufferData, chunkpos), sizeof(UBufferData::chunkpos), &ubuffer_data.chunkpos);
            mesh.vao.bind();
            mesh.cmd.invoke();
        }
    }
}

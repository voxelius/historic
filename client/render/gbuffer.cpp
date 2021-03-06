/*
 * gbuffer.cpp
 * Copyright (c) 2021, Kirill GPRB.
 * All Rights Reserved.
 */
#include <client/render/gbuffer.hpp>
#include <exception>

void GBuffer::init(int width, int height)
{
    shutdown();

    albedo.create();
    albedo.storage(width, height, gl::PixelFormat::R32G32B32A32_FLOAT);

    normal.create();
    normal.storage(width, height, gl::PixelFormat::R32G32B32_FLOAT);

    position.create();
    position.storage(width, height, gl::PixelFormat::R32G32B32_FLOAT);

    shadow_projcoord.create();
    shadow_projcoord.storage(width, height, gl::PixelFormat::R32G32B32_FLOAT);

    rbo.create();
    rbo.storage(width, height, gl::PixelFormat::D24_UNORM);

    fbo.create();
    fbo.attach(GL_DEPTH_ATTACHMENT, rbo);
    fbo.attach(GL_COLOR_ATTACHMENT0, albedo);
    fbo.attach(GL_COLOR_ATTACHMENT1, normal);
    fbo.attach(GL_COLOR_ATTACHMENT2, position);
    fbo.attach(GL_COLOR_ATTACHMENT3, shadow_projcoord);
    fbo.setFragmentTargets(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3);
}

void GBuffer::shutdown()
{
    fbo.destroy();
    rbo.destroy();
    shadow_projcoord.destroy();
    position.destroy();
    normal.destroy();
    albedo.destroy();
}

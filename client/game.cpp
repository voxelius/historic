/*
 * game.cpp
 * Copyright (c) 2021, Kirill GPRB.
 * All Rights Reserved.
 */
#include <common/math/const.hpp>
#include <common/math/math.hpp>
#include <exception>
#include <client/components/local_player.hpp>
#include <client/systems/chunk_mesher.hpp>
#include <client/systems/player_look.hpp>
#include <client/systems/player_move.hpp>
#include <client/systems/proj_view.hpp>
#include <client/render/atlas.hpp>
#include <client/systems/chunk_renderer.hpp>
#include <client/systems/username_renderer.hpp>
#include <client/deferred_pass.hpp>
#include <client/render/gbuffer.hpp>
#include <client/util/screenshots.hpp>
#include <client/chunks.hpp>
#include <client/debug_overlay.hpp>
#include <client/fontlib.hpp>
#include <client/game.hpp>
#include <client/globals.hpp>
#include <client/input.hpp>
#include <client/network.hpp>
#include <client/screen.hpp>
#include <client/shadow_manager.hpp>
#include <chrono>
#include <shared/components/creature.hpp>
#include <shared/components/head.hpp>
#include <shared/components/player.hpp>
#include <shared/session.hpp>
#include <common/util/clock.hpp>
#include <shared/util/enet.hpp>
#include <shared/voxels.hpp>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <imgui.h>
#include <client/config.hpp>

static constexpr const float TICK_DT = 1.0f / protocol::DEFAULT_TICKRATE;
static ChronoClock<std::chrono::system_clock> tick_clock;

void cl_game::init()
{
    chunk_renderer::init();
    deferred_pass::init();

    shadow_manager::init();
    shadow_manager::setPolygonOffset(float2(3.0f, 0.25f));

    // Aside from 45 degrees azimuth, this should
    // approximately look like a solstice in Moscow.
    //shadow_manager::setLightOrientation(floatquat(glm::radians(float3(60.0f, 45.0f, 0.0f))));

    shadow_manager::setLightOrientation(floatquat(glm::radians(float3(45.0f, 45.0f, 0.0f))));

    if(globals::config.autoconnect.enable)
        network::connect(globals::config.autoconnect.host, globals::config.autoconnect.port);
}

void cl_game::postInit()
{
    fontlib::build("Console")
        .file("fonts/RobotoMono-Regular.ttf")
            .range(ImGui::GetIO().Fonts->GetGlyphRangesCyrillic())
            .size(20.0f)
            .endFile()
        .submit();

    tick_clock.restart();
}

void cl_game::shutdown()
{
    chunk_mesher::shutdown();

    globals::registry.clear();

    globals::solid_textures.destroy();

    globals::solid_gbuffer.shutdown();

    shadow_manager::shutdown();

    deferred_pass::shutdown();
    chunk_renderer::shutdown();
}

void cl_game::modeChange(int width, int height)
{
    globals::solid_gbuffer.init(width, height);
}

void cl_game::update()
{
    if(globals::session.state == SessionState::PLAYING) {
        // NOTENOTE: when the new chunks arrive (during the login stage
        // when clientside receives some important data like voxel info)
        // sometimes shit gets fucked and one side of a chunk becomes
        // visible. This is not a problem for now because these quads
        // are occluded by the actual geometry. Things will get hot only
        // when mesher will start to skip actual geometry leaving holes.
        chunk_mesher::update();

        // Input-dependent game systems
        if(!globals::ui_grabs_input) {
            player_look::update();
            player_move::update();
        }

        proj_view::update();

        if(util::seconds<float>(tick_clock.elapsed()) >= TICK_DT) {
            player_look::send();
            player_move::send();
            tick_clock.restart();
        }
    }

    input::enableCursor(globals::ui_grabs_input || globals::session.state != SessionState::PLAYING);
}

void cl_game::draw()
{
    // Clear shadowmap(s)
    shadow_manager::getShadowMap().getFBO().bind();
    glClearDepthf(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // Draw things to GBuffers
    chunk_renderer::draw();

    int width, height;
    screen::getSize(width, height);
    glViewport(0, 0, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw things to the main framebuffer
    deferred_pass::draw();
}

void cl_game::drawImgui()
{
    username_renderer::drawImgui();
    debug_overlay::drawImgui();
}

void cl_game::postDraw()
{
    if(input::isKeyJustPressed(GLFW_KEY_F2))
        screenshots::jpeg(100);
}

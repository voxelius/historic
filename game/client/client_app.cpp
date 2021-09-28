/*
 * Copyright (c) 2021, Kirill GPRB. All Rights Reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <game/shared/enet/host.hpp>
#include <game/shared/packets/common.hpp>
#include <game/client/comp/camera.hpp>
#include <game/client/comp/local_player.hpp>
#include <game/client/util/clock.hpp>
#include <game/client/comp/chunk_mesh.hpp>
#include <game/client/sys/player_look.hpp>
#include <game/client/sys/player_move.hpp>
#include <game/client/sys/proj_view.hpp>
#include <game/client/sys/chunk_mesher.hpp>
#include <game/client/sys/chunk_renderer.hpp>
#include <game/client/client_app.hpp>
#include <game/client/globals.hpp>
#include <game/client/input.hpp>
#include <game/client/screen.hpp>
#include <game/shared/comp/creature.hpp>
#include <game/shared/comp/head.hpp>
#include <game/shared/comp/player.hpp>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <ctime>
#include <game/client/gl/context.hpp>
#include <glm/gtc/noise.hpp>
#include <game/client/chunks.hpp>
#include <game/shared/voxels.hpp>
#include <game/client/atlas.hpp>
#include <game/client/gbuffer.hpp>
#include <game/client/shadow_manager.hpp>
#include <game/client/composite.hpp>
#include <random>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <game/client/debug_overlay.hpp>
#include <game/client/util/screenshots.hpp>

static void glfwOnError(int code, const char *message)
{
    spdlog::error("GLFW ({}): {}", code, message);
}

static inline float octanoise(const float3 &v, unsigned int oct)
{
    float result = 1.0;
    for(unsigned int i = 1; i <= oct; i++)
        result += glm::simplex(v * static_cast<float>(i));
    return result / static_cast<float>(oct);
}

static void generate(uint64_t seed = 0)
{
    constexpr const int64_t START = -128;
    constexpr const int64_t END = 128;

    std::mt19937_64 mtgen = std::mt19937_64(seed);
    const float seed_f = std::uniform_real_distribution<float>()(mtgen);
    for(int64_t vx = START; vx < END; vx++) {
        for(int64_t vz = START; vz < END; vz++) {
            const float3 vxz = float3(vx, vz, seed_f * 5120.0f);
            const float solidity = octanoise(vxz / 160.0f, 3);
            const float hmod = octanoise((vxz + 1.0f) / 160.0f, 8);
            if(solidity > 0.1f) {
                int64_t h1 = ((solidity - 0.1f) * 32.0f);
                int64_t h2 = (hmod * 16.0f);
                for(int64_t vy = 1; vy < h1; vy++)
                    cl_globals::chunks.set(voxelpos_t(vx, -vy, vz), 0x01, true);
                for(int64_t vy = 0; h1 && vy < h2; vy++)
                    cl_globals::chunks.set(voxelpos_t(vx, vy, vz), (vy == h2 - 1) ? 0x03 : 0x02, VOXEL_SET_FORCE);
            }
        }
    }
}

void client_app::run()
{
    enet::Host host(enet::NULL_ADDRESS, 1, 2, 0, 0);
    if(host.get()) {
        enet::Address address;
        address.setHost("localhost");
        address.setPort(24000);

        enet::PeerRef peer(host.connect(address, 2));
        if(peer.get()) {
            ENetEvent event;

            if(host.service(event, 3000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
                spdlog::info("CL: connected.");

                packets::TestPacket tp;
                tp.a = 1337;
                tp.b = 0xAB0BA;

                packets::buffer_type buffer;
                size_t length = bitsery::quickSerialization(packets::output_adapter { buffer }, tp);

                enet::Packet packet(length, ENET_PACKET_FLAG_RELIABLE, buffer.data());
                peer.send(0, std::move(packet));
                host.flush();

                spdlog::info("CL: sent a packet with the size of {}", length);

                peer.disconnect();
                while(host.service(event, 3000) > 0) {
                    if(event.type == ENET_EVENT_TYPE_DISCONNECT) {
                        spdlog::info("CL: disconnected.");
                        break;
                    }

                    if(event.type == ENET_EVENT_TYPE_RECEIVE) {
                        enet_packet_destroy(event.packet);
                        continue;
                    }
                }
                
                peer.reset();
            }
        }
    }

    glfwSetErrorCallback(glfwOnError);
    if(!glfwInit()) {
        spdlog::error("glfwInit() failed.");
        std::terminate();
    }

    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    gl::setHints();

    cl_globals::window = glfwCreateWindow(1152, 648, "Client", nullptr, nullptr);
    if(!cl_globals::window) {
        spdlog::error("glfwCreateWindow() failed.");
        std::terminate();
    }

    glfwMakeContextCurrent(cl_globals::window);
    gl::init();

    input::init();
    screen::init();

    chunk_renderer::init();

    cl_globals::registry.clear();

    // Stone
    {
        VoxelInfo vinfo = {};
        vinfo.type = VoxelType::SOLID;
        vinfo.faces.push_back({ VoxelFace::LF, "textures/stone.png" });
        vinfo.faces.push_back({ VoxelFace::RT, "textures/stone.png" });
        vinfo.faces.push_back({ VoxelFace::FT, "textures/stone.png" });
        vinfo.faces.push_back({ VoxelFace::BK, "textures/stone.png" });
        vinfo.faces.push_back({ VoxelFace::UP, "textures/stone.png" });
        vinfo.faces.push_back({ VoxelFace::DN, "textures/stone.png" });
        cl_globals::voxels.set(0x01, vinfo);
    }

    // Dirt
    {
        VoxelInfo vinfo = {};
        vinfo.type = VoxelType::SOLID;
        vinfo.faces.push_back({ VoxelFace::LF, "textures/dirt.png" });
        vinfo.faces.push_back({ VoxelFace::RT, "textures/dirt.png" });
        vinfo.faces.push_back({ VoxelFace::FT, "textures/dirt.png" });
        vinfo.faces.push_back({ VoxelFace::BK, "textures/dirt.png" });
        vinfo.faces.push_back({ VoxelFace::UP, "textures/dirt.png" });
        vinfo.faces.push_back({ VoxelFace::DN, "textures/dirt.png" });
        cl_globals::voxels.set(0x02, vinfo);
    }

    // Grass
    {
        VoxelInfo vinfo = {};
        vinfo.type = VoxelType::SOLID;
        vinfo.faces.push_back({ VoxelFace::LF, "textures/grass_side.png" });
        vinfo.faces.push_back({ VoxelFace::RT, "textures/grass_side.png" });
        vinfo.faces.push_back({ VoxelFace::FT, "textures/grass_side.png" });
        vinfo.faces.push_back({ VoxelFace::BK, "textures/grass_side.png" });
        vinfo.faces.push_back({ VoxelFace::UP, "textures/grass.png" });
        vinfo.faces.push_back({ VoxelFace::DN, "textures/dirt.png" });
        cl_globals::voxels.set(0x03, vinfo);
    }

    // Player entity >_<
    {
        entt::entity player = cl_globals::registry.create();
        cl_globals::registry.emplace<ActiveCameraComponent>(player);
        cl_globals::registry.emplace<LocalPlayerComponent>(player);
        cl_globals::registry.emplace<CreatureComponent>(player);
        cl_globals::registry.emplace<PlayerComponent>(player);

        HeadComponent &head = cl_globals::registry.emplace<HeadComponent>(player);
        head.offset = FLOAT3_ZERO;

        CameraComponent &camera = cl_globals::registry.emplace<CameraComponent>(player);
        camera.fov = glm::radians(90.0f);
        camera.z_far = 1024.0f;
        camera.z_near = 0.01f;
    }

    uint64_t seed = static_cast<uint64_t>(std::time(nullptr));
    spdlog::info("Generating ({})...", seed);
    generate(seed);

    cl_globals::solid_textures.create(32, 32, MAX_VOXELS);
    for(VoxelDef::const_iterator it = cl_globals::voxels.cbegin(); it != cl_globals::voxels.cend(); it++) {
        for(const VoxelFaceInfo &face : it->second.faces) {
            cl_globals::solid_textures.push(face.texture);
        }
    }
    cl_globals::solid_textures.submit();

    float sun_time = 0.0f;
    shadow_manager::init(8192, 8192);
    shadow_manager::setLightOrientation(floatquat(glm::radians(float3(0.0f, 0.0f, 45.0f))));
    shadow_manager::setPolygonOffset(float2(3.0f, 0.5f));

    composite::init();

    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(cl_globals::window, false);
    ImGui_ImplOpenGL3_Init("#version 460");

    input::enableCursor(false);

    Clock fps_clock;
    cl_globals::frame_count = 0;
    while(!glfwWindowShouldClose(cl_globals::window)) {
        cl_globals::vertices_drawn = 0;
        cl_globals::curtime = static_cast<float>(glfwGetTime());
        cl_globals::frametime = fps_clock.restart();
        cl_globals::avg_frametime += cl_globals::frametime;
        cl_globals::avg_frametime *= 0.5f;

        shadow_manager::rotateLight(glm::radians(cl_globals::frametime * 15.0f), float3(1.0f, 0.0f, 0.0f));

        // This should be an easier way to exit
        // than breaking my fingers to do alt+f4
        if(input::isKeyJustPressed(GLFW_KEY_ESCAPE))
            break;

        player_look::update();
        player_move::update();

        proj_view::update();

        chunk_mesher::update();

        chunk_renderer::draw();

        composite::draw();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        debug_overlay::draw();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if(input::isKeyJustPressed(GLFW_KEY_F2))
            screenshots::jpeg(100);

        glfwSwapBuffers(cl_globals::window);

        input::update();

        glfwPollEvents();
        cl_globals::frame_count++;
    }

    composite::shutdown();

    chunk_mesher::shutdown();

    shadow_manager::shutdown();

    // We don't create it in main()
    // but we do destroy it here.
    cl_globals::solid_gbuffer.shutdown();

    cl_globals::solid_textures.destroy();

    cl_globals::registry.clear();

    chunk_renderer::shutdown();

    spdlog::info("Client shutdown after {} frames, avg. dt: {}", cl_globals::frame_count, cl_globals::avg_frametime);

    glfwDestroyWindow(cl_globals::window);
    glfwTerminate();
}
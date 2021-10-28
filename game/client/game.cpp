/*
 * Copyright (c) 2021, Kirill GPRB. All Rights Reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <common/math/const.hpp>
#include <common/math/math.hpp>
#include <exception>
#include <game/client/comp/camera.hpp>
#include <game/client/comp/local_player.hpp>
#include <game/client/sys/chunk_mesher.hpp>
#include <game/client/sys/chunk_renderer.hpp>
#include <game/client/sys/player_look.hpp>
#include <game/client/sys/player_move.hpp>
#include <game/client/sys/proj_view.hpp>
#include <game/client/util/screenshots.hpp>
#include <game/client/atlas.hpp>
#include <game/client/chunks.hpp>
#include <game/client/debug_overlay.hpp>
#include <game/client/deferred_pass.hpp>
#include <game/client/game.hpp>
#include <game/client/gamedata.hpp>
#include <game/client/gbuffer.hpp>
#include <game/client/globals.hpp>
#include <game/client/input.hpp>
#include <game/client/network_entities.hpp>
#include <game/client/screen.hpp>
#include <game/client/shadow_manager.hpp>
#include <game/shared/comp/creature.hpp>
#include <game/shared/comp/head.hpp>
#include <game/shared/comp/player.hpp>
#include <game/shared/protocol/packets/client/handshake.hpp>
#include <game/shared/protocol/packets/client/login_start.hpp>
#include <game/shared/protocol/packets/client/request_gamedata.hpp>
#include <game/shared/protocol/packets/server/login_success.hpp>
#include <game/shared/protocol/packets/shared/disconnect.hpp>
#include <game/shared/util/enet.hpp>
#include <game/shared/voxels.hpp>
#include <spdlog/spdlog.h>
#include <unordered_map>

// NOTENOTE: THESE THINGS SHOULD NOT BE HERE!!!!
// MOVE THIS TO THE WORLD HANDLING CODE ASAP!
#include <game/shared/protocol/packets/server/attach_creature.hpp>
#include <game/shared/protocol/packets/server/attach_head.hpp>
#include <game/shared/protocol/packets/server/attach_player.hpp>
#include <game/shared/protocol/packets/server/create_entity.hpp>

static void onDisconnect(const std::vector<uint8_t> &payload)
{
    protocol::packets::Disconnect packet;
    protocol::deserialize(payload, packet);

    spdlog::info("Disconnected: {}", packet.reason);

    enet_peer_disconnect(globals::peer, 0);

    globals::peer = nullptr;
    globals::session_id = 0;
    globals::state = ClientState::DISCONNECTED;
    globals::local_player = entt::null;

    globals::registry.clear();
}

static void onLoginSuccess(const std::vector<uint8_t> &payload)
{
    protocol::packets::LoginSuccess packet;
    protocol::deserialize(payload, packet);

    globals::state = ClientState::RECEIVING_GAME_DATA;
    globals::session_id = packet.session_id;

    spdlog::info("Logged in as {} with session_id={}", packet.username, globals::session_id);

    util::sendPacket(globals::peer, protocol::packets::RequestGamedata {}, 0, 0);
}

// NOTENOTE: THESE THINGS SHOULD NOT BE HERE!!!!
// MOVE THIS TO THE WORLD HANDLING CODE ASAP!

static void onAttachCreature(const std::vector<uint8_t> &payload)
{
    protocol::packets::AttachCreature packet;
    protocol::deserialize(payload, packet);

    entt::entity entity = network_entities::find(packet.network_id);
    if(entity != entt::null) {
        CreatureComponent &creature = globals::registry.emplace<CreatureComponent>(entity);
        creature.position = math::arrayToVec<float3>(packet.position);
        creature.yaw = packet.yaw;
    }
}

static void onAttachHead(const std::vector<uint8_t> &payload)
{
    protocol::packets::AttachHead packet;
    protocol::deserialize(payload, packet);

    entt::entity entity = network_entities::find(packet.network_id);
    if(entity != entt::null) {
        HeadComponent &head = globals::registry.emplace<HeadComponent>(entity);
        head.angles = math::arrayToVec<float3>(packet.angles);
        head.offset = math::arrayToVec<float3>(packet.offset);
    }
}

static void onAttachPlayer(const std::vector<uint8_t> &payload)
{
    protocol::packets::AttachPlayer packet;
    protocol::deserialize(payload, packet);

    entt::entity entity = network_entities::find(packet.network_id);
    if(entity != entt::null) {
        PlayerComponent &player = globals::registry.emplace<PlayerComponent>(entity);
        player.session_id = packet.session_id;

        if(packet.session_id == globals::session_id) {
            globals::registry.emplace<LocalPlayerComponent>(entity);
            globals::registry.emplace<ActiveCameraComponent>(entity);

            CameraComponent &camera = globals::registry.emplace<CameraComponent>(entity);
            camera.fov = glm::radians(90.0f);
            camera.z_near = 0.01f;
            camera.z_far = 1024.0f;

            globals::local_player = entity;
            globals::state = ClientState::PLAYING;
        }
    }
}

static void onCreateEntity(const std::vector<uint8_t> &payload)
{
    protocol::packets::CreateEntity packet;
    protocol::deserialize(payload, packet);
    network_entities::create(packet.network_id);
}


void cl_game::init()
{
    globals::packet_handlers[protocol::packets::Disconnect::id] = &onDisconnect;
    globals::packet_handlers[protocol::packets::LoginSuccess::id] = &onLoginSuccess;

    // NOTENOTE: THESE THINGS SHOULD NOT BE HERE!!!!
    // MOVE THIS TO THE WORLD HANDLING CODE ASAP!
    globals::packet_handlers[protocol::packets::AttachCreature::id] = &onAttachCreature;
    globals::packet_handlers[protocol::packets::AttachHead::id] = &onAttachHead;
    globals::packet_handlers[protocol::packets::AttachPlayer::id] = &onAttachPlayer;
    globals::packet_handlers[protocol::packets::CreateEntity::id] = &onCreateEntity;

    gamedata::init();

    chunk_renderer::init();
    deferred_pass::init();

    shadow_manager::init(8192, 8192);
    shadow_manager::setLightOrientation(floatquat(glm::radians(float3(45.0f, 0.0f, 45.0f))));
    shadow_manager::setPolygonOffset(float2(3.0f, 0.5f));
}

void cl_game::postInit()
{
    cl_game::connect("localhost", protocol::DEFAULT_PORT);
}

void cl_game::shutdown()
{
    cl_game::disconnect("cl_game::shutdown");

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

bool cl_game::connect(const std::string &host, uint16_t port)
{
    cl_game::disconnect("cl_game::connect");

    globals::registry.clear();

    ENetAddress address;
    address.port = port;
    if(enet_address_set_host(&address, host.c_str()) < 0) {
        spdlog::error("Unable to find {}:{}", host, port);
        return false;
    }

    globals::peer = enet_host_connect(globals::host, &address, 2, 0);
    if(!globals::peer) {
        spdlog::error("Unable to connect to {}:{}", host, port);
        return false;
    }

    ENetEvent event;
    while(enet_host_service(globals::host, &event, 5000) > 0) {
        if(event.type == ENET_EVENT_TYPE_CONNECT) {
            spdlog::debug("Logging in...");

            protocol::packets::Handshake handshake = {};
            util::sendPacket(globals::peer, handshake, 0, 0);

            // TODO: change-able username
            protocol::packets::LoginStart login = {};
            login.username = "Kirill";
            util::sendPacket(globals::peer, login, 0, 0);

            globals::state = ClientState::LOGGING_IN;
            return true;
        }

        if(event.type == ENET_EVENT_TYPE_RECEIVE) {
            enet_packet_destroy(event.packet);
            continue;
        }
    }

    enet_peer_reset(globals::peer);
    globals::peer = nullptr;

    spdlog::error("Unable to establish connection with {}:{}", host, port);
    return false;
}

void cl_game::disconnect(const std::string &reason)
{
    if(globals::peer) {
        // Send Disconnect
        protocol::packets::Disconnect packet;
        packet.reason = reason;
        util::sendPacket(globals::peer, packet, 0, 0);

        enet_peer_disconnect_later(globals::peer, 0);

        ENetEvent event;
        while(enet_host_service(globals::host, &event, 5000) > 0) {
            if(event.type == ENET_EVENT_TYPE_DISCONNECT) {
                spdlog::info("Disconnected.");
                break;
            }

            if(event.type == ENET_EVENT_TYPE_RECEIVE) {
                spdlog::debug("Unwanted (right now) packet with the size of {:.02f} KiB", static_cast<float>(event.packet->dataLength) / 1024.0f);
                enet_packet_destroy(event.packet);
                continue;
            }
        }

        enet_peer_reset(globals::peer);
        globals::peer = nullptr;
    }
}

void cl_game::update()
{
    ENetEvent event;
    while(enet_host_service(globals::host, &event, 0) > 0) {
        if(event.type == ENET_EVENT_TYPE_RECEIVE) {
            const std::vector<uint8_t> packet = std::vector<uint8_t>(event.packet->data, event.packet->data + event.packet->dataLength);
            enet_packet_destroy(event.packet);

            uint16_t packet_id;
            std::vector<uint8_t> payload;
            if(!protocol::split(packet, packet_id, payload)) {
                spdlog::warn("Invalid packet format!");
                continue;
            }

            const auto &it = globals::packet_handlers.find(packet_id);
            if(it == globals::packet_handlers.cend()) {
                spdlog::warn("Invalid packet 0x{:04X}", packet_id);
                continue;
            }

            it->second(payload);
        }
    }

    proj_view::update();

    bool is_playing = (globals::state == ClientState::PLAYING);
    if(is_playing) {
        // NOTENOTE: when the new chunks arrive (during the login stage
        // when clientside receives some important data like voxel info)
        // sometimes shit gets fucked and one side of a chunk becomes
        // visible. This is not a problem for now because these quads
        // are occluded by the actual geometry. Things will get hot only
        // when mesher will start to skip actual geometry leaving holes.
        chunk_mesher::update();

        player_look::update();
        player_move::update();
    }

    input::enableCursor(!is_playing);
}

void cl_game::draw()
{
    // Draw things to GBuffers
    chunk_renderer::draw();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    int width, height;
    screen::getSize(width, height);
    glViewport(0, 0, width, height);

    // Draw things to the main framebuffer
    deferred_pass::draw();
}

void cl_game::drawImgui()
{
    debug_overlay::draw();
}

void cl_game::postDraw()
{
    if(input::isKeyJustPressed(GLFW_KEY_F2))
        screenshots::jpeg(100);
}

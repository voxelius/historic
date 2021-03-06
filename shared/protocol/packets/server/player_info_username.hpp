/*
 * player_info_username.hpp
 * Copyright (c) 2021, Kirill GPRB.
 * All Rights Reserved.
 */
#pragma once
#include <shared/protocol/protocol.hpp>
#include <string>

namespace protocol::packets
{
struct PlayerInfoUsername final : public ServerPacket<0x007> {
    uint32_t session_id;
    std::string username;

    template<typename S>
    inline void serialize(S &s)
    {
        s.value4b(session_id);
        s.text1b(username, 63);
    }
};
} // namespace protocol::packets


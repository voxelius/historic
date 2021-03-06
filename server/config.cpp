/*
 * config.cpp
 * Author: Kirill GPRB
 * Created: Sun Dec 12 2021 18:47:06
 */
#include <common/math/math.hpp>
#include <shared/protocol/protocol.hpp>
#include <server/config.hpp>

void ServerConfig::implPostRead()
{
    simulation_distance = math::max(toml["simulation_distance"].value_or(4), 1);
    net.maxplayers = static_cast<size_t>(toml["net"]["maxplayers"].value_or<unsigned int>(16));
    net.port = toml["net"]["port"].value_or(protocol::DEFAULT_PORT);
}

void ServerConfig::implPreWrite()
{
    toml = toml::table {{
        { "simulation_distance", simulation_distance },
        { "net", toml::table {{
            { "maxplayers", static_cast<unsigned int>(net.maxplayers) },
            { "port", net.port }
        }}}
    }};
}

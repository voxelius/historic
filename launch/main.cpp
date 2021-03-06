/*
 * main.cpp
 * Copyright (c) 2021, Kirill GPRB.
 * All Rights Reserved.
 */
#include <common/filesystem.hpp>
#include <enet/enet.h>
#include <client/client_app.hpp>
#include <server/server_app.hpp>
#include <iostream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <common/util/spdlog.hpp>

int main(int argc, char **argv)
{
    try {
        spdlog::logger *lp = spdlog::default_logger_raw();
        lp->sinks().clear();
        lp->sinks().push_back(util::createSink<spdlog::sinks::stderr_color_sink_mt>());
        lp->set_pattern("[%H:%M:%S] %^[%L]%$ %v");
        lp->set_level(spdlog::level::trace);
    }
    catch(const spdlog::spdlog_ex &ex) {
        // Fallback to iostream
        std::cerr << ex.what() << std::endl;
        std::terminate();
    }

    fs::init();
    if(!fs::setRoot("game")) {
        spdlog::error("uh oh");
        std::terminate();
    }

    if(enet_initialize() < 0) {
        spdlog::error("Unable to initialize ENet.");
        std::terminate();
    }    

#if defined(VGAME_CLIENT)
    client_app::run();
#elif defined(VGAME_SERVER)
    server_app::run();
#else
    #error No side defined
#endif

    enet_deinitialize();

    fs::shutdown();

    return 0;
}


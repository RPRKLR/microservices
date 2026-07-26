#pragma once

#include <atomic>
#include <cstdint>
#include <thread>

#include <crow.h>

#include "store.hpp"

namespace telemetryd {

class HttpServer {
public:
    HttpServer(std::uint16_t port, Store& store, const std::atomic<bool>& ready);

    void start();
    void stop();

private:
    void setup_routes();

    std::uint16_t port_;
    Store& store_;
    const std::atomic<bool>& ready_;
    crow::SimpleApp app_;
    std::thread thread_;
};

} // namespace telemetryd

#pragma once

#include <atomic>
#include <cstdint>
#include <thread>

#include <crow.h>

#include "metrics.hpp"
#include "store.hpp"

namespace telemetryd {

class HttpServer {
public:
    HttpServer(std::uint16_t port, Store& store, const std::atomic<bool>& ready,
               Metrics& metrics);

    void start();
    void stop();

private:
    void setup_routes();
    crow::response handle_query(const crow::request& req);

    std::uint16_t port_;
    Store& store_;
    const std::atomic<bool>& ready_;
    Metrics& metrics_;
    crow::SimpleApp app_;
    std::thread thread_;
};

} // namespace telemetryd

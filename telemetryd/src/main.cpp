#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <string>
#include <thread>

#include <spdlog/spdlog.h>

#include "config.hpp"
#include "logging.hpp"

namespace {

// only async-signal-safe work in the handler: flip a flag, act on it in main
std::atomic<bool> g_shutdown_requested{false};

void handle_signal(int /*signum*/) {
    g_shutdown_requested.store(true);
}

} // namespace

int main(int argc, char* argv[]) {
    std::string config_path = "config/telemetryd.toml";
    if (argc > 1) {
        config_path = argv[1];
    }

    telemetryd::Config cfg;
    try {
        cfg = telemetryd::load_config(config_path);
    } catch (const std::exception& e) {
        // logging isn't up yet, stderr is all we have
        std::fprintf(stderr, "config error: %s\n", e.what());
        return 1;
    }

    telemetryd::init_logging(cfg.log_level, cfg.service_name);

    spdlog::info("starting {} v0.1.0", cfg.service_name);
    spdlog::info("effective config: {}", telemetryd::to_string(cfg));

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    // no servers yet, just prove the process lifecycle
    spdlog::info("up and idling, send SIGINT/SIGTERM to stop");
    while (!g_shutdown_requested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    spdlog::info("shutdown signal received, exiting");
    spdlog::shutdown();
    return 0;
}

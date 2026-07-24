#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include "config.hpp"
#include "ingest_service.hpp"
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

    telemetryd::IngestServiceImpl ingest_service;

    grpc::ServerBuilder builder;
    const auto addr = fmt::format("0.0.0.0:{}", cfg.grpc_port);
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&ingest_service);

    auto server = builder.BuildAndStart();
    if (!server) {
        spdlog::error("failed to start gRPC server on {}", addr);
        return 1;
    }
    spdlog::info("gRPC ingest listening on {}", addr);

    while (!g_shutdown_requested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    spdlog::info("shutdown signal received, draining for up to {}s", cfg.shutdown_drain_seconds);
    server->Shutdown(std::chrono::system_clock::now() +
                     std::chrono::seconds(cfg.shutdown_drain_seconds));
    server->Wait();

    spdlog::info("exiting, lifetime totals: accepted={} rejected={}",
                 ingest_service.total_accepted(), ingest_service.total_rejected());
    spdlog::shutdown();
    return 0;
}

#pragma once

#include <cstdint>
#include <string>

namespace telemetryd {

struct Config {
    std::string service_name = "telemetryd";
    std::string log_level = "info";

    std::uint16_t http_port = 8080;
    std::uint16_t grpc_port = 50051;

    std::int64_t store_max_samples_per_series = 10000;
    std::int64_t store_retention_seconds = 3600;

    std::int64_t shutdown_drain_seconds = 5;
};

// defaults -> config file (optional) -> env vars; throws on invalid values
Config load_config(const std::string& file_path);

std::string to_string(const Config& cfg);

} // namespace telemetryd

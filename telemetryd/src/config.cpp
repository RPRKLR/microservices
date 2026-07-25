#include "config.hpp"

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <set>
#include <stdexcept>

#include <spdlog/fmt/fmt.h>
#include <toml++/toml.hpp>

namespace telemetryd {

namespace {

std::optional<std::string> get_env(const char* name) {
    const char* v = std::getenv(name);
    if (v == nullptr || *v == '\0') {
        return std::nullopt;
    }
    return std::string(v);
}

std::int64_t parse_int(const std::string& value, const std::string& what) {
    try {
        return std::stoll(value);
    } catch (const std::exception&) {
        throw std::runtime_error(fmt::format("{}: expected an integer, got '{}'", what, value));
    }
}

std::uint16_t parse_port(const std::string& value, const std::string& what) {
    const auto n = parse_int(value, what);
    if (n < 1 || n > 65535) {
        throw std::runtime_error(fmt::format("{}: port out of range: {}", what, n));
    }
    return static_cast<std::uint16_t>(n);
}

void apply_file(Config& cfg, const std::string& path) {
    toml::table tbl;
    try {
        tbl = toml::parse_file(path);
    } catch (const toml::parse_error& err) {
        throw std::runtime_error(
            fmt::format("failed to parse config file '{}': {}", path, err.description()));
    }

    cfg.service_name = tbl["service"]["name"].value_or(cfg.service_name);
    cfg.log_level = tbl["service"]["log_level"].value_or(cfg.log_level);

    cfg.http_port = tbl["server"]["http_port"].value_or(cfg.http_port);
    cfg.grpc_port = tbl["server"]["grpc_port"].value_or(cfg.grpc_port);
    cfg.shutdown_drain_seconds =
        tbl["server"]["shutdown_drain_seconds"].value_or(cfg.shutdown_drain_seconds);

    cfg.store_max_samples_per_series =
        tbl["store"]["max_samples_per_series"].value_or(cfg.store_max_samples_per_series);
    cfg.store_max_series = tbl["store"]["max_series"].value_or(cfg.store_max_series);
    cfg.store_retention_seconds =
        tbl["store"]["retention_seconds"].value_or(cfg.store_retention_seconds);
}

void apply_env(Config& cfg) {
    if (auto v = get_env("TELEMETRYD_LOG_LEVEL")) {
        cfg.log_level = *v;
    }
    if (auto v = get_env("TELEMETRYD_HTTP_PORT")) {
        cfg.http_port = parse_port(*v, "TELEMETRYD_HTTP_PORT");
    }
    if (auto v = get_env("TELEMETRYD_GRPC_PORT")) {
        cfg.grpc_port = parse_port(*v, "TELEMETRYD_GRPC_PORT");
    }
    if (auto v = get_env("TELEMETRYD_SHUTDOWN_DRAIN_SECONDS")) {
        cfg.shutdown_drain_seconds = parse_int(*v, "TELEMETRYD_SHUTDOWN_DRAIN_SECONDS");
    }
    if (auto v = get_env("TELEMETRYD_STORE_MAX_SAMPLES_PER_SERIES")) {
        cfg.store_max_samples_per_series =
            parse_int(*v, "TELEMETRYD_STORE_MAX_SAMPLES_PER_SERIES");
    }
    if (auto v = get_env("TELEMETRYD_STORE_MAX_SERIES")) {
        cfg.store_max_series = parse_int(*v, "TELEMETRYD_STORE_MAX_SERIES");
    }
    if (auto v = get_env("TELEMETRYD_STORE_RETENTION_SECONDS")) {
        cfg.store_retention_seconds = parse_int(*v, "TELEMETRYD_STORE_RETENTION_SECONDS");
    }
}

void validate(const Config& cfg) {
    static const std::set<std::string> levels = {"trace", "debug", "info", "warn", "error"};
    if (levels.count(cfg.log_level) == 0) {
        throw std::runtime_error(fmt::format(
            "log_level must be one of trace/debug/info/warn/error, got '{}'", cfg.log_level));
    }
    if (cfg.http_port == cfg.grpc_port) {
        throw std::runtime_error(
            fmt::format("http_port and grpc_port must differ (both {})", cfg.http_port));
    }
    if (cfg.store_max_samples_per_series <= 0) {
        throw std::runtime_error("store.max_samples_per_series must be positive");
    }
    if (cfg.store_max_series <= 0) {
        throw std::runtime_error("store.max_series must be positive");
    }
    if (cfg.store_retention_seconds <= 0) {
        throw std::runtime_error("store.retention_seconds must be positive");
    }
    if (cfg.shutdown_drain_seconds < 0 || cfg.shutdown_drain_seconds > 60) {
        throw std::runtime_error(fmt::format(
            "shutdown_drain_seconds must be between 0 and 60, got {}", cfg.shutdown_drain_seconds));
    }
}

} // namespace

Config load_config(const std::string& file_path) {
    Config cfg;

    if (std::filesystem::exists(file_path)) {
        apply_file(cfg, file_path);
    }
    apply_env(cfg);
    validate(cfg);
    return cfg;
}

std::string to_string(const Config& cfg) {
    return fmt::format(
        "service_name={} log_level={} http_port={} grpc_port={} "
        "store_max_samples_per_series={} store_max_series={} store_retention_seconds={} "
        "shutdown_drain_seconds={}",
        cfg.service_name, cfg.log_level, cfg.http_port, cfg.grpc_port,
        cfg.store_max_samples_per_series, cfg.store_max_series, cfg.store_retention_seconds,
        cfg.shutdown_drain_seconds);
}

} // namespace telemetryd

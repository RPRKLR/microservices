#include "logging.hpp"

#include <memory>

#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

namespace telemetryd {

void init_logging(const std::string& level, const std::string& service_name) {
    auto sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("telemetryd", sink);

    // pattern-based JSON: breaks if the message contains a double quote
    logger->set_pattern(fmt::format(
        R"({{"ts":"%Y-%m-%dT%H:%M:%S.%e%z","level":"%l","service":"{}","msg":"%v"}})",
        service_name));

    logger->set_level(spdlog::level::from_str(level));
    logger->flush_on(spdlog::level::warn);

    spdlog::set_default_logger(std::move(logger));
}

} // namespace telemetryd

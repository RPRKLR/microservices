#pragma once

#include <string>

namespace telemetryd {

// one JSON object per line on stdout; call once at startup
void init_logging(const std::string& level, const std::string& service_name);

} // namespace telemetryd

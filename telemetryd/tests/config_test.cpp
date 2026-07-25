#include "config.hpp"

#include <cstdlib>
#include <stdexcept>

#include <gtest/gtest.h>

namespace {

void set_env(const char* name, const char* value) {
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

} // namespace

TEST(ConfigTest, DefaultsWhenNoFile) {
    const auto cfg = telemetryd::load_config("does_not_exist.toml");
    EXPECT_EQ(cfg.service_name, "telemetryd");
    EXPECT_EQ(cfg.http_port, 8080);
    EXPECT_EQ(cfg.grpc_port, 50051);
}

TEST(ConfigTest, EnvOverridesDefault) {
    set_env("TELEMETRYD_HTTP_PORT", "9191");
    const auto cfg = telemetryd::load_config("does_not_exist.toml");
    EXPECT_EQ(cfg.http_port, 9191);
    set_env("TELEMETRYD_HTTP_PORT", "");
}

TEST(ConfigTest, InvalidPortThrows) {
    set_env("TELEMETRYD_HTTP_PORT", "70000");
    EXPECT_THROW(telemetryd::load_config("does_not_exist.toml"), std::runtime_error);
    set_env("TELEMETRYD_HTTP_PORT", "");
}

TEST(ConfigTest, InvalidLogLevelThrows) {
    set_env("TELEMETRYD_LOG_LEVEL", "verbose");
    EXPECT_THROW(telemetryd::load_config("does_not_exist.toml"), std::runtime_error);
    set_env("TELEMETRYD_LOG_LEVEL", "");
}

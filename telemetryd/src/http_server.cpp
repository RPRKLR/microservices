#include "http_server.hpp"

#include <limits>
#include <stdexcept>
#include <string>

#include <spdlog/spdlog.h>

namespace telemetryd {

namespace {

std::int64_t param_or(const crow::request& req, const char* name, std::int64_t fallback) {
    const char* v = req.url_params.get(name);
    if (v == nullptr) {
        return fallback;
    }
    try {
        return std::stoll(v);
    } catch (const std::exception&) {
        throw std::invalid_argument(std::string(name) + " must be an integer");
    }
}

} // namespace

HttpServer::HttpServer(std::uint16_t port, Store& store, const std::atomic<bool>& ready)
    : port_(port), store_(store), ready_(ready) {
    setup_routes();
}

void HttpServer::setup_routes() {
    // liveness: process is up, deliberately checks nothing else
    CROW_ROUTE(app_, "/healthz")([] { return crow::response(200, "ok"); });

    // readiness: flips to 503 during shutdown drain
    CROW_ROUTE(app_, "/readyz")([this] {
        return ready_.load() ? crow::response(200, "ok") : crow::response(503, "draining");
    });

    CROW_ROUTE(app_, "/api/v1/query")([this](const crow::request& req) {
        const char* robot = req.url_params.get("robot");
        const char* metric = req.url_params.get("metric");
        if (robot == nullptr || metric == nullptr) {
            return crow::response(400, "robot and metric query params are required");
        }

        std::int64_t from_ms, to_ms, max_points;
        try {
            from_ms = param_or(req, "from_ms", 0);
            to_ms = param_or(req, "to_ms", std::numeric_limits<std::int64_t>::max());
            max_points = param_or(req, "max_points", 0);
        } catch (const std::invalid_argument& e) {
            return crow::response(400, e.what());
        }

        auto samples = store_.query(robot, metric, from_ms, to_ms);
        if (max_points > 0) {
            samples = downsample(samples, static_cast<std::size_t>(max_points));
        }

        crow::json::wvalue body;
        body["robot"] = robot;
        body["metric"] = metric;
        body["count"] = samples.size();
        crow::json::wvalue::list points;
        points.reserve(samples.size());
        for (const auto& p : samples) {
            crow::json::wvalue o;
            o["ts"] = p.ts_unix_ms;
            o["value"] = p.value;
            points.push_back(std::move(o));
        }
        body["samples"] = std::move(points);
        return crow::response(body);
    });
}

void HttpServer::start() {
    app_.port(port_).loglevel(crow::LogLevel::Warning).multithreaded();
    thread_ = std::thread([this] { app_.run(); });
    spdlog::info("http listening on 0.0.0.0:{}", port_);
}

void HttpServer::stop() {
    app_.stop();
    if (thread_.joinable()) {
        thread_.join();
    }
}

} // namespace telemetryd

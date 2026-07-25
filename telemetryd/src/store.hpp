#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace telemetryd {

struct SamplePoint {
    std::int64_t ts_unix_ms;
    double value;
};

struct StoreLimits {
    std::size_t max_series;
    std::size_t max_samples_per_series;
    std::int64_t retention_ms;
};

class Store {
public:
    enum class PushResult { ok, series_limit };

    explicit Store(StoreLimits limits);

    // assumes samples arrive roughly in ts order per series; retention is
    // relative to the newest ts seen in the series, not wall clock
    PushResult push(const std::string& robot_id, const std::string& metric, SamplePoint p);

    std::vector<SamplePoint> query(const std::string& robot_id, const std::string& metric,
                                   std::int64_t from_ms, std::int64_t to_ms) const;

    std::size_t series_count() const;

private:
    struct Series {
        explicit Series(std::size_t capacity) : ring(capacity) {}

        mutable std::mutex mu;
        std::vector<SamplePoint> ring;
        std::size_t start = 0;
        std::size_t size = 0;
        std::int64_t newest_ts = 0;
    };

    static std::string key(const std::string& robot_id, const std::string& metric);
    void push_locked(Series& s, SamplePoint p) const;

    StoreLimits limits_;
    mutable std::shared_mutex map_mu_;
    std::unordered_map<std::string, std::unique_ptr<Series>> series_;
};

} // namespace telemetryd

#include "store.hpp"

#include <algorithm>

namespace telemetryd {

Store::Store(StoreLimits limits) : limits_(limits) {}

std::string Store::key(const std::string& robot_id, const std::string& metric) {
    return robot_id + '\n' + metric;
}

void Store::push_locked(Series& s, SamplePoint p) const {
    s.ring[(s.start + s.size) % s.ring.size()] = p;
    if (s.size < s.ring.size()) {
        ++s.size;
    } else {
        s.start = (s.start + 1) % s.ring.size();
    }
    s.newest_ts = std::max(s.newest_ts, p.ts_unix_ms);

    const std::int64_t cutoff = s.newest_ts - limits_.retention_ms;
    while (s.size > 0 && s.ring[s.start].ts_unix_ms < cutoff) {
        s.start = (s.start + 1) % s.ring.size();
        --s.size;
    }
}

Store::PushResult Store::push(const std::string& robot_id, const std::string& metric,
                              SamplePoint p) {
    const auto k = key(robot_id, metric);

    {
        std::shared_lock lock(map_mu_);
        auto it = series_.find(k);
        if (it != series_.end()) {
            std::lock_guard series_lock(it->second->mu);
            push_locked(*it->second, p);
            return PushResult::ok;
        }
    }

    std::unique_lock lock(map_mu_);
    auto it = series_.find(k);
    if (it == series_.end()) {
        if (series_.size() >= limits_.max_series) {
            return PushResult::series_limit;
        }
        it = series_.emplace(k, std::make_unique<Series>(limits_.max_samples_per_series)).first;
    }
    std::lock_guard series_lock(it->second->mu);
    push_locked(*it->second, p);
    return PushResult::ok;
}

std::vector<SamplePoint> Store::query(const std::string& robot_id, const std::string& metric,
                                      std::int64_t from_ms, std::int64_t to_ms) const {
    std::vector<SamplePoint> out;

    std::shared_lock lock(map_mu_);
    auto it = series_.find(key(robot_id, metric));
    if (it == series_.end()) {
        return out;
    }

    const Series& s = *it->second;
    std::lock_guard series_lock(s.mu);
    for (std::size_t i = 0; i < s.size; ++i) {
        const auto& p = s.ring[(s.start + i) % s.ring.size()];
        if (p.ts_unix_ms >= from_ms && p.ts_unix_ms <= to_ms) {
            out.push_back(p);
        }
    }
    return out;
}

std::size_t Store::series_count() const {
    std::shared_lock lock(map_mu_);
    return series_.size();
}

} // namespace telemetryd

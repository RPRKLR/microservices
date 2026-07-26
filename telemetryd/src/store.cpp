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

std::vector<SamplePoint> downsample(const std::vector<SamplePoint>& in, std::size_t max_points) {
    if (max_points == 0 || in.size() <= max_points) {
        return in;
    }

    std::vector<SamplePoint> out;
    out.reserve(max_points);
    const double stride = static_cast<double>(in.size()) / static_cast<double>(max_points);

    for (std::size_t b = 0; b < max_points; ++b) {
        auto begin = static_cast<std::size_t>(static_cast<double>(b) * stride);
        auto end = std::min(in.size(), static_cast<std::size_t>(static_cast<double>(b + 1) * stride));
        if (end <= begin) {
            end = begin + 1;
        }

        double ts_sum = 0.0;
        double value_sum = 0.0;
        for (std::size_t i = begin; i < end; ++i) {
            ts_sum += static_cast<double>(in[i].ts_unix_ms);
            value_sum += in[i].value;
        }
        const auto n = static_cast<double>(end - begin);
        out.push_back({static_cast<std::int64_t>(ts_sum / n), value_sum / n});
    }
    return out;
}

} // namespace telemetryd

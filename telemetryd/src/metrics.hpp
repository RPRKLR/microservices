#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>

namespace telemetryd {

class Metrics {
public:
    Metrics();

    void count_http_request(const std::string& endpoint, int status, double seconds);
    void count_ingest_stream(double seconds);
    void count_samples(const std::string& result, double n);
    void set_store_gauges(std::size_t series, std::size_t samples);

    std::string serialize() const;

private:
    std::shared_ptr<prometheus::Registry> registry_;
    prometheus::Family<prometheus::Counter>* http_requests_;
    prometheus::Family<prometheus::Histogram>* http_duration_;
    prometheus::Family<prometheus::Counter>* samples_;
    prometheus::Counter* streams_;
    prometheus::Histogram* stream_duration_;
    prometheus::Gauge* store_series_;
    prometheus::Gauge* store_samples_;
};

} // namespace telemetryd

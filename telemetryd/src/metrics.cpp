#include "metrics.hpp"

#include <prometheus/text_serializer.h>

namespace telemetryd {

namespace {

const prometheus::Histogram::BucketBoundaries kHttpBuckets = {0.001, 0.005, 0.01, 0.05,
                                                              0.1,   0.5,   1.0,  5.0};
const prometheus::Histogram::BucketBoundaries kStreamBuckets = {0.01, 0.1, 1.0, 10.0, 60.0};

} // namespace

Metrics::Metrics() : registry_(std::make_shared<prometheus::Registry>()) {
    http_requests_ = &prometheus::BuildCounter()
                          .Name("http_requests_total")
                          .Help("HTTP requests by endpoint and status")
                          .Register(*registry_);
    http_duration_ = &prometheus::BuildHistogram()
                          .Name("http_request_duration_seconds")
                          .Help("HTTP request duration by endpoint")
                          .Register(*registry_);

    samples_ = &prometheus::BuildCounter()
                    .Name("ingest_samples_total")
                    .Help("Ingested samples by result (accepted/invalid/store_full)")
                    .Register(*registry_);
    streams_ = &prometheus::BuildCounter()
                    .Name("ingest_streams_total")
                    .Help("Completed ingest streams")
                    .Register(*registry_)
                    .Add({});
    stream_duration_ = &prometheus::BuildHistogram()
                            .Name("ingest_stream_duration_seconds")
                            .Help("Ingest stream duration")
                            .Register(*registry_)
                            .Add({}, kStreamBuckets);

    auto& gauges = prometheus::BuildGauge()
                       .Name("store_series")
                       .Help("Number of series currently in the store")
                       .Register(*registry_);
    store_series_ = &gauges.Add({});
    store_samples_ = &prometheus::BuildGauge()
                          .Name("store_samples")
                          .Help("Number of samples currently in the store")
                          .Register(*registry_)
                          .Add({});
}

void Metrics::count_http_request(const std::string& endpoint, int status, double seconds) {
    http_requests_->Add({{"endpoint", endpoint}, {"status", std::to_string(status)}}).Increment();
    http_duration_->Add({{"endpoint", endpoint}}, kHttpBuckets).Observe(seconds);
}

void Metrics::count_ingest_stream(double seconds) {
    streams_->Increment();
    stream_duration_->Observe(seconds);
}

void Metrics::count_samples(const std::string& result, double n) {
    if (n > 0) {
        samples_->Add({{"result", result}}).Increment(n);
    }
}

void Metrics::set_store_gauges(std::size_t series, std::size_t samples) {
    store_series_->Set(static_cast<double>(series));
    store_samples_->Set(static_cast<double>(samples));
}

std::string Metrics::serialize() const {
    return prometheus::TextSerializer().Serialize(registry_->Collect());
}

} // namespace telemetryd

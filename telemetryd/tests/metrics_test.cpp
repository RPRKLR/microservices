#include "metrics.hpp"

#include <gtest/gtest.h>

// labelled families only serialize once they have a child, so unlabelled
// ones must be present from the start and labelled ones after first use
TEST(MetricsTest, UnlabelledFamiliesPresentFromStart) {
    telemetryd::Metrics m;
    const auto text = m.serialize();
    EXPECT_NE(text.find("ingest_streams_total"), std::string::npos);
    EXPECT_NE(text.find("store_series"), std::string::npos);
    EXPECT_NE(text.find("store_samples"), std::string::npos);
}

TEST(MetricsTest, CountersShowUpWithLabels) {
    telemetryd::Metrics m;
    m.count_http_request("/api/v1/query", 200, 0.01);
    m.count_http_request("/api/v1/query", 200, 0.02);
    m.count_http_request("/api/v1/query", 400, 0.001);
    m.count_samples("accepted", 42);
    m.set_store_gauges(3, 1234);

    const auto text = m.serialize();
    EXPECT_NE(text.find("http_requests_total{endpoint=\"/api/v1/query\",status=\"200\"} 2"),
              std::string::npos);
    EXPECT_NE(text.find("http_requests_total{endpoint=\"/api/v1/query\",status=\"400\"} 1"),
              std::string::npos);
    EXPECT_NE(text.find("ingest_samples_total{result=\"accepted\"} 42"), std::string::npos);
    EXPECT_NE(text.find("store_samples 1234"), std::string::npos);
}

TEST(MetricsTest, ZeroSampleCountNotEmitted) {
    telemetryd::Metrics m;
    m.count_samples("invalid", 0);
    EXPECT_EQ(m.serialize().find("result=\"invalid\""), std::string::npos);
}

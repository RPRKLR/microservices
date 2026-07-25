#include "store.hpp"

#include <gtest/gtest.h>

using telemetryd::SamplePoint;
using telemetryd::Store;
using telemetryd::StoreLimits;

namespace {

Store make_store(std::size_t max_series = 100, std::size_t capacity = 10,
                 std::int64_t retention_ms = 1000000) {
    return Store(StoreLimits{max_series, capacity, retention_ms});
}

} // namespace

TEST(StoreTest, PushThenQueryReturnsSamplesInRange) {
    auto store = make_store();
    for (std::int64_t ts = 100; ts <= 500; ts += 100) {
        ASSERT_EQ(store.push("r1", "temp", {ts, double(ts)}), Store::PushResult::ok);
    }

    const auto all = store.query("r1", "temp", 0, 1000);
    ASSERT_EQ(all.size(), 5u);
    EXPECT_EQ(all.front().ts_unix_ms, 100);
    EXPECT_EQ(all.back().ts_unix_ms, 500);

    const auto mid = store.query("r1", "temp", 200, 400);
    ASSERT_EQ(mid.size(), 3u);
    EXPECT_EQ(mid.front().ts_unix_ms, 200);
    EXPECT_EQ(mid.back().ts_unix_ms, 400);
}

TEST(StoreTest, UnknownSeriesReturnsEmpty) {
    auto store = make_store();
    EXPECT_TRUE(store.query("nope", "nothing", 0, 1000).empty());
}

TEST(StoreTest, CapacityBoundEvictsOldest) {
    auto store = make_store(100, 3);
    for (std::int64_t ts = 1; ts <= 5; ++ts) {
        store.push("r1", "temp", {ts, 0.0});
    }

    const auto all = store.query("r1", "temp", 0, 100);
    ASSERT_EQ(all.size(), 3u);
    EXPECT_EQ(all.front().ts_unix_ms, 3);
    EXPECT_EQ(all.back().ts_unix_ms, 5);
}

TEST(StoreTest, RetentionTrimsOldSamples) {
    auto store = make_store(100, 10, 100);
    store.push("r1", "temp", {1000, 1.0});
    store.push("r1", "temp", {1050, 2.0});
    store.push("r1", "temp", {1200, 3.0});

    const auto all = store.query("r1", "temp", 0, 10000);
    ASSERT_EQ(all.size(), 1u);
    EXPECT_EQ(all.front().ts_unix_ms, 1200);
}

TEST(StoreTest, SeriesLimitRejectsNewSeries) {
    auto store = make_store(2);
    EXPECT_EQ(store.push("r1", "a", {1, 0.0}), Store::PushResult::ok);
    EXPECT_EQ(store.push("r1", "b", {1, 0.0}), Store::PushResult::ok);
    EXPECT_EQ(store.push("r1", "c", {1, 0.0}), Store::PushResult::series_limit);
    EXPECT_EQ(store.push("r1", "a", {2, 0.0}), Store::PushResult::ok);
    EXPECT_EQ(store.series_count(), 2u);
}

TEST(StoreTest, SeriesAreIndependent) {
    auto store = make_store();
    store.push("r1", "temp", {100, 1.0});
    store.push("r2", "temp", {100, 2.0});
    store.push("r1", "volt", {100, 3.0});

    EXPECT_EQ(store.series_count(), 3u);
    const auto r1_temp = store.query("r1", "temp", 0, 1000);
    ASSERT_EQ(r1_temp.size(), 1u);
    EXPECT_DOUBLE_EQ(r1_temp[0].value, 1.0);
}

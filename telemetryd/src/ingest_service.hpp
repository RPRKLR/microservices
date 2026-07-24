#pragma once

#include <atomic>
#include <cstdint>

#include "telemetry.grpc.pb.h"

namespace telemetryd {

class IngestServiceImpl final : public v1::TelemetryIngest::Service {
public:
    grpc::Status StreamTelemetry(grpc::ServerContext* ctx,
                                 grpc::ServerReader<v1::TelemetryBatch>* reader,
                                 v1::IngestSummary* summary) override;

    std::uint64_t total_accepted() const { return total_accepted_.load(); }
    std::uint64_t total_rejected() const { return total_rejected_.load(); }

private:
    std::atomic<std::uint64_t> total_accepted_{0};
    std::atomic<std::uint64_t> total_rejected_{0};
};

} // namespace telemetryd

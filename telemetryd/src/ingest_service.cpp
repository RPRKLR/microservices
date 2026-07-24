#include "ingest_service.hpp"

#include <spdlog/spdlog.h>

namespace telemetryd {

namespace {

bool valid(const v1::Sample& s) {
    return !s.robot_id().empty() && !s.metric().empty() && s.ts_unix_ms() > 0;
}

} // namespace

grpc::Status IngestServiceImpl::StreamTelemetry(grpc::ServerContext* ctx,
                                                grpc::ServerReader<v1::TelemetryBatch>* reader,
                                                v1::IngestSummary* summary) {
    std::uint64_t accepted = 0;
    std::uint64_t rejected = 0;

    v1::TelemetryBatch batch;
    while (reader->Read(&batch)) {
        if (ctx->IsCancelled()) {
            spdlog::warn("ingest stream cancelled by peer {}", ctx->peer());
            return grpc::Status::CANCELLED;
        }
        for (const auto& sample : batch.samples()) {
            if (valid(sample)) {
                // TODO: hand off to the store (M3), counting only for now
                ++accepted;
            } else {
                ++rejected;
            }
        }
        spdlog::debug("batch from {}: {} samples", ctx->peer(), batch.samples_size());
    }

    total_accepted_.fetch_add(accepted);
    total_rejected_.fetch_add(rejected);

    summary->set_accepted(accepted);
    summary->set_rejected(rejected);
    spdlog::info("stream from {} closed: accepted={} rejected={}", ctx->peer(), accepted, rejected);
    return grpc::Status::OK;
}

} // namespace telemetryd

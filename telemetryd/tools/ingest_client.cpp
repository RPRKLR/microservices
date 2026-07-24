// throwaway load/test client: ingest_client <host:port> [batches] [samples_per_batch]
#include <chrono>
#include <cstdio>
#include <string>

#include <grpcpp/grpcpp.h>

#include "telemetry.grpc.pb.h"

int main(int argc, char* argv[]) {
    const std::string target = argc > 1 ? argv[1] : "localhost:50051";
    const int batches = argc > 2 ? std::stoi(argv[2]) : 10;
    const int per_batch = argc > 3 ? std::stoi(argv[3]) : 100;

    auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    auto stub = telemetryd::v1::TelemetryIngest::NewStub(channel);

    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(30));

    telemetryd::v1::IngestSummary summary;
    auto writer = stub->StreamTelemetry(&ctx, &summary);

    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();

    for (int b = 0; b < batches; ++b) {
        telemetryd::v1::TelemetryBatch batch;
        for (int i = 0; i < per_batch; ++i) {
            auto* s = batch.add_samples();
            s->set_robot_id("robot-" + std::to_string(i % 5));
            s->set_metric("battery_voltage");
            s->set_ts_unix_ms(now_ms + b * per_batch + i);
            s->set_value(48.0 + (i % 10) * 0.1);
        }
        if (!writer->Write(batch)) {
            std::fprintf(stderr, "write failed at batch %d\n", b);
            break;
        }
    }

    writer->WritesDone();
    const grpc::Status status = writer->Finish();
    if (!status.ok()) {
        std::fprintf(stderr, "stream failed: %s\n", status.error_message().c_str());
        return 1;
    }

    std::printf("sent %d batches, server accepted=%llu rejected=%llu\n", batches,
                static_cast<unsigned long long>(summary.accepted()),
                static_cast<unsigned long long>(summary.rejected()));
    return 0;
}

# telemetryd

Telemetry ingestion and query service for a robot fleet, written in modern C++.
Ingests high-rate structured telemetry over gRPC, keeps a bounded in-memory
time-series store, and serves queries over HTTP.

**Status: in progress** — config layering, structured JSON logging, clean
process lifecycle and the gRPC streaming ingest path are in place. The
time-series store is next.

## Build

Requires CMake ≥ 3.25 and [vcpkg](https://vcpkg.io) with `VCPKG_ROOT` set.

```
cmake --preset default
cmake --build --preset default
```

## Run

```
./build/telemetryd [path/to/config.toml]
```

Defaults to `config/telemetryd.toml` if no path is given. The file is optional;
every setting has a default and can be overridden with `TELEMETRYD_*`
environment variables (e.g. `TELEMETRYD_LOG_LEVEL=debug`). Invalid config fails
loudly at startup and the effective config is logged on boot.

## Roadmap

- [x] Config layering (defaults → file → env) with validation
- [x] Structured JSON logging
- [x] Graceful exit on SIGINT/SIGTERM
- [x] gRPC client-streaming ingest (with `tools/ingest_client` for manual testing)
- [ ] In-memory time-series store with bounded retention
- [ ] HTTP query API
- [ ] Prometheus metrics, `/healthz` + `/readyz`
- [ ] Dockerfile + docker-compose stack (Prometheus, Grafana)
- [ ] Integration tests in CI, load test with published numbers

## Non-goals

Authentication, persistence to disk, clustering.

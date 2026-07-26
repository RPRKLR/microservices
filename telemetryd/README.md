# telemetryd

Telemetry ingestion and query service for a robot fleet, written in modern C++.
Ingests high-rate structured telemetry over gRPC, keeps a bounded in-memory
time-series store, and serves queries over HTTP.

**Status: in progress** — config layering, structured JSON logging, gRPC
streaming ingest, the bounded in-memory store, the HTTP query API and
liveness/readiness endpoints are in place. Prometheus metrics are next.

## Query API

```
GET /api/v1/query?robot=robot-1&metric=battery_voltage&from_ms=0&to_ms=...&max_points=500
```

Returns `{robot, metric, count, samples: [{ts, value}]}`. `from_ms`/`to_ms`
default to everything retained; `max_points` enables bucket-mean downsampling.
`/healthz` is liveness (process up, checks nothing else), `/readyz` is
readiness and returns 503 during the shutdown drain window.

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
- [x] In-memory time-series store with bounded retention (ring buffer per
      series, series-count cap, unit tested)
- [x] HTTP query API (Crow) with bucket-mean downsampling
- [x] `/healthz` + `/readyz`, readiness-aware graceful drain
- [ ] Prometheus metrics
- [ ] Dockerfile + docker-compose stack (Prometheus, Grafana)
- [ ] Integration tests in CI, load test with published numbers

## Non-goals

Authentication, persistence to disk, clustering.

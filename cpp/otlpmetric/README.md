# OpenTelemetry C++ OTLP gRPC Metric Export Sample

This sample demonstrates how to export OpenTelemetry metrics directly from a C++ application to Google Cloud Monitoring using the standard OTLP gRPC exporter with Google Application Default Credentials (ADC) and automatic dynamic token refresh.

## Prerequisites

- CMake 3.20+
- C++17 compatible compiler (GCC 9+, Clang 10+)
- `gRPC` C++ development libraries (`libgrpc++-dev`, `protobuf-compiler-grpc`)
- `Protobuf` development libraries (`libprotobuf-dev`, `protobuf-compiler`)
- `opentelemetry-cpp` v1.28.0 (automatically downloaded via CMake `FetchContent`)

## Permissions

Grant the `roles/monitoring.metricWriter` IAM role to your account or service account:
https://cloud.google.com/monitoring/access-control#roles_descriptions

## Running Locally

1. Authenticate with Google Cloud:
   ```bash
   gcloud auth application-default login
   ```

2. Export configuration:
   ```bash
   export GOOGLE_CLOUD_PROJECT="your-project-id"
   export OTEL_EXPORTER_OTLP_HEADERS="X-Goog-User-Project=$GOOGLE_CLOUD_PROJECT"
   export OTEL_EXPORTER_OTLP_METRICS_ENDPOINT="telemetry.googleapis.com:443"
   ```

3. Build and run:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   ./build/otlpmetric
   ```

## Viewing Metrics

View generated metrics in Google Cloud Console under [Metrics Explorer](https://console.cloud.google.com/monitoring/metrics-explorer).

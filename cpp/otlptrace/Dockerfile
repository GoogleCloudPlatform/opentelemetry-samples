FROM ubuntu:24.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler \
    protobuf-compiler-grpc \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_OTLP_GRPC_CREDENTIAL_PREVIEW=ON && cmake --build build --parallel && ctest --test-dir build --output-on-failure

FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    libgrpc++1.51t64 \
    libprotobuf32t64 \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /src/build/ /app/
RUN if [ -d /app/_deps/opentelemetry_cpp-build ]; then find /app -name "*.so*" -exec cp {} /usr/local/lib/ \; 2>/dev/null || true; ldconfig; fi
CMD ["/app/otlptrace"]

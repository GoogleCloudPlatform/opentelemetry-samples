# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# [START opentelemetry_instrumentation_agent_dockerfile]
FROM ubuntu:24.04 as builder
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    libssl-dev \
    libprotobuf-dev \
    protobuf-compiler \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel && ctest --test-dir build --output-on-failure

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    libcurl4 \
    libprotobuf32t64 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && mkdir -p /var/log

WORKDIR /app
COPY --from=builder /src/build/app /app/app

CMD ["/bin/sh", "-c", "/app/app 2>&1 | tee /var/log/app.log"]
# [END opentelemetry_instrumentation_agent_dockerfile]

// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "http_instrumentation.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "opentelemetry/context/propagation/global_propagator.h"
#include "opentelemetry/context/runtime_context.h"
#include "opentelemetry/metrics/provider.h"
#include "opentelemetry/nostd/variant.h"
#include "opentelemetry/sdk/metrics/meter_provider.h"
#include "opentelemetry/sdk/trace/exporter.h"
#include "opentelemetry/sdk/trace/simple_processor.h"
#include "opentelemetry/sdk/trace/span_data.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/trace/propagation/http_trace_context.h"
#include "opentelemetry/trace/provider.h"

#define ASSERT_TRUE(cond)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "Assertion failed: " #cond " at " << __FILE__ << ":"        \
                << __LINE__ << std::endl;                                      \
      std::exit(1);                                                            \
    }                                                                          \
  } while (0)

#define ASSERT_EQ(a, b)                                                        \
  do {                                                                         \
    if (!((a) == (b))) {                                                       \
      std::cerr << "Assertion failed: " #a " == " #b " at " << __FILE__ << ":" \
                << __LINE__ << std::endl;                                      \
      std::exit(1);                                                            \
    }                                                                          \
  } while (0)

using opentelemetry::nostd::get;

struct SpanStorage {
  std::mutex mutex;
  std::vector<opentelemetry::sdk::trace::SpanData> spans;
};

class TestSpanExporter : public opentelemetry::sdk::trace::SpanExporter {
 public:
  explicit TestSpanExporter(std::shared_ptr<SpanStorage> storage)
      : storage_(storage) {}

  std::unique_ptr<opentelemetry::sdk::trace::Recordable>
  MakeRecordable() noexcept override {
    return std::unique_ptr<opentelemetry::sdk::trace::Recordable>(
        new opentelemetry::sdk::trace::SpanData());
  }

  opentelemetry::sdk::common::ExportResult Export(
      const opentelemetry::nostd::span<
          std::unique_ptr<opentelemetry::sdk::trace::Recordable>>&
          spans) noexcept override {
    std::lock_guard<std::mutex> lock(storage_->mutex);
    for (auto& recordable : spans) {
      auto* data =
          static_cast<opentelemetry::sdk::trace::SpanData*>(recordable.get());
      if (data) {
        storage_->spans.push_back(*data);
      }
    }
    return opentelemetry::sdk::common::ExportResult::kSuccess;
  }

  bool ForceFlush(std::chrono::microseconds) noexcept override { return true; }
  bool Shutdown(std::chrono::microseconds) noexcept override { return true; }

 private:
  std::shared_ptr<SpanStorage> storage_;
};

void TestHttpInstrumentation() {
  auto storage = std::make_shared<SpanStorage>();

  // 1. Setup TracerProvider with SimpleSpanProcessor & TestSpanExporter
  auto exporter = std::unique_ptr<opentelemetry::sdk::trace::SpanExporter>(
      new TestSpanExporter(storage));
  auto processor = std::unique_ptr<opentelemetry::sdk::trace::SpanProcessor>(
      new opentelemetry::sdk::trace::SimpleSpanProcessor(std::move(exporter)));
  auto tracer_provider = std::shared_ptr<opentelemetry::trace::TracerProvider>(
      new opentelemetry::sdk::trace::TracerProvider(std::move(processor)));
  opentelemetry::trace::Provider::SetTracerProvider(tracer_provider);

  // 2. Setup HttpTraceContext propagator
  opentelemetry::context::propagation::GlobalTextMapPropagator::SetGlobalPropagator(
      opentelemetry::nostd::shared_ptr<
          opentelemetry::context::propagation::TextMapPropagator>(
          new opentelemetry::trace::propagation::HttpTraceContext()));

  // 3. Setup MeterProvider and instruments
  auto meter_provider = std::shared_ptr<opentelemetry::metrics::MeterProvider>(
      new opentelemetry::sdk::metrics::MeterProvider());
  opentelemetry::metrics::Provider::SetMeterProvider(meter_provider);
  auto instruments =
      opentelemetry_quickstart::CreateHttpInstruments("otel-quickstart-cpp");
  ASSERT_TRUE(instruments->server_duration != nullptr);
  ASSERT_TRUE(instruments->client_duration != nullptr);
  opentelemetry_quickstart::SetGlobalHttpInstruments(instruments);

  // 4. Setup server with instrumented route
  httplib::Server svr;
  opentelemetry_quickstart::RegisterInstrumentedGet(
      svr, "/test-route", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("hello-world", "text/plain");
      });

  int port = svr.bind_to_any_port("127.0.0.1");
  ASSERT_TRUE(port > 0);

  std::thread svr_thread([&]() { svr.listen_after_bind(); });

  // Give server a moment to start listening
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // 5. Send request using HttpClientGet
  std::string host = "http://127.0.0.1:" + std::to_string(port);
  bool success =
      opentelemetry_quickstart::HttpClientGet(host, "/test-route");
  ASSERT_TRUE(success);

  // Stop server
  svr.stop();
  svr_thread.join();

  // 6. Verify exported spans
  std::lock_guard<std::mutex> lock(storage->mutex);
  ASSERT_EQ(storage->spans.size(), 2);

  // Identify server and client spans
  const opentelemetry::sdk::trace::SpanData* server_span = nullptr;
  const opentelemetry::sdk::trace::SpanData* client_span = nullptr;

  for (const auto& span : storage->spans) {
    if (span.GetSpanKind() == opentelemetry::trace::SpanKind::kServer) {
      server_span = &span;
    } else if (span.GetSpanKind() == opentelemetry::trace::SpanKind::kClient) {
      client_span = &span;
    }
  }

  ASSERT_TRUE(server_span != nullptr);
  ASSERT_TRUE(client_span != nullptr);

  // Verify server span properties & semantic convention attributes
  ASSERT_EQ(server_span->GetName(), "GET /test-route");
  const auto& server_attrs = server_span->GetAttributes();
  ASSERT_TRUE(server_attrs.find("http.request.method") != server_attrs.end());
  ASSERT_EQ(get<std::string>(server_attrs.at("http.request.method")),
            "GET");
  ASSERT_TRUE(server_attrs.find("http.route") != server_attrs.end());
  ASSERT_EQ(get<std::string>(server_attrs.at("http.route")),
            "/test-route");
  ASSERT_TRUE(server_attrs.find("http.response.status_code") !=
              server_attrs.end());
  ASSERT_EQ(get<int>(server_attrs.at("http.response.status_code")), 200);

  // Verify client span properties & semantic convention attributes
  ASSERT_EQ(client_span->GetName(), "GET");
  const auto& client_attrs = client_span->GetAttributes();
  ASSERT_TRUE(client_attrs.find("http.request.method") != client_attrs.end());
  ASSERT_EQ(get<std::string>(client_attrs.at("http.request.method")),
            "GET");
  ASSERT_TRUE(client_attrs.find("url.full") != client_attrs.end());
  ASSERT_EQ(get<std::string>(client_attrs.at("url.full")),
            host + "/test-route");
  ASSERT_TRUE(client_attrs.find("http.response.status_code") !=
              client_attrs.end());
  ASSERT_EQ(get<int>(client_attrs.at("http.response.status_code")), 200);

  // Verify Distributed Trace Context propagation:
  // Server span must share the same trace ID and have client span as parent
  ASSERT_EQ(server_span->GetTraceId(), client_span->GetTraceId());
  ASSERT_EQ(server_span->GetParentSpanId(), client_span->GetSpanId());

  opentelemetry_quickstart::SetGlobalHttpInstruments(nullptr);
}

int main() {
  TestHttpInstrumentation();
  std::cout << "All HTTP instrumentation tests passed successfully!"
            << std::endl;
  return 0;
}

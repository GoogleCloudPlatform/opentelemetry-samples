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

#include "gcp_logging.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "opentelemetry/context/runtime_context.h"
#include "opentelemetry/sdk/trace/exporter.h"
#include "opentelemetry/sdk/trace/simple_processor.h"
#include "opentelemetry/sdk/trace/span_data.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/scope.h"

#define ASSERT_TRUE(cond)                                                      \
  if (!(cond)) {                                                               \
    std::cerr << "Assertion failed: " #cond " at " << __FILE__ << ":"          \
              << __LINE__ << std::endl;                                        \
    std::exit(1);                                                              \
  }

#define ASSERT_EQ(a, b)                                                        \
  if (!((a) == (b))) {                                                         \
    std::cerr << "Assertion failed: " #a " == " #b " at " << __FILE__ << ":"   \
              << __LINE__ << std::endl;                                        \
    std::exit(1);                                                              \
  }

namespace {

class NoopSpanExporter : public opentelemetry::sdk::trace::SpanExporter {
 public:
  std::unique_ptr<opentelemetry::sdk::trace::Recordable>
  MakeRecordable() noexcept override {
    return std::unique_ptr<opentelemetry::sdk::trace::Recordable>(
        new opentelemetry::sdk::trace::SpanData());
  }

  opentelemetry::sdk::common::ExportResult Export(
      const opentelemetry::nostd::span<
          std::unique_ptr<opentelemetry::sdk::trace::Recordable>>&)
      noexcept override {
    return opentelemetry::sdk::common::ExportResult::kSuccess;
  }

  bool ForceFlush(std::chrono::microseconds) noexcept override { return true; }
  bool Shutdown(std::chrono::microseconds) noexcept override { return true; }
};

}  // namespace

void TestLogEntryFormattingWithoutSpan() {
  nlohmann::json extra;
  extra["subRequests"] = 5;
  extra["customField"] = "testValue";

  auto entry = opentelemetry_quickstart::FormatLogEntry("test message", extra);

  ASSERT_TRUE(entry.is_object());
  ASSERT_EQ(entry["severity"], "INFO");
  ASSERT_EQ(entry["message"], "test message");
  ASSERT_EQ(entry["subRequests"], 5);
  ASSERT_EQ(entry["customField"], "testValue");
  ASSERT_TRUE(entry.contains("timestamp"));
  std::string ts = entry["timestamp"];
  ASSERT_TRUE(ts.length() > 20 && ts.back() == 'Z');

  // Without active span, GCP trace fields should not be populated
  ASSERT_TRUE(!entry.contains("logging.googleapis.com/trace"));
  ASSERT_TRUE(!entry.contains("logging.googleapis.com/spanId"));
  ASSERT_TRUE(!entry.contains("logging.googleapis.com/trace_sampled"));
}

void TestLogEntryFormattingWithActiveSpan() {
  auto exporter = std::unique_ptr<opentelemetry::sdk::trace::SpanExporter>(
      new NoopSpanExporter());
  auto processor = std::unique_ptr<opentelemetry::sdk::trace::SpanProcessor>(
      new opentelemetry::sdk::trace::SimpleSpanProcessor(std::move(exporter)));
  auto tracer_provider = std::shared_ptr<opentelemetry::trace::TracerProvider>(
      new opentelemetry::sdk::trace::TracerProvider(std::move(processor)));
  opentelemetry::trace::Provider::SetTracerProvider(tracer_provider);

  auto tracer = tracer_provider->GetTracer("test-tracer");
  auto span = tracer->StartSpan("test-span");
  {
    opentelemetry::trace::Scope scope(span);

    auto entry = opentelemetry_quickstart::FormatLogEntry("correlated log");

    ASSERT_EQ(entry["message"], "correlated log");
    ASSERT_TRUE(entry.contains("logging.googleapis.com/trace"));
    ASSERT_TRUE(entry.contains("logging.googleapis.com/spanId"));
    ASSERT_TRUE(entry.contains("logging.googleapis.com/trace_sampled"));

    char expected_trace_id[32];
    span->GetContext().trace_id().ToLowerBase16(expected_trace_id);
    char expected_span_id[16];
    span->GetContext().span_id().ToLowerBase16(expected_span_id);

    ASSERT_EQ(entry["logging.googleapis.com/trace"],
              std::string(expected_trace_id, 32));
    ASSERT_EQ(entry["logging.googleapis.com/spanId"],
              std::string(expected_span_id, 16));
  }
  span->End();
}

void TestConcurrentLogging() {
  std::vector<std::thread> threads;
  for (int i = 0; i < 4; ++i) {
    threads.emplace_back([i]() {
      for (int j = 0; j < 20; ++j) {
        opentelemetry_quickstart::LogInfo("concurrent log",
                                         {{"thread", i}, {"iter", j}});
      }
    });
  }
  for (auto& t : threads) {
    t.join();
  }
}

int main() {
  TestLogEntryFormattingWithoutSpan();
  TestLogEntryFormattingWithActiveSpan();
  TestConcurrentLogging();
  std::cout << "All GCP logging tests passed successfully!" << std::endl;
  return 0;
}

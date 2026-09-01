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

#include "otlptrace.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "opentelemetry/nostd/variant.h"
#include "opentelemetry/sdk/trace/exporter.h"
#include "opentelemetry/sdk/trace/simple_processor.h"
#include "opentelemetry/sdk/trace/span_data.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
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

struct ExportedSpan {
  std::string name;
  std::unordered_map<std::string, std::string> attributes;
};

struct SpanStorage {
  std::mutex mutex;
  std::vector<ExportedSpan> spans;
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
    for (const auto& recordable : spans) {
      auto* data =
          static_cast<const opentelemetry::sdk::trace::SpanData*>(recordable.get());
      if (data) {
        ExportedSpan s;
        s.name = std::string(data->GetName());
        for (const auto& kv : data->GetAttributes()) {
          if (opentelemetry::nostd::holds_alternative<std::string>(kv.second)) {
            s.attributes[kv.first] = get<std::string>(kv.second);
          }
        }
        storage_->spans.push_back(s);
      }
    }
    return opentelemetry::sdk::common::ExportResult::kSuccess;
  }

  bool ForceFlush(std::chrono::microseconds) noexcept override { return true; }
  bool Shutdown(std::chrono::microseconds) noexcept override { return true; }

 private:
  std::shared_ptr<SpanStorage> storage_;
};

void TestResourceCreation() {
  auto resource = otlptrace_sample::CreateResource("my-test-project");
  const auto& attrs = resource.GetAttributes();

  ASSERT_EQ(get<std::string>(attrs.at("service.name")), "otlp-gcp-cpp-trace-sample");
  ASSERT_EQ(get<std::string>(attrs.at("gcp.project_id")), "my-test-project");
}

void TestExporterOptionsDefault() {
  auto opts = otlptrace_sample::GetExporterOptions(/*load_credentials=*/false);
  ASSERT_TRUE(!opts.endpoint.empty());
  ASSERT_TRUE(opts.endpoint.find("telemetry.googleapis.com") != std::string::npos ||
              opts.endpoint.find("443") != std::string::npos);
}

void TestEmitSampleTrace() {
  auto storage = std::make_shared<SpanStorage>();
  auto exporter = std::unique_ptr<opentelemetry::sdk::trace::SpanExporter>(
      new TestSpanExporter(storage));
  auto processor = std::unique_ptr<opentelemetry::sdk::trace::SpanProcessor>(
      new opentelemetry::sdk::trace::SimpleSpanProcessor(std::move(exporter)));
  auto provider = std::shared_ptr<opentelemetry::trace::TracerProvider>(
      new opentelemetry::sdk::trace::TracerProvider(std::move(processor)));

  auto tracer = provider->GetTracer("otlp-gcp-cpp-trace-sample");
  ASSERT_TRUE(tracer != nullptr);

  otlptrace_sample::EmitSampleTrace(tracer.get());

  std::lock_guard<std::mutex> lock(storage->mutex);
  ASSERT_EQ(storage->spans.size(), 1);
  const auto& span = storage->spans[0];
  ASSERT_EQ(span.name, "sample-grpc-span");
  ASSERT_EQ(span.attributes.at("sample.type"), "direct-otlp-grpc-export");
  ASSERT_EQ(span.attributes.at("auth.type"), "google_default_credentials");
}

int main() {
  TestResourceCreation();
  TestExporterOptionsDefault();
  TestEmitSampleTrace();
  std::cout << "All otlptrace tests passed successfully!" << std::endl;
  return 0;
}

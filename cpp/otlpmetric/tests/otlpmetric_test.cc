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

#include "otlpmetric.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "opentelemetry/nostd/variant.h"
#include "opentelemetry/sdk/metrics/meter_provider.h"
#include "opentelemetry/sdk/metrics/view/view_registry.h"

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

void TestResourceCreation() {
  // Test with explicit project ID
  auto resource = otlpmetric_sample::CreateResource("my-test-project");
  const auto& attrs = resource.GetAttributes();

  ASSERT_EQ(get<std::string>(attrs.at("service.name")), "otlp-gcp-metric-sample");
  ASSERT_EQ(get<std::string>(attrs.at("location")), "us-central1");
  ASSERT_EQ(get<std::string>(attrs.at("cluster")), "sample-cluster");
  ASSERT_EQ(get<std::string>(attrs.at("namespace")), "default");
  ASSERT_EQ(get<std::string>(attrs.at("job")), "otlp-metric-sample");
  ASSERT_TRUE(!get<std::string>(attrs.at("instance")).empty());
  ASSERT_EQ(get<std::string>(attrs.at("gcp.project_id")), "my-test-project");
}

void TestExporterOptionsDefault() {
  auto opts = otlpmetric_sample::GetExporterOptions(/*load_credentials=*/false);
  ASSERT_TRUE(!opts.endpoint.empty());
  ASSERT_TRUE(opts.endpoint.find("telemetry.googleapis.com") != std::string::npos ||
              opts.endpoint.find("443") != std::string::npos);
}

void TestRecordMetrics() {
  auto provider = std::shared_ptr<opentelemetry::metrics::MeterProvider>(
      new opentelemetry::sdk::metrics::MeterProvider(
          std::make_unique<opentelemetry::sdk::metrics::ViewRegistry>()));

  auto meter = provider->GetMeter("gcp.otlp.sample");
  ASSERT_TRUE(meter != nullptr);

  // Record metrics against meter and verify execution
  otlpmetric_sample::RecordSampleMetrics(meter.get());
}

int main() {
  TestResourceCreation();
  TestExporterOptionsDefault();
  TestRecordMetrics();
  std::cout << "All otlpmetric tests passed successfully!" << std::endl;
  return 0;
}

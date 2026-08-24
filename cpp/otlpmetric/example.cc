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

#include <iostream>
#include <memory>

// [START opentelemetry_otlp_grpc_auth_imports]
#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>
// [END opentelemetry_otlp_grpc_auth_imports]

// [START opentelemetry_otlp_grpc_imports]
#include "opentelemetry/exporters/otlp/otlp_grpc_metric_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_metric_exporter_options.h"
#include "opentelemetry/metrics/provider.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader.h"
#include "opentelemetry/sdk/metrics/meter_provider.h"
#include "opentelemetry/sdk/resource/resource.h"
// [END opentelemetry_otlp_grpc_imports]

#include "otlpmetric.h"

int main(int argc, char** argv) {
  // [START opentelemetry_otlp_grpc_auth_setup]
  auto resource = otlpmetric_sample::CreateResource();
  auto opts = otlpmetric_sample::GetExporterOptions();
  // [END opentelemetry_otlp_grpc_auth_setup]

  // [START opentelemetry_otlp_grpc_init]
  auto provider = otlpmetric_sample::CreateMeterProvider(resource, opts);
  opentelemetry::metrics::Provider::SetMeterProvider(provider);
  auto meter = provider->GetMeter("gcp.otlp.sample");
  // [END opentelemetry_otlp_grpc_init]

  std::cout << "Recording metrics with gRPC and Google Default Credentials..." << std::endl;
  otlpmetric_sample::RecordSampleMetrics(meter.get());

  std::cout << "Flushing and shutting down meter provider..." << std::endl;
  auto* sdk_mp = static_cast<opentelemetry::sdk::metrics::MeterProvider*>(provider.get());
  if (sdk_mp != nullptr) {
    sdk_mp->ForceFlush();
    sdk_mp->Shutdown();
  }
  provider.reset();
  opentelemetry::metrics::Provider::SetMeterProvider(
      opentelemetry::nostd::shared_ptr<opentelemetry::metrics::MeterProvider>());

  std::cout << "Done!" << std::endl;
  return 0;
}

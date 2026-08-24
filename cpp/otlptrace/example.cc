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
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_options.h"
#include "opentelemetry/sdk/resource/resource.h"
#include "opentelemetry/sdk/trace/batch_span_processor.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/trace/provider.h"
// [END opentelemetry_otlp_grpc_imports]

#include "otlptrace.h"

int main(int argc, char** argv) {
  // [START opentelemetry_otlp_grpc_auth_setup]
  auto resource = otlptrace_sample::CreateResource();
  auto opts = otlptrace_sample::GetExporterOptions();
  // [END opentelemetry_otlp_grpc_auth_setup]

  // [START opentelemetry_otlp_grpc_init]
  auto provider = otlptrace_sample::CreateTracerProvider(resource, opts);
  opentelemetry::trace::Provider::SetTracerProvider(provider);
  auto tracer = provider->GetTracer("otlp-gcp-cpp-trace-sample");
  // [END opentelemetry_otlp_grpc_init]

  std::cout << "Starting trace export with gRPC and Google Default Credentials..." << std::endl;
  otlptrace_sample::EmitSampleTrace(tracer.get());

  std::cout << "Flushing and shutting down tracer provider..." << std::endl;
  auto* sdk_tp = static_cast<opentelemetry::sdk::trace::TracerProvider*>(provider.get());
  if (sdk_tp != nullptr) {
    sdk_tp->ForceFlush();
    sdk_tp->Shutdown();
  }
  provider.reset();
  opentelemetry::trace::Provider::SetTracerProvider(
      opentelemetry::nostd::shared_ptr<opentelemetry::trace::TracerProvider>());

  std::cout << "Done!" << std::endl;
  return 0;
}

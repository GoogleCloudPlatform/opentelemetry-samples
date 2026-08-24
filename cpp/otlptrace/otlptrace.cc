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
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>

#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h"
#include "opentelemetry/sdk/trace/batch_span_processor.h"
#include "opentelemetry/sdk/trace/batch_span_processor_options.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/trace/provider.h"

namespace otlptrace_sample {

namespace {

std::string GetEnv(const char* name, const std::string& default_val = "") {
  const char* val = std::getenv(name);
  return (val != nullptr && val[0] != '\0') ? std::string(val) : default_val;
}

}  // namespace

opentelemetry::sdk::resource::Resource CreateResource(const std::string& project_id) {
  std::string proj = project_id.empty()
                         ? GetEnv("GOOGLE_CLOUD_PROJECT", GetEnv("GCP_PROJECT"))
                         : project_id;

  opentelemetry::sdk::resource::ResourceAttributes attributes = {
      {"service.name", "otlp-gcp-cpp-trace-sample"},
  };
  if (!proj.empty()) {
    attributes["gcp.project_id"] = proj;
  }
  return opentelemetry::sdk::resource::Resource::Create(attributes);
}

opentelemetry::exporter::otlp::OtlpGrpcExporterOptions GetExporterOptions(bool load_credentials) {
  opentelemetry::exporter::otlp::OtlpGrpcExporterOptions opts;
  opts.endpoint = GetEnv("OTEL_EXPORTER_OTLP_TRACES_ENDPOINT",
                         GetEnv("OTEL_EXPORTER_OTLP_ENDPOINT", "telemetry.googleapis.com:443"));
  if (load_credentials) {
    opts.credentials = grpc::GoogleDefaultCredentials();
    if (!opts.credentials) {
      throw std::runtime_error(
          "Could not load Application Default Credentials. Run: "
          "gcloud auth application-default login");
    }
  }
  return opts;
}

std::shared_ptr<opentelemetry::trace::TracerProvider> CreateTracerProvider(
    const opentelemetry::sdk::resource::Resource& resource,
    const opentelemetry::exporter::otlp::OtlpGrpcExporterOptions& opts) {
  auto exporter = opentelemetry::exporter::otlp::OtlpGrpcExporterFactory::Create(opts);

  opentelemetry::sdk::trace::BatchSpanProcessorOptions bsp_opts;
  bsp_opts.schedule_delay_millis = std::chrono::milliseconds(500);

  auto processor = std::unique_ptr<opentelemetry::sdk::trace::SpanProcessor>(
      new opentelemetry::sdk::trace::BatchSpanProcessor(std::move(exporter), bsp_opts));

  auto provider = std::shared_ptr<opentelemetry::trace::TracerProvider>(
      new opentelemetry::sdk::trace::TracerProvider(std::move(processor), resource));
  return provider;
}

void EmitSampleTrace(opentelemetry::trace::Tracer* tracer) {
  if (tracer == nullptr) return;
  auto span = tracer->StartSpan("sample-grpc-span");
  span->SetAttribute("sample.type", "direct-otlp-grpc-export");
  span->SetAttribute("auth.type", "google_default_credentials");
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  span->End();
}

}  // namespace otlptrace_sample

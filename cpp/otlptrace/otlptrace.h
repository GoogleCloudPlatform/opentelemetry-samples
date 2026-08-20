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

#ifndef CPP_OTLPTRACE_OTLPTRACE_H_
#define CPP_OTLPTRACE_OTLPTRACE_H_

#include <memory>
#include <string>

#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_options.h"
#include "opentelemetry/sdk/resource/resource.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/trace/tracer.h"

namespace otlptrace_sample {

// Builds OpenTelemetry Resource with service name and GCP project ID.
opentelemetry::sdk::resource::Resource CreateResource(
    const std::string& project_id = "");

// Resolves OTLP gRPC trace exporter options from environment variables and credentials.
opentelemetry::exporter::otlp::OtlpGrpcExporterOptions GetExporterOptions();

// Creates and initializes the TracerProvider with batch span processor.
std::shared_ptr<opentelemetry::trace::TracerProvider> CreateTracerProvider(
    const opentelemetry::sdk::resource::Resource& resource,
    const opentelemetry::exporter::otlp::OtlpGrpcExporterOptions& opts);

// Emits sample trace span with direct OTLP gRPC attributes.
void EmitSampleTrace(opentelemetry::trace::Tracer* tracer);

}  // namespace otlptrace_sample

#endif  // CPP_OTLPTRACE_OTLPTRACE_H_

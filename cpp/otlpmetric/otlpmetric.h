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

#ifndef CPP_OTLPMETRIC_OTLPMETRIC_H_
#define CPP_OTLPMETRIC_OTLPMETRIC_H_

#include <memory>
#include <string>

#include "opentelemetry/exporters/otlp/otlp_grpc_metric_exporter_options.h"
#include "opentelemetry/metrics/meter.h"
#include "opentelemetry/sdk/metrics/meter_provider.h"
#include "opentelemetry/sdk/resource/resource.h"

namespace otlpmetric_sample {

// Builds OpenTelemetry Resource with GCP service attributes and optional project ID.
opentelemetry::sdk::resource::Resource CreateResource(
    const std::string& project_id = "");

// Resolves OTLP gRPC metric exporter options from environment variables and credentials.
opentelemetry::exporter::otlp::OtlpGrpcMetricExporterOptions GetExporterOptions();

// Creates and initializes the MeterProvider with periodic exporting metric reader.
std::shared_ptr<opentelemetry::metrics::MeterProvider> CreateMeterProvider(
    const opentelemetry::sdk::resource::Resource& resource,
    const opentelemetry::exporter::otlp::OtlpGrpcMetricExporterOptions& opts);

// Records sample metric counter data.
void RecordSampleMetrics(opentelemetry::metrics::Meter* meter);

}  // namespace otlpmetric_sample

#endif  // CPP_OTLPMETRIC_OTLPMETRIC_H_

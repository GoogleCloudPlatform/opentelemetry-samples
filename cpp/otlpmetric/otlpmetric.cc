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

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <unistd.h>

#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>

#include "opentelemetry/exporters/otlp/otlp_grpc_metric_exporter_factory.h"
#include "opentelemetry/metrics/provider.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_options.h"
#include "opentelemetry/sdk/metrics/meter_provider.h"
#include "opentelemetry/sdk/metrics/view/view_registry.h"

namespace otlpmetric_sample {

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

  // Google Managed Prometheus (GMP) maps resource attributes to prometheus_target:
  // location, cluster, namespace, job, instance. Since opentelemetry-cpp lacks
  // an automatic GCP resource detector, these attributes are configured explicitly.
  std::string instance_id = GetEnv("INSTANCE", "instance-" + std::to_string(getpid()));
  opentelemetry::sdk::resource::ResourceAttributes attributes = {
      {"service.name", GetEnv("OTEL_SERVICE_NAME", "otlp-gcp-metric-sample")},
      {"location", GetEnv("LOCATION", "us-central1")},
      {"cluster", GetEnv("CLUSTER", "sample-cluster")},
      {"namespace", GetEnv("NAMESPACE", "default")},
      {"job", GetEnv("JOB", "otlp-metric-sample")},
      {"instance", instance_id},
  };
  if (!proj.empty()) {
    attributes["gcp.project_id"] = proj;
  }
  return opentelemetry::sdk::resource::Resource::Create(attributes);
}

opentelemetry::exporter::otlp::OtlpGrpcMetricExporterOptions GetExporterOptions() {
  opentelemetry::exporter::otlp::OtlpGrpcMetricExporterOptions opts;
  opts.endpoint = GetEnv("OTEL_EXPORTER_OTLP_METRICS_ENDPOINT",
                         GetEnv("OTEL_EXPORTER_OTLP_ENDPOINT", "telemetry.googleapis.com:443"));
  opts.credentials = grpc::GoogleDefaultCredentials();
  return opts;
}

std::shared_ptr<opentelemetry::metrics::MeterProvider> CreateMeterProvider(
    const opentelemetry::sdk::resource::Resource& resource,
    const opentelemetry::exporter::otlp::OtlpGrpcMetricExporterOptions& opts) {
  auto exporter = opentelemetry::exporter::otlp::OtlpGrpcMetricExporterFactory::Create(opts);

  opentelemetry::sdk::metrics::PeriodicExportingMetricReaderOptions reader_opts;
  reader_opts.export_interval_millis = std::chrono::milliseconds(5000);
  reader_opts.export_timeout_millis = std::chrono::milliseconds(2500);

  auto reader = std::unique_ptr<opentelemetry::sdk::metrics::MetricReader>(
      new opentelemetry::sdk::metrics::PeriodicExportingMetricReader(
          std::move(exporter), reader_opts));

  auto provider = std::shared_ptr<opentelemetry::metrics::MeterProvider>(
      new opentelemetry::sdk::metrics::MeterProvider(
          std::make_unique<opentelemetry::sdk::metrics::ViewRegistry>(), resource));
  static_cast<opentelemetry::sdk::metrics::MeterProvider*>(provider.get())
      ->AddMetricReader(std::move(reader));

  return provider;
}

void RecordSampleMetrics(opentelemetry::metrics::Meter* meter) {
  if (meter == nullptr) return;
  auto counter = meter->CreateUInt64Counter("sample.otlp.counter");
  counter->Add(1);
}

}  // namespace otlpmetric_sample

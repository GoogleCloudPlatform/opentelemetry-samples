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

#include "setup_opentelemetry.h"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <unistd.h>

#include "opentelemetry/context/propagation/global_propagator.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter_options.h"
#include "opentelemetry/exporters/otlp/otlp_http_metric_exporter.h"
#include "opentelemetry/exporters/otlp/otlp_http_metric_exporter_options.h"
#include "opentelemetry/metrics/provider.h"
#include "opentelemetry/sdk/metrics/aggregation/histogram_aggregation.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_options.h"
#include "opentelemetry/sdk/metrics/meter_provider.h"
#include "opentelemetry/sdk/metrics/view/instrument_selector_factory.h"
#include "opentelemetry/sdk/metrics/view/meter_selector_factory.h"
#include "opentelemetry/sdk/metrics/view/view_factory.h"
#include "opentelemetry/sdk/metrics/view/view_registry.h"
#include "opentelemetry/sdk/resource/resource.h"
#include "opentelemetry/sdk/trace/batch_span_processor.h"
#include "opentelemetry/sdk/trace/batch_span_processor_options.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/trace/propagation/http_trace_context.h"
#include "opentelemetry/trace/provider.h"

namespace opentelemetry_quickstart {

// [START opentelemetry_instrumentation_setup_opentelemetry]
void SetupOpenTelemetry() {
  std::string service_name = "otel-quickstart-cpp";
  const char* env_service_name = std::getenv("OTEL_SERVICE_NAME");
  if (env_service_name != nullptr && env_service_name[0] != '\0') {
    service_name = env_service_name;
  }

  // Set service.instance.id to identify unique process instance
  auto resource = opentelemetry::sdk::resource::Resource::Create({
      {"service.name", service_name},
      {"service.instance.id", "worker-" + std::to_string(getpid())},
  });

  // 1. Initialize Tracing with OTLP HTTP exporter
  // Endpoint and headers are read from OTEL_EXPORTER_OTLP_ENDPOINT / OTEL_EXPORTER_OTLP_TRACES_ENDPOINT
  opentelemetry::exporter::otlp::OtlpHttpExporterOptions trace_opts;
  trace_opts.content_type = opentelemetry::exporter::otlp::HttpRequestContentType::kBinary;
  auto trace_exporter = std::unique_ptr<opentelemetry::sdk::trace::SpanExporter>(
      new opentelemetry::exporter::otlp::OtlpHttpExporter(trace_opts));

  opentelemetry::sdk::trace::BatchSpanProcessorOptions bsp_opts;
  bsp_opts.schedule_delay_millis = std::chrono::milliseconds(500);
  auto span_processor = std::unique_ptr<opentelemetry::sdk::trace::SpanProcessor>(
      new opentelemetry::sdk::trace::BatchSpanProcessor(std::move(trace_exporter), bsp_opts));

  auto tracer_provider = std::shared_ptr<opentelemetry::trace::TracerProvider>(
      new opentelemetry::sdk::trace::TracerProvider(std::move(span_processor), resource));
  opentelemetry::trace::Provider::SetTracerProvider(tracer_provider);

  // Set W3C Trace Context as global propagator
  opentelemetry::context::propagation::GlobalTextMapPropagator::SetGlobalPropagator(
      opentelemetry::nostd::shared_ptr<opentelemetry::context::propagation::TextMapPropagator>(
          new opentelemetry::trace::propagation::HttpTraceContext()));

  // 2. Initialize Metrics with OTLP HTTP metric exporter
  // Endpoint and headers are read from OTEL_EXPORTER_OTLP_ENDPOINT / OTEL_EXPORTER_OTLP_METRICS_ENDPOINT
  opentelemetry::exporter::otlp::OtlpHttpMetricExporterOptions metric_opts;
  metric_opts.content_type = opentelemetry::exporter::otlp::HttpRequestContentType::kBinary;
  auto metric_exporter = std::unique_ptr<opentelemetry::sdk::metrics::PushMetricExporter>(
      new opentelemetry::exporter::otlp::OtlpHttpMetricExporter(metric_opts));

  opentelemetry::sdk::metrics::PeriodicExportingMetricReaderOptions reader_opts;
  reader_opts.export_interval_millis = std::chrono::milliseconds(5000);
  reader_opts.export_timeout_millis = std::chrono::milliseconds(2500);
  auto metric_reader = std::unique_ptr<opentelemetry::sdk::metrics::MetricReader>(
      new opentelemetry::sdk::metrics::PeriodicExportingMetricReader(
          std::move(metric_exporter), reader_opts));

  auto view_registry = std::make_unique<opentelemetry::sdk::metrics::ViewRegistry>();
  auto hist_config = std::make_shared<opentelemetry::sdk::metrics::HistogramAggregationConfig>();
  hist_config->boundaries_ = {0.005, 0.01,  0.025, 0.05, 0.075, 0.1, 0.25,
                              0.5,   0.75, 1.0,   2.5,  5.0,   7.5, 10.0};
  auto instrument_selector = opentelemetry::sdk::metrics::InstrumentSelectorFactory::Create(
      opentelemetry::sdk::metrics::InstrumentType::kHistogram, "http.*.request.duration", "s");
  auto meter_selector = opentelemetry::sdk::metrics::MeterSelectorFactory::Create(
      "otel-quickstart-cpp", "", "");
  auto view = opentelemetry::sdk::metrics::ViewFactory::Create(
      "", "", opentelemetry::sdk::metrics::AggregationType::kHistogram, hist_config);
  view_registry->AddView(std::move(instrument_selector), std::move(meter_selector), std::move(view));

  auto meter_provider = std::shared_ptr<opentelemetry::metrics::MeterProvider>(
      new opentelemetry::sdk::metrics::MeterProvider(std::move(view_registry), resource));
  static_cast<opentelemetry::sdk::metrics::MeterProvider*>(meter_provider.get())
      ->AddMetricReader(std::move(metric_reader));
  opentelemetry::metrics::Provider::SetMeterProvider(meter_provider);
}
// [END opentelemetry_instrumentation_setup_opentelemetry]

void CleanUpOpenTelemetry() {
  auto tracer_provider = opentelemetry::trace::Provider::GetTracerProvider();
  auto* sdk_tp = dynamic_cast<opentelemetry::sdk::trace::TracerProvider*>(tracer_provider.get());
  if (sdk_tp != nullptr) {
    sdk_tp->ForceFlush();
    sdk_tp->Shutdown();
  }
  opentelemetry::trace::Provider::SetTracerProvider(
      opentelemetry::nostd::shared_ptr<opentelemetry::trace::TracerProvider>());

  auto meter_provider = opentelemetry::metrics::Provider::GetMeterProvider();
  auto* sdk_mp = dynamic_cast<opentelemetry::sdk::metrics::MeterProvider*>(meter_provider.get());
  if (sdk_mp != nullptr) {
    sdk_mp->ForceFlush();
    sdk_mp->Shutdown();
  }
  opentelemetry::metrics::Provider::SetMeterProvider(
      opentelemetry::nostd::shared_ptr<opentelemetry::metrics::MeterProvider>());

  opentelemetry::context::propagation::GlobalTextMapPropagator::SetGlobalPropagator(
      opentelemetry::nostd::shared_ptr<opentelemetry::context::propagation::TextMapPropagator>());
}

}  // namespace opentelemetry_quickstart

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

#ifndef CPP_INSTRUMENTATION_QUICKSTART_HTTP_INSTRUMENTATION_H_
#define CPP_INSTRUMENTATION_QUICKSTART_HTTP_INSTRUMENTATION_H_

#include <functional>
#include <memory>
#include <string>

#include "httplib.h"
#include "opentelemetry/context/propagation/text_map_propagator.h"
#include "opentelemetry/metrics/sync_instruments.h"

namespace opentelemetry_quickstart {

// HTTP headers carrier for context propagation injection/extraction
class HttpHeadersCarrier : public opentelemetry::context::propagation::TextMapCarrier {
 public:
  explicit HttpHeadersCarrier(httplib::Headers& headers);

  opentelemetry::nostd::string_view Get(
      opentelemetry::nostd::string_view key) const noexcept override;

  void Set(opentelemetry::nostd::string_view key,
           opentelemetry::nostd::string_view value) noexcept override;

  bool Keys(opentelemetry::nostd::function_ref<
            bool(opentelemetry::nostd::string_view)> callback) const noexcept override;

 private:
  httplib::Headers& headers_;
};

// Const HTTP headers carrier for context extraction from read-only headers
class ConstHttpHeadersCarrier : public opentelemetry::context::propagation::TextMapCarrier {
 public:
  explicit ConstHttpHeadersCarrier(const httplib::Headers& headers);

  opentelemetry::nostd::string_view Get(
      opentelemetry::nostd::string_view key) const noexcept override;

  void Set(opentelemetry::nostd::string_view key,
           opentelemetry::nostd::string_view value) noexcept override;

  bool Keys(opentelemetry::nostd::function_ref<
            bool(opentelemetry::nostd::string_view)> callback) const noexcept override;

 private:
  const httplib::Headers& headers_;
};

struct HttpInstruments {
  opentelemetry::nostd::unique_ptr<opentelemetry::metrics::Histogram<double>> server_duration;
  opentelemetry::nostd::unique_ptr<opentelemetry::metrics::Histogram<double>> client_duration;
};

// Initializes the standard OpenTelemetry HTTP metrics
std::shared_ptr<HttpInstruments> CreateHttpInstruments(
    const std::string& meter_name = "otel-quickstart-cpp");

void SetGlobalHttpInstruments(std::shared_ptr<HttpInstruments> instruments);
std::shared_ptr<HttpInstruments> GetGlobalHttpInstruments();

// HTTP client wrapper with automatic trace context propagation and HTTP client metrics
bool HttpClientGet(const std::string& host, const std::string& path);

// Server route registration wrapper with automatic span creation, context extraction, and HTTP server metrics
void RegisterInstrumentedGet(
    httplib::Server& svr,
    const std::string& pattern,
    std::function<void(const httplib::Request&, httplib::Response&)> handler);

}  // namespace opentelemetry_quickstart

#endif  // CPP_INSTRUMENTATION_QUICKSTART_HTTP_INSTRUMENTATION_H_

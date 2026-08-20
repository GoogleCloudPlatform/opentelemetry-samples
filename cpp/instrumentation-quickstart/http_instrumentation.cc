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

#include "http_instrumentation.h"

#include <chrono>
#include <string>

#include "opentelemetry/context/propagation/global_propagator.h"
#include "opentelemetry/metrics/provider.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/scope.h"
#include "opentelemetry/trace/tracer.h"

namespace opentelemetry_quickstart {

namespace {

std::shared_ptr<HttpInstruments> g_instruments{nullptr};

void ParseHostAndPort(const std::string& host_str, std::string& address, int& port) {
  address = "127.0.0.1";
  port = 8080;
  std::string h = host_str;
  if (h.rfind("http://", 0) == 0) {
    h = h.substr(7);
  } else if (h.rfind("https://", 0) == 0) {
    h = h.substr(8);
  }
  auto slash = h.find('/');
  if (slash != std::string::npos) {
    h = h.substr(0, slash);
  }
  auto colon = h.find(':');
  if (colon != std::string::npos) {
    address = h.substr(0, colon);
    try {
      port = std::stoi(h.substr(colon + 1));
    } catch (...) {
      port = 8080;
    }
  } else if (!h.empty()) {
    address = h;
  }
}

}  // namespace

HttpHeadersCarrier::HttpHeadersCarrier(httplib::Headers& headers) : headers_(headers) {}

opentelemetry::nostd::string_view HttpHeadersCarrier::Get(
    opentelemetry::nostd::string_view key) const noexcept {
  auto it = headers_.find(std::string(key.data(), key.size()));
  if (it != headers_.end()) {
    return it->second;
  }
  return "";
}

void HttpHeadersCarrier::Set(opentelemetry::nostd::string_view key,
                             opentelemetry::nostd::string_view value) noexcept {
  headers_.emplace(std::string(key.data(), key.size()),
                   std::string(value.data(), value.size()));
}

bool HttpHeadersCarrier::Keys(opentelemetry::nostd::function_ref<
                              bool(opentelemetry::nostd::string_view)> callback) const noexcept {
  for (const auto& kv : headers_) {
    if (!callback(kv.first)) {
      return false;
    }
  }
  return true;
}

ConstHttpHeadersCarrier::ConstHttpHeadersCarrier(const httplib::Headers& headers)
    : headers_(headers) {}

opentelemetry::nostd::string_view ConstHttpHeadersCarrier::Get(
    opentelemetry::nostd::string_view key) const noexcept {
  auto it = headers_.find(std::string(key.data(), key.size()));
  if (it != headers_.end()) {
    return it->second;
  }
  return "";
}

void ConstHttpHeadersCarrier::Set(opentelemetry::nostd::string_view,
                                  opentelemetry::nostd::string_view) noexcept {}

bool ConstHttpHeadersCarrier::Keys(opentelemetry::nostd::function_ref<
                                   bool(opentelemetry::nostd::string_view)> callback) const noexcept {
  for (const auto& kv : headers_) {
    if (!callback(kv.first)) {
      return false;
    }
  }
  return true;
}

std::shared_ptr<HttpInstruments> CreateHttpInstruments(const std::string& meter_name) {
  auto meter = opentelemetry::metrics::Provider::GetMeterProvider()->GetMeter(meter_name);
  auto instruments = std::make_shared<HttpInstruments>();
  instruments->server_duration = meter->CreateDoubleHistogram(
      "http.server.request.duration", "Duration of HTTP server requests.", "s");
  instruments->client_duration = meter->CreateDoubleHistogram(
      "http.client.request.duration", "Duration of HTTP client requests.", "s");
  return instruments;
}

void SetGlobalHttpInstruments(std::shared_ptr<HttpInstruments> instruments) {
  g_instruments = std::move(instruments);
}

std::shared_ptr<HttpInstruments> GetGlobalHttpInstruments() {
  return g_instruments;
}

bool HttpClientGet(const std::string& host, const std::string& path) {
  auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("otel-quickstart-cpp");
  opentelemetry::trace::StartSpanOptions options;
  options.kind = opentelemetry::trace::SpanKind::kClient;

  std::string server_address;
  int server_port;
  ParseHostAndPort(host, server_address, server_port);

  auto span = tracer->StartSpan("GET", options);
  span->SetAttribute("http.request.method", "GET");
  span->SetAttribute("url.full", host + path);
  span->SetAttribute("server.address", server_address);
  span->SetAttribute("server.port", server_port);
  span->SetAttribute("network.protocol.name", "http");
  span->SetAttribute("network.protocol.version", "1.1");
  opentelemetry::trace::Scope scope(span);

  httplib::Client cli(host);
  cli.set_connection_timeout(5, 0);
  cli.set_read_timeout(5, 0);

  httplib::Headers headers;
  HttpHeadersCarrier carrier(headers);
  opentelemetry::context::propagation::GlobalTextMapPropagator::GetGlobalPropagator()->Inject(
      carrier, opentelemetry::context::RuntimeContext::GetCurrent());

  auto start_time = std::chrono::steady_clock::now();
  auto res = cli.Get(path.c_str(), headers);
  auto end_time = std::chrono::steady_clock::now();
  double duration_s = std::chrono::duration<double>(end_time - start_time).count();

  int status_code = res ? res->status : 0;
  if (res) {
    span->SetAttribute("http.response.status_code", res->status);
  }

  if (g_instruments && g_instruments->client_duration) {
    g_instruments->client_duration->Record(
        duration_s,
        {
            {"http.request.method", "GET"},
            {"http.response.status_code", status_code},
            {"network.protocol.name", "http"},
            {"network.protocol.version", "1.1"},
            {"server.address", server_address},
            {"server.port", server_port},
        },
        opentelemetry::context::RuntimeContext::GetCurrent());
  }

  span->End();
  return res && res->status == 200;
}

void RegisterInstrumentedGet(
    httplib::Server& svr,
    const std::string& pattern,
    std::function<void(const httplib::Request&, httplib::Response&)> handler) {
  auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("otel-quickstart-cpp");

  svr.Get(pattern.c_str(), [pattern, handler, tracer](
                               const httplib::Request& req, httplib::Response& res) {
    auto start_time = std::chrono::steady_clock::now();

    ConstHttpHeadersCarrier carrier(req.headers);
    auto current_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
    auto parent_ctx =
        opentelemetry::context::propagation::GlobalTextMapPropagator::GetGlobalPropagator()->Extract(
            carrier, current_ctx);

    opentelemetry::trace::StartSpanOptions options;
    options.parent = parent_ctx;
    options.kind = opentelemetry::trace::SpanKind::kServer;

    std::string server_address = "127.0.0.1";
    int server_port = 8080;
    if (req.has_header("Host")) {
      ParseHostAndPort(req.get_header_value("Host"), server_address, server_port);
    }

    auto span = tracer->StartSpan("GET " + pattern, options);
    span->SetAttribute("http.request.method", "GET");
    span->SetAttribute("http.route", pattern);
    span->SetAttribute("url.path", req.path);
    span->SetAttribute("url.scheme", "http");
    span->SetAttribute("server.address", server_address);
    span->SetAttribute("server.port", server_port);
    span->SetAttribute("network.protocol.name", "http");
    span->SetAttribute("network.protocol.version", "1.1");
    opentelemetry::trace::Scope scope(span);

    handler(req, res);

    int status_code = res.status > 0 ? res.status : 200;
    span->SetAttribute("http.response.status_code", status_code);

    auto end_time = std::chrono::steady_clock::now();
    double duration_s = std::chrono::duration<double>(end_time - start_time).count();

    if (g_instruments && g_instruments->server_duration) {
      g_instruments->server_duration->Record(
          duration_s,
          {
              {"http.request.method", "GET"},
              {"http.route", pattern},
              {"http.response.status_code", status_code},
              {"network.protocol.name", "http"},
              {"network.protocol.version", "1.1"},
              {"server.address", server_address},
              {"server.port", server_port},
              {"url.scheme", "http"},
          },
          opentelemetry::context::RuntimeContext::GetCurrent());
    }

    span->End();
  });
}

}  // namespace opentelemetry_quickstart

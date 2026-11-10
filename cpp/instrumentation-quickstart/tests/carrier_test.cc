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

#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

#include "opentelemetry/context/propagation/global_propagator.h"
#include "opentelemetry/context/runtime_context.h"
#include "opentelemetry/trace/default_span.h"
#include "opentelemetry/trace/propagation/http_trace_context.h"
#include "opentelemetry/trace/span_context.h"
#include "opentelemetry/trace/span_id.h"
#include "opentelemetry/trace/trace_flags.h"
#include "opentelemetry/trace/trace_id.h"

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

void TestHttpHeadersCarrier() {
  httplib::Headers headers;
  opentelemetry_quickstart::HttpHeadersCarrier carrier(headers);

  // Set headers
  carrier.Set("content-type", "application/json");
  carrier.Set("x-custom-header", "12345");

  // Get headers
  ASSERT_EQ(carrier.Get("content-type"), "application/json");
  ASSERT_EQ(carrier.Get("Content-Type"), "application/json");
  ASSERT_EQ(carrier.Get("x-custom-header"), "12345");
  ASSERT_EQ(carrier.Get("X-Custom-Header"), "12345");
  ASSERT_EQ(carrier.Get("nonexistent"), "");

  // Iterate keys
  std::set<std::string> keys;
  carrier.Keys([&](opentelemetry::nostd::string_view key) {
    keys.insert(std::string(key));
    return true;
  });
  ASSERT_EQ(keys.count("content-type"), 1);
  ASSERT_EQ(keys.count("x-custom-header"), 1);
}

void TestConstHttpHeadersCarrier() {
  httplib::Headers headers;
  headers.emplace("authorization", "Bearer token");
  headers.emplace("user-agent", "test-agent");

  opentelemetry_quickstart::ConstHttpHeadersCarrier carrier(headers);

  ASSERT_EQ(carrier.Get("authorization"), "Bearer token");
  ASSERT_EQ(carrier.Get("Authorization"), "Bearer token");
  ASSERT_EQ(carrier.Get("user-agent"), "test-agent");
  ASSERT_EQ(carrier.Get("missing"), "");

  // Set should be a no-op on const carrier
  carrier.Set("new-header", "value");
  ASSERT_EQ(carrier.Get("new-header"), "");

  std::set<std::string> keys;
  carrier.Keys([&](opentelemetry::nostd::string_view key) {
    keys.insert(std::string(key));
    return true;
  });
  ASSERT_EQ(keys.count("authorization"), 1);
  ASSERT_EQ(keys.count("user-agent"), 1);
}

void TestTraceContextPropagation() {
  // Setup global propagator
  opentelemetry::context::propagation::GlobalTextMapPropagator::SetGlobalPropagator(
      opentelemetry::nostd::shared_ptr<
          opentelemetry::context::propagation::TextMapPropagator>(
          new opentelemetry::trace::propagation::HttpTraceContext()));

  const uint8_t trace_id_buf[16] = {1, 2,  3,  4,  5,  6,  7, 8,
                                    9, 10, 11, 12, 13, 14, 15, 16};
  const uint8_t span_id_buf[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  opentelemetry::trace::TraceId trace_id(trace_id_buf);
  opentelemetry::trace::SpanId span_id(span_id_buf);
  opentelemetry::trace::TraceFlags trace_flags(
      opentelemetry::trace::TraceFlags::kIsSampled);

  opentelemetry::trace::SpanContext span_context(trace_id, span_id, trace_flags,
                                                 false);
  ASSERT_TRUE(span_context.IsValid());

  auto current_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
  auto context = opentelemetry::trace::SetSpan(
      current_ctx,
      std::shared_ptr<opentelemetry::trace::Span>(
          new opentelemetry::trace::DefaultSpan(span_context)));

  // Inject into HttpHeadersCarrier
  httplib::Headers headers;
  opentelemetry_quickstart::HttpHeadersCarrier carrier(headers);
  opentelemetry::context::propagation::GlobalTextMapPropagator::
      GetGlobalPropagator()
          ->Inject(carrier, context);

  ASSERT_TRUE(headers.find("traceparent") != headers.end());
  std::string traceparent = headers.find("traceparent")->second;
  ASSERT_TRUE(traceparent.find("0102030405060708090a0b0c0d0e0f10") !=
              std::string::npos);
  ASSERT_TRUE(traceparent.find("0102030405060708") != std::string::npos);

  // Extract from ConstHttpHeadersCarrier
  opentelemetry_quickstart::ConstHttpHeadersCarrier const_carrier(headers);
  auto extract_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
  auto extracted_context =
      opentelemetry::context::propagation::GlobalTextMapPropagator::
          GetGlobalPropagator()
              ->Extract(const_carrier, extract_ctx);

  auto extracted_span = opentelemetry::trace::GetSpan(extracted_context);
  ASSERT_TRUE(extracted_span->GetContext().IsValid());
  ASSERT_EQ(extracted_span->GetContext().trace_id(), trace_id);
  ASSERT_EQ(extracted_span->GetContext().span_id(), span_id);
  ASSERT_TRUE(extracted_span->GetContext().IsSampled());
}

int main() {
  TestHttpHeadersCarrier();
  TestConstHttpHeadersCarrier();
  TestTraceContextPropagation();
  std::cout << "All carrier tests passed successfully!" << std::endl;
  return 0;
}

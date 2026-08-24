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

#include "gcp_logging.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/span.h"
#include "opentelemetry/trace/tracer.h"

namespace opentelemetry_quickstart {

namespace {

std::mutex g_log_mutex;

std::string GetCurrentTimestampRFC3339() {
  auto now = std::chrono::system_clock::now();
  auto now_time_t = std::chrono::system_clock::to_time_t(now);
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    now.time_since_epoch()) %
                1000000;

  std::tm tm_buf;
  gmtime_r(&now_time_t, &tm_buf);

  std::ostringstream oss;
  oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S") << "."
      << std::setfill('0') << std::setw(6) << now_us.count() << "Z";
  return oss.str();
}

}  // namespace

nlohmann::json FormatLogEntry(const std::string& message, const nlohmann::json& extra) {
  nlohmann::json log_entry;
  log_entry["timestamp"] = GetCurrentTimestampRFC3339();
  log_entry["severity"] = "INFO";
  log_entry["message"] = message;

  if (extra.is_object()) {
    for (auto it = extra.begin(); it != extra.end(); ++it) {
      log_entry[it.key()] = it.value();
    }
  }

  // Inject current OpenTelemetry trace context for Google Cloud Logging correlation
  auto current_span = opentelemetry::trace::Tracer::GetCurrentSpan();
  if (current_span && current_span->GetContext().IsValid()) {
    const auto& span_context = current_span->GetContext();

    char trace_id_hex[32];
    span_context.trace_id().ToLowerBase16(trace_id_hex);
    log_entry["logging.googleapis.com/trace"] = std::string(trace_id_hex, 32);

    char span_id_hex[16];
    span_context.span_id().ToLowerBase16(span_id_hex);
    log_entry["logging.googleapis.com/spanId"] = std::string(span_id_hex, 16);

    log_entry["logging.googleapis.com/trace_sampled"] = span_context.IsSampled();
  }

  return log_entry;
}

// [START opentelemetry_instrumentation_setup_logging]
void LogInfo(const std::string& message, const nlohmann::json& extra) {
  nlohmann::json log_entry = FormatLogEntry(message, extra);
  std::string line = log_entry.dump() + "\n";
  std::lock_guard<std::mutex> lock(g_log_mutex);
  std::cout << line << std::flush;
}

void LogWarn(const std::string& message, const nlohmann::json& extra) {
  nlohmann::json log_entry = FormatLogEntry(message, extra);
  log_entry["severity"] = "WARNING";
  std::string line = log_entry.dump() + "\n";
  std::lock_guard<std::mutex> lock(g_log_mutex);
  std::cout << line << std::flush;
}
// [END opentelemetry_instrumentation_setup_logging]

}  // namespace opentelemetry_quickstart

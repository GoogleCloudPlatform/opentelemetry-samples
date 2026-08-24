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

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include "httplib.h"
#include "gcp_logging.h"
#include "http_instrumentation.h"
#include "setup_opentelemetry.h"

namespace {

std::atomic<httplib::Server*> g_server{nullptr};

void SignalHandler(int) {
  auto* server = g_server.load();
  if (server != nullptr) {
    server->stop();
  }
}

int GetRandomSubRequests() {
  thread_local std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution<int> dist(3, 7);
  return dist(gen);
}

double GetRandomDuration() {
  thread_local std::mt19937 gen(std::random_device{}());
  std::uniform_real_distribution<double> dist(0.1, 0.2);
  return dist(gen);
}

}  // namespace

// [START opentelemetry_instrumentation_handle_multi]
// Handle an http request by making 3-7 http requests to the /single endpoint.
void HandleMulti(const httplib::Request& req, httplib::Response& res) {
  int sub_requests = GetRandomSubRequests();
  opentelemetry_quickstart::LogInfo("handle /multi request",
                                   {{"subRequests", sub_requests}});

  for (int i = 0; i < sub_requests; ++i) {
    if (!opentelemetry_quickstart::HttpClientGet("http://127.0.0.1:8080", "/single")) {
      opentelemetry_quickstart::LogWarn("subrequest to /single failed",
                                       {{"subRequestIndex", i}});
    }
  }

  res.set_content("ok", "text/plain");
}
// [END opentelemetry_instrumentation_handle_multi]

// [START opentelemetry_instrumentation_handle_single]
// Handle an http request by sleeping for 100-200 ms, and writing the duration as response.
void HandleSingle(const httplib::Request& req, httplib::Response& res) {
  double duration = GetRandomDuration();
  opentelemetry_quickstart::LogInfo("handle /single request",
                                   {{"duration", duration}});

  std::this_thread::sleep_for(std::chrono::duration<double>(duration));
  res.set_content("slept " + std::to_string(duration) + " seconds", "text/plain");
}
// [END opentelemetry_instrumentation_handle_single]

// [START opentelemetry_instrumentation_main]
int main(int argc, char** argv) {
  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);

  // Initialize OpenTelemetry C++ SDK
  opentelemetry_quickstart::SetupOpenTelemetry();

  // Create semantic convention HTTP metric instruments
  auto instruments = opentelemetry_quickstart::CreateHttpInstruments("otel-quickstart-cpp");
  opentelemetry_quickstart::SetGlobalHttpInstruments(instruments);

  httplib::Server svr;
  g_server.store(&svr);

  // Register instrumented endpoints
  opentelemetry_quickstart::RegisterInstrumentedGet(svr, "/multi", HandleMulti);
  opentelemetry_quickstart::RegisterInstrumentedGet(svr, "/single", HandleSingle);

  std::cout << "Starting server on 0.0.0.0:8080..." << std::endl;
  svr.listen("0.0.0.0", 8080);

  std::cout << "Shutting down..." << std::endl;
  g_server.store(nullptr);
  opentelemetry_quickstart::SetGlobalHttpInstruments(nullptr);
  opentelemetry_quickstart::CleanUpOpenTelemetry();
  return 0;
}
// [END opentelemetry_instrumentation_main]

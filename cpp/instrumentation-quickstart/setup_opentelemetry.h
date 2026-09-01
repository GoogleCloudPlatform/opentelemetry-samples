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

#ifndef CPP_INSTRUMENTATION_QUICKSTART_SETUP_OPENTELEMETRY_H_
#define CPP_INSTRUMENTATION_QUICKSTART_SETUP_OPENTELEMETRY_H_

namespace opentelemetry_quickstart {

// Initializes the OpenTelemetry C++ SDK with OTLP HTTP exporters for
// traces and metrics.
void SetupOpenTelemetry();

// Shuts down, flushes, and resets all registered OpenTelemetry providers and propagators.
void CleanUpOpenTelemetry();

}  // namespace opentelemetry_quickstart

#endif  // CPP_INSTRUMENTATION_QUICKSTART_SETUP_OPENTELEMETRY_H_

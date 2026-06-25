/*
 * Copyright 2026 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.cloud.opentelemetry.samples.otlpmetricexporterwrapper;

import io.opentelemetry.sdk.autoconfigure.spi.AutoConfigurationCustomizer;
import io.opentelemetry.sdk.autoconfigure.spi.AutoConfigurationCustomizerProvider;

/**
 * This class customizes the autoconfiguration to replace the configured OTLP Http and gRPC
 * exporters with a {@link PrefixedMetricExporter}.
 */
public class OtlpMetricCustomizer implements AutoConfigurationCustomizerProvider {

  @Override
  public void customize(AutoConfigurationCustomizer autoConfigurationCustomizer) {
    autoConfigurationCustomizer.addMetricExporterCustomizer(
        (metricExporter, configProperties) -> {
          if (metricExporter.getClass().getName().contains("Otlp")) {
            return new PrefixedMetricExporter(metricExporter, "workload.googleapis.com/");
          }
          return metricExporter;
        });
  }

  @Override
  public int order() {
    // The customizer will be attempted to be applied towards the end.
    return Integer.MAX_VALUE;
  }
}

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
package com.google.cloud.opentelemetry.samples.otlpiap;

import io.opentelemetry.api.metrics.LongCounter;
import io.opentelemetry.api.metrics.Meter;
import io.opentelemetry.api.trace.Span;
import io.opentelemetry.api.trace.Tracer;
import io.opentelemetry.context.Scope;
import io.opentelemetry.sdk.OpenTelemetrySdk;
import io.opentelemetry.sdk.autoconfigure.AutoConfiguredOpenTelemetrySdk;
import java.util.concurrent.TimeUnit;

public class OTLPIAPExample {
  private static final String INSTRUMENTATION_SCOPE_NAME = OTLPIAPExample.class.getName();

  public static void main(String[] args) throws Exception {
    // Enable detailed logging for OpenTelemetry to debug export issues
    setupDebugLogging(true);

    String resolvedCollectorUrl = System.getProperty("otel.exporter.otlp.endpoint");
    if (resolvedCollectorUrl == null || resolvedCollectorUrl.isEmpty()) {
      resolvedCollectorUrl = System.getenv("OTEL_EXPORTER_OTLP_ENDPOINT");
    }
    final String collectorUrl = resolvedCollectorUrl;

    String resolvedIapClientId = System.getProperty("google.auth.id.token.audience");
    if (resolvedIapClientId == null || resolvedIapClientId.isEmpty()) {
      resolvedIapClientId = System.getenv("GOOGLE_AUTH_ID_TOKEN_AUDIENCE");
    }
    final String iapClientId = resolvedIapClientId;

    if (collectorUrl == null || collectorUrl.isEmpty()) {
      System.err.println(
          "Error: Collector URL is not set (via otel.exporter.otlp.endpoint property or OTEL_EXPORTER_OTLP_ENDPOINT env var).");
      System.exit(1);
    }
    if (iapClientId == null || iapClientId.isEmpty()) {
      System.err.println(
          "Error: IAP Client ID is not set (via google.auth.id.token.audience property or GOOGLE_AUTH_ID_TOKEN_AUDIENCE env var).");
      System.exit(1);
    }

    System.out.println(
        "Initializing OpenTelemetry SDK with IAP authentication (using GCP Auth Extension)...");
    System.out.println("Collector URL: " + collectorUrl);
    System.out.println("IAP Client ID: " + iapClientId);

    // Initialize using Autoconfigure. The GCP Auth Extension will be loaded from
    // the classpath.
    OpenTelemetrySdk openTelemetrySdk =
        AutoConfiguredOpenTelemetrySdk.initialize().getOpenTelemetrySdk();

    // Generate sample traces and metrics
    Tracer tracer = openTelemetrySdk.getTracer(INSTRUMENTATION_SCOPE_NAME);
    Meter meter = openTelemetrySdk.getMeter(INSTRUMENTATION_SCOPE_NAME);

    System.out.println("Sending test span...");
    Span span = tracer.spanBuilder("iap-test-span").startSpan();
    try (Scope scope = span.makeCurrent()) {
      span.setAttribute("example.attribute", "direct-iap-test");
      System.out.println("Span is current.");
      Thread.sleep(500);
    } finally {
      span.end();
      System.out.println("Span ended.");
    }

    System.out.println("Sending test metric...");
    LongCounter counter =
        meter
            .counterBuilder("iap_test_counter")
            .setDescription("A counter to test metrics export through IAP")
            .setUnit("1")
            .build();
    counter.add(1);

    // Clean shutdown to flush all buffered metrics and spans
    System.out.println("Flushing and shutting down OpenTelemetry SDK...");
    openTelemetrySdk.getSdkTracerProvider().shutdown().join(10, TimeUnit.SECONDS);
    openTelemetrySdk.getSdkMeterProvider().shutdown().join(10, TimeUnit.SECONDS);
    System.out.println("Shutdown complete. Telemetry exported successfully!");
  }

  private static void setupDebugLogging(boolean enableDebugLogs) {
    java.util.logging.Logger otelLogger = java.util.logging.Logger.getLogger("io.opentelemetry");
    if (enableDebugLogs) {
      otelLogger.setLevel(java.util.logging.Level.ALL);
      java.util.logging.ConsoleHandler handler = new java.util.logging.ConsoleHandler();
      handler.setLevel(java.util.logging.Level.ALL);
      otelLogger.addHandler(handler);
      otelLogger.setUseParentHandlers(false);
    } else {
      otelLogger.setLevel(java.util.logging.Level.INFO);
      otelLogger.setUseParentHandlers(true);
    }
  }
}

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

import static java.util.Arrays.stream;

import com.google.auth.oauth2.GoogleCredentials;
import com.google.auth.oauth2.IdTokenCredentials;
import com.google.auth.oauth2.IdTokenProvider;
import io.opentelemetry.api.metrics.LongCounter;
import io.opentelemetry.api.metrics.Meter;
import io.opentelemetry.api.trace.Span;
import io.opentelemetry.api.trace.Tracer;
import io.opentelemetry.context.Scope;
import io.opentelemetry.exporter.otlp.http.metrics.OtlpHttpMetricExporter;
import io.opentelemetry.exporter.otlp.http.trace.OtlpHttpSpanExporter;
import io.opentelemetry.sdk.OpenTelemetrySdk;
import io.opentelemetry.sdk.autoconfigure.AutoConfiguredOpenTelemetrySdk;
import java.io.IOException;

import java.security.cert.CertPathValidatorException.Reason;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.function.Supplier;
import static java.util.stream.Collectors.joining;
import static java.util.stream.Collectors.toMap;
import java.util.Arrays;
import java.util.Objects;

public class OTLPIAPExample {
  private static final String INSTRUMENTATION_SCOPE_NAME = OTLPIAPExample.class.getName();
  private static final String IAM_SCOPE = "https://www.googleapis.com/auth/iam";
  private static final String EMAIL_SCOPE = "https://www.googleapis.com/auth/userinfo.email";
  private static final String CLOUD_PLATFORM_SCOPE = "https://www.googleapis.com/auth/cloud-platform";

  public static void main(String[] args) throws Exception {
    // Enable detailed logging for OpenTelemetry to debug export issues
    java.util.logging.Logger otelLogger = java.util.logging.Logger.getLogger("io.opentelemetry");
    otelLogger.setLevel(java.util.logging.Level.ALL);
    java.util.logging.ConsoleHandler handler = new java.util.logging.ConsoleHandler();
    handler.setLevel(java.util.logging.Level.ALL);
    otelLogger.addHandler(handler);
    otelLogger.setUseParentHandlers(false);

    String resolvedCollectorUrl = System.getProperty("otel.exporter.otlp.endpoint");
    if (resolvedCollectorUrl == null || resolvedCollectorUrl.isEmpty()) {
      resolvedCollectorUrl = System.getenv("COLLECTOR_URL");
    }
    final String collectorUrl = resolvedCollectorUrl;

    String resolvedIapClientId = System.getProperty("iap.client.id");
    if (resolvedIapClientId == null || resolvedIapClientId.isEmpty()) {
      resolvedIapClientId = System.getenv("IAP_CLIENT_ID");
    }
    final String iapClientId = resolvedIapClientId;

    if (collectorUrl == null || collectorUrl.isEmpty()) {
      System.err.println("Error: Collector URL is not set (via otel.exporter.otlp.endpoint property or COLLECTOR_URL env var).");
      System.exit(1);
    }
    if (iapClientId == null || iapClientId.isEmpty()) {
      System.err.println("Error: IAP Client ID is not set (via iap.client.id property or IAP_CLIENT_ID env var).");
      System.exit(1);
    }

    System.out.println("Initializing OpenTelemetry SDK with IAP authentication...");
    System.out.println("Collector URL: " + collectorUrl);
    System.out.println("IAP Client ID: " + iapClientId);

    // 1. Setup ID Token credentials using Application Default Credentials (ADC)
    GoogleCredentials credentials = GoogleCredentials.getApplicationDefault();

    if (credentials.createScopedRequired()) {
      credentials = credentials.createScoped(Arrays.asList(IAM_SCOPE, EMAIL_SCOPE, CLOUD_PLATFORM_SCOPE, "openid"));
    }

    if (!(credentials instanceof IdTokenProvider)) {
      throw new IllegalStateException(
          "Default credentials do not support generating ID tokens. "
              + "Please ensure you have authenticated with a Service Account, or configured impersonation via gcloud.");
    }

    IdTokenCredentials idTokenCredentials = IdTokenCredentials.newBuilder()
        .setIdTokenProvider((IdTokenProvider) credentials)
        .setTargetAudience(iapClientId)
        .build();

    // Supplier that will automatically refresh and supply the authorization header
    Supplier<Map<String, String>> headerSupplier = () -> {
      try {
        idTokenCredentials.refreshIfExpired();
        return getRequiredHeaderMap(idTokenCredentials);
      } catch (IOException e) {
        throw new RuntimeException("Failed to refresh ID token for IAP", e);
      }
    };

    // 2. Configure AutoConfiguredOpenTelemetrySdk with custom HTTP exporters that include IAP headers
    OpenTelemetrySdk openTelemetrySdk = AutoConfiguredOpenTelemetrySdk.builder()
        .addSpanExporterCustomizer((existingExporter, config) -> 
            OtlpHttpSpanExporter.builder()
                .setEndpoint(collectorUrl + "/v1/traces")
                .setHeaders(headerSupplier)
                .build()
        )
        .addMetricExporterCustomizer((existingExporter, config) -> 
            OtlpHttpMetricExporter.builder()
                .setEndpoint(collectorUrl + "/v1/metrics")
                .setHeaders(headerSupplier)
                .build()
        )
        .build()
        .getOpenTelemetrySdk();

    // 3. Generate sample traces and metrics
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
    LongCounter counter = meter
        .counterBuilder("iap_test_counter")
        .setDescription("A counter to test metrics export through IAP")
        .setUnit("1")
        .build();
    counter.add(1);

    // 4. Clean shutdown to flush all buffered metrics and spans
    System.out.println("Flushing and shutting down OpenTelemetry SDK...");
    openTelemetrySdk.getSdkTracerProvider().shutdown().join(10, TimeUnit.SECONDS);
    openTelemetrySdk.getSdkMeterProvider().shutdown().join(10, TimeUnit.SECONDS);
    System.out.println("Shutdown complete. Telemetry exported successfully!");
  }

  private static Map<String, String> getRequiredHeaderMap(IdTokenCredentials credentials) {
    Map<String, List<String>> gcpHeaders;
    try {
      // this also refreshes the credentials, if required
      gcpHeaders = credentials.getRequestMetadata();
    } catch (IOException e) {
      throw new RuntimeException("Error getting request metadata from ADC", e);
    }
    Map<String, String> flattenedHeaders = gcpHeaders.entrySet().stream()
        .collect(
            toMap(
                Map.Entry::getKey,
                entry -> entry.getValue().stream()
                    .filter(Objects::nonNull) // Filter nulls
                    .filter(s -> !s.isEmpty()) // Filter empty strings
                    .collect(joining(","))));

    // flattenedHeaders.putIfAbsent("Proxy-Authorization", "Bearer " +
    // credentials.getAccessToken().getTokenValue());
    System.out.println("Request Headers: " + flattenedHeaders);
    return flattenedHeaders;
  }
}

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
package com.google.cloud.opentelemetry.samples.otlplog;

import io.opentelemetry.api.trace.Span;
import io.opentelemetry.api.trace.Tracer;
import io.opentelemetry.context.Scope;
import io.opentelemetry.instrumentation.log4j.appender.v2_17.OpenTelemetryAppender;
import io.opentelemetry.sdk.OpenTelemetrySdk;
import io.opentelemetry.sdk.autoconfigure.AutoConfiguredOpenTelemetrySdk;
import io.opentelemetry.sdk.common.CompletableResultCode;
import java.util.Random;
import java.util.concurrent.TimeUnit;
import org.apache.logging.log4j.CloseableThreadContext;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.message.StringMapMessage;

/**
 * Demonstrates the recommended way to export application logs to Google Cloud Observability with
 * OpenTelemetry OTLP exporters.
 *
 * <p>Application code keeps using a standard Java logging library (Log4j2 here) instead of calling
 * the OpenTelemetry Logs API directly. The {@code OpenTelemetryAppender} log bridge converts Log4j2
 * log events into OpenTelemetry {@code LogRecord}s, which the SDK then batches and exports over
 * OTLP.
 */
public class OTLPLogExample {
  private static final String INSTRUMENTATION_SCOPE_NAME = OTLPLogExample.class.getName();
  private static final Random RANDOM = new Random();
  private static final Logger logger = LogManager.getLogger(OTLPLogExample.class);

  private static OpenTelemetrySdk openTelemetrySdk;

  private static void myUseCase(String description) {
    Tracer tracer = openTelemetrySdk.getTracer(INSTRUMENTATION_SCOPE_NAME);
    Span span = tracer.spanBuilder("myUseCase").startSpan();

    // Values placed in the Log4j ThreadContext become log record attributes,
    // because the appender
    // is configured with contextDataAttributesIncluded="*".
    try (Scope scope = span.makeCurrent();
        CloseableThreadContext.Instance ignored =
            CloseableThreadContext.put("usecase.name", description)) {
      // While the span above is current, the log records emitted here are
      // automatically correlated
      // with it: the bridge stamps them with the active trace and span IDs.
      logger.info("Starting use case: {}", description);

      // Do some work for the use case and emit progress logs.
      for (int i = 0; i < 3; i++) {
        doWork(String.format("%s - Work #%d", description, (i + 1)));
      }

      logger.info("Completed use case: {}", description);
    } finally {
      span.end();
    }
  }

  private static void doWork(String description) {
    // Simulate work: this could be simulating a network request or an expensive
    // disk operation
    int randomSleep = RANDOM.nextInt(5) * 100;
    try {
      Thread.sleep(100 + randomSleep);
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
      throw new RuntimeException(e);
    }

    logger.debug(
        new StringMapMessage()
            .with("message", "Completed work item: " + description)
            .with("work.description", description)
            .with("work.duration_ms", String.valueOf(100 + randomSleep)));
  }

  private static void logErrorExample() {
    try {
      throw new IllegalStateException("Simulated transient failure for demonstration");
    } catch (Exception e) {
      // Passing the throwable as the last argument lets the bridge record the
      // exception type,
      // message and stack trace on the exported log record.
      logger.warn("Handled exception during sample execution: {}", e.getMessage(), e);
    }
  }

  public static void main(String[] args) {
    openTelemetrySdk = AutoConfiguredOpenTelemetrySdk.initialize().getOpenTelemetrySdk();

    // Find the OpenTelemetryAppender declared in log4j2.xml and attach the SDK to
    // it. Until this runs, the appender has nowhere to send log records.
    OpenTelemetryAppender.install(openTelemetrySdk);

    // Application-specific logic emitting log records
    myUseCase("One");
    myUseCase("Two");
    logErrorExample();

    logger.info("Logs emitted. Flushing and shutting down SDK...");

    // Flush all buffered logs and shut down the SDK providers
    CompletableResultCode completableResultCode = openTelemetrySdk.shutdown();
    // Wait till export finishes
    completableResultCode.join(10000, TimeUnit.MILLISECONDS);

    System.out.println("SDK shut down complete.");
  }
}

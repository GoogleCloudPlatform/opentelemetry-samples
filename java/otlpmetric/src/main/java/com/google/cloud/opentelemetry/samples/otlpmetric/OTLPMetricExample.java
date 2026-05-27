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
package com.google.cloud.opentelemetry.samples.otlpmetric;

import io.opentelemetry.api.metrics.LongCounter;
import io.opentelemetry.sdk.OpenTelemetrySdk;
import io.opentelemetry.sdk.autoconfigure.AutoConfiguredOpenTelemetrySdk;
import io.opentelemetry.sdk.common.CompletableResultCode;
import java.util.Random;
import java.util.concurrent.TimeUnit;

public class OTLPMetricExample {
  private static final String INSTRUMENTATION_SCOPE_NAME = OTLPMetricExample.class.getName();
  private static final Random RANDOM = new Random();

  private static OpenTelemetrySdk openTelemetrySdk;

  private static void myUseCase() {
    LongCounter counter =
        openTelemetrySdk
            .getMeter(INSTRUMENTATION_SCOPE_NAME)
            .counterBuilder("example_counter")
            .setDescription("Processed jobs")
            .setUnit("1")
            .build();
    doWork(counter);
  }

  private static void doWork(LongCounter counter) {
    try {
      for (int i = 0; i < 10; i++) {
        counter.add(RANDOM.nextInt(100));
        Thread.sleep(RANDOM.nextInt(1000));
      }
    } catch (InterruptedException e) {
      throw new RuntimeException(e);
    }
  }

  public static void main(String[] args) {
    // Configure the OpenTelemetry pipeline with Auto configuration
    openTelemetrySdk = AutoConfiguredOpenTelemetrySdk.initialize().getOpenTelemetrySdk();

    // Application-specific logic
    myUseCase();

    // Flush all buffered metrics
    CompletableResultCode completableResultCode = openTelemetrySdk.getSdkMeterProvider().shutdown();
    // wait till export finishes
    completableResultCode.join(10000, TimeUnit.MILLISECONDS);
  }
}

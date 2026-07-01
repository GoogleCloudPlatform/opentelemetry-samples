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

import io.opentelemetry.sdk.common.CompletableResultCode;
import io.opentelemetry.sdk.common.export.MemoryMode;
import io.opentelemetry.sdk.metrics.Aggregation;
import io.opentelemetry.sdk.metrics.InstrumentType;
import io.opentelemetry.sdk.metrics.data.MetricData;
import io.opentelemetry.sdk.metrics.export.MetricExporter;
import java.util.Collection;
import java.util.List;
import java.util.stream.Collectors;

public class PrefixedMetricExporter implements MetricExporter {
  private final MetricExporter delegate;
  private final String prefix;

  public PrefixedMetricExporter(MetricExporter delegate, String prefix) {
    this.delegate = delegate;
    this.prefix = prefix;
  }

  @Override
  public CompletableResultCode export(Collection<MetricData> metrics) {
    List<MetricData> prefixedMetrics =
        metrics.stream()
            .map(metric -> new PrefixedMetricData(metric, prefix))
            .collect(Collectors.toList());
    return delegate.export(prefixedMetrics);
  }

  @Override
  public CompletableResultCode flush() {
    return delegate.flush();
  }

  @Override
  public CompletableResultCode shutdown() {
    return delegate.shutdown();
  }

  @Override
  public void close() {
    delegate.close();
  }

  @Override
  public Aggregation getDefaultAggregation(InstrumentType instrumentType) {
    return delegate.getDefaultAggregation(instrumentType);
  }

  @Override
  public io.opentelemetry.sdk.metrics.data.AggregationTemporality getAggregationTemporality(
      InstrumentType instrumentType) {
    return delegate.getAggregationTemporality(instrumentType);
  }

  @Override
  public MemoryMode getMemoryMode() {
    return delegate.getMemoryMode();
  }
}

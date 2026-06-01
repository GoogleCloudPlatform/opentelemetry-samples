# OpenTelemetry Samples for Google Cloud

> [!NOTE]
> This is not an officially supported Google product. This project is not eligible for the [Google Open Source Software Vulnerability Rewards Program](https://bughunters.google.com/open-source-security).

This repository contains code samples demonstrating how to instrument applications using [OpenTelemetry](https://opentelemetry.io/) and export the telemetry data to [Google Cloud Observability](https://cloud.google.com/products/observability).

## Overview

OpenTelemetry is a collection of tools, APIs, and SDKs used to instrument, generate, collect, and export telemetry data (metrics, logs, and traces) to help you analyze your software's performance and behavior.

This project provides practical examples across multiple programming languages to help you get started with OpenTelemetry on Google Cloud.

## Repository Structure

- **[java](./java)**: Java samples.
  - **[autoconf-auth-extension](./java/autoconf-auth-extension)**: Example showing OTLP exporter with GCP Auth Extension for traces and metrics.
  - **[otlp-spring](./java/otlp-spring)**: Example showcasing OTLP trace ingest on GCP and Google Auth in a Spring Boot App.
  - **[otlpmetric](./java/otlpmetric)**: Example for OTLP Metric exporter.
  - **[otlpmetrics-function](./java/otlpmetrics-function)**: Example showing how to export OpenTelemetry metrics from a Google Cloud Run Function to Google Managed Prometheus using OpenTelemetry Collector running as a sidecar.
  - **[otlptrace](./java/otlptrace)**: Example showing OTLP exporter being used for traces export to GCP.
  - **[resource](./java/resource)**: Examples for showing resource detection in various GCP environments.
- **[golang](./golang)**: Go samples.
  - **[metric/otlpgrpc](./golang/metric/otlpgrpc)**: Example for OTLP Metric exporter via gRPC.
  - **[trace/otlpgrpc](./golang/trace/otlpgrpc)**: Example showing OTLP exporter being used for traces export to GCP via gRPC.
  - **[trace/otlphttp](./golang/trace/otlphttp)**: Example showing OTLP exporter being used for traces export to GCP via HTTP.
- **[javascript](./javascript)**: JavaScript samples (coming soon).
- **[python](./python)**: Python samples.
  - **[otlpmetric](./python/otlpmetric)**: Example for OTLP Metric exporter.
  - **[otlptrace](./python/otlptrace)**: Example showing OTLP exporter being used for traces export to GCP.

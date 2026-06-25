# OTLP Metric Exporter Wrapper Example

This example demonstrates how to export OTLP metrics to Google Cloud Platform (GCP) and shows how you can wrap existing configured OTLP Metric exporters in a custom Metric exporter that attaches a prefix to all exported metric names. 

It leverages the OpenTelemetry Auto-Configuration SPI (`AutoConfigurationCustomizerProvider`) to intercept the default OTLP exporter and wrap it with a `PrefixedMetricExporter`, which prepends `custom_prefix_` to all metric names.

## Setup

Run this sample to connect to an endpoint that is protected by GCP authentication.

First, get GCP credentials on your machine:

```shell
gcloud auth application-default login
```
Executing this command will save your application credentials to default path which will depend on the type of machine -
 - Linux, macOS: `$HOME/.config/gcloud/application_default_credentials.json`
 - Windows: `%APPDATA%\gcloud\application_default_credentials.json`

Next, update [`build.gradle`](build.gradle) to set the following:

```
	'-Dgoogle.cloud.project=<YOUR_PROJECT_ID>,
	# Optional - if you want to export using gRPC protocol
	'-Dotel.exporter.otlp.protocol=grpc',
```

Finally, to run the sample from the `java` directory:
```
./gradlew :otlpmetric-exporter-wrapper:run
```

# OTLP Logs with Google Cloud Telemetry API Example

This sample demonstrates how to export OpenTelemetry (OTLP) logs directly to Google Cloud Observability using the Google Cloud Telemetry API (`https://telemetry.googleapis.com`).

The Telemetry API provides native OTLP ingestion for Google Cloud, allowing applications to use standard OpenTelemetry SDKs and OTLP exporters instead of vendor-specific logging agents or exporters.

Rather than calling the OpenTelemetry Logs API from application code, this sample follows the recommended approach: keep using a standard Java logging library — **Log4j2** here — and attach an OpenTelemetry **log appender bridge** to it. The OpenTelemetry SDK is configured with a `LogRecordProcessor` and a `LogRecordExporter`, and the bridge feeds Log4j2 log events into that pipeline.

## Prerequisites

1. **Enable the Google Cloud Telemetry API**:
   ```shell
   gcloud services enable telemetry.googleapis.com
   ```

2. **Obtain Google Cloud Application Default Credentials (ADC)**:
   ```shell
   gcloud auth application-default login
   ```
   Executing this command saves your application credentials to the default path:
   - Linux, macOS: `$HOME/.config/gcloud/application_default_credentials.json`
   - Windows: `%APPDATA%\gcloud\application_default_credentials.json`

   Ensure the authenticated principal or service account has the **Cloud Telemetry Logs Writer** (`roles/telemetry.logsWriter`) and **Cloud Telemetry Traces Writer** (`roles/telemetry.tracesWriter`, for emitting correlated traces) roles.

3. **Set the Google Cloud Project ID**:
   ```shell
   export GOOGLE_CLOUD_PROJECT="your-gcp-project-id"
   ```
   You can also set this explicitly by uncommenting the `-Dgoogle.cloud.project` JVM argument in [`build.gradle`](build.gradle) and filling in your project ID.

## Running the Sample

To run the sample from the `java` directory:

```shell
./gradlew :otlplog:run
```

Running this sample will initialize the OpenTelemetry SDK using Autoconfigure, install the OpenTelemetry Log4j2 appender, emit log records at different severity levels (`INFO`, `DEBUG`, `WARN`) using plain Log4j2 API calls, flush them to Google Cloud Observability via OTLP over HTTP (`http/protobuf`), and gracefully shut down the SDK.

## How Logging Is Wired Up

Application code in [`OTLPLogExample.java`](src/main/java/com/google/cloud/opentelemetry/samples/otlplog/OTLPLogExample.java) only uses Log4j2:

```java
private static final Logger logger = LogManager.getLogger(OTLPLogExample.class);
...
logger.info("Starting use case: {}", description);
```

The path from a Log4j2 call to Google Cloud looks like this:

```
Log4j2 Logger -> OpenTelemetryAppender -> SDK LoggerProvider
              -> BatchLogRecordProcessor -> OtlpHttpLogRecordExporter
              -> https://telemetry.googleapis.com/v1/logs
```

### Instrumentation libraries

Two libraries from the [opentelemetry-java-instrumentation](https://github.com/open-telemetry/opentelemetry-java-instrumentation) repository are used:

| Library | Purpose |
| --- | --- |
| [`opentelemetry-log4j-appender-2.17`](https://github.com/open-telemetry/opentelemetry-java-instrumentation/tree/main/instrumentation/log4j/log4j-appender-2.17/library) | The log bridge. Provides the `<OpenTelemetry>` Log4j2 appender that converts Log4j2 log events into OpenTelemetry log records. |
| [`opentelemetry-log4j-context-data-2.17-autoconfigure`](https://github.com/open-telemetry/opentelemetry-java-instrumentation/tree/main/instrumentation/log4j/log4j-context-data/log4j-context-data-2.17/library-autoconfigure) | A Log4j2 `ContextDataProvider` that injects `trace_id`, `span_id` and `trace_flags` from the active span into Log4j's context data (MDC), so they can be rendered by any layout. It is loaded automatically via the Log4j2 SPI, hence `runtimeOnly`. |

### Appender configuration

The appender is declared in [`log4j2.xml`](src/main/resources/log4j2.xml):

```xml
<OpenTelemetry name="OpenTelemetryAppender"
    captureCodeAttributes="true"
    captureMarkerAttribute="true"
    captureTemplate="true"
    captureArguments="true"
    mapMessageAttributesIncluded="*"
    contextDataAttributesIncluded="*"
    contextDataAttributesExcluded="trace_id,span_id,trace_flags"/>
```

- `captureCodeAttributes` records the class, method, file and line of the logging call site.
- `captureMarkerAttribute` records the Log4j2 `Marker` name under the `log4j.marker` attribute when a marker is attached to a log event.
- `captureTemplate` / `captureArguments` preserve the unformatted message template and its arguments as attributes, which makes logs easier to group and query.
- `mapMessageAttributesIncluded="*"` turns Log4j2 `MapMessage` entries into log record attributes. The sample uses a `StringMapMessage` in `doWork` to emit structured key/value data.
- `contextDataAttributesIncluded="*"` turns Log4j `ThreadContext` (MDC) entries into log record attributes. The sample uses `CloseableThreadContext` to attach `usecase.name` to every record within a use case.
- `contextDataAttributesExcluded` drops the trace correlation keys injected by the context-data library, since the appender already sets trace context as first-class fields on the log record — keeping them would duplicate the data as attributes.

> [!IMPORTANT]
> The appender needs an `OpenTelemetry` instance, which cannot be supplied from XML. It must be installed programmatically once the SDK is built:
> ```java
> OpenTelemetryAppender.install(openTelemetrySdk);
> ```
> Log events emitted before this call are buffered by the appender (up to `numLogsCapturedBeforeOtelInstall`, 1000 by default) and replayed afterwards, so early startup logs are not lost.

### Trace correlation

`myUseCase` starts a span and makes it current before logging. Because the log records are emitted inside that scope, the bridge stamps them with the active trace and span IDs, letting Cloud Logging correlate logs with traces. The console appender renders the same values from MDC:

```
%d{HH:mm:ss.SSS} [%t] %-5level %logger{36} trace_id=%X{trace_id} span_id=%X{span_id} trace_flags=%X{trace_flags} - %msg%n
```

## Configuration Details

This sample uses OpenTelemetry SDK Autoconfigure. The default JVM arguments configured in [`build.gradle`](build.gradle) are:

- `otel.exporter.otlp.endpoint=https://telemetry.googleapis.com`: The Google Cloud Telemetry API endpoint. The SDK automatically routes logs to `/v1/logs`.
- `otel.logs.exporter=otlp`: Enables the OTLP exporter for logs.
- `otel.traces.exporter=otlp`: Enables the OTLP exporter for traces so that the spans correlated with the emitted logs are also exported to Cloud Trace.
- `otel.metrics.exporter=none`: Disables metric export in this standalone logs sample.
- `otel.service.name=otlplog-example`: Specifies the service name attribute associated with the emitted logs.
- `otel.exporter.otlp.protocol=http/protobuf`: Uses HTTP/Protobuf for export. You can also switch to `grpc` if desired.
- `otel.java.global-autoconfigure.enabled=true`: Enables global OpenTelemetry SDK autoconfiguration.

Autoconfigure installs a `BatchLogRecordProcessor` wired to the OTLP `LogRecordExporter`. You can tune it with the standard `otel.blrp.*` properties (for example `otel.blrp.schedule.delay`), or build the pipeline by hand with `SdkLoggerProvider.builder().addLogRecordProcessor(...)` if you prefer explicit configuration over autoconfiguration.

## Viewing Logs in Google Cloud Console

Logs ingested through the Telemetry API default to the `otlp` log name when the OTLP `LogRecord` does not specify an `event_name` or a log-name attribute (such as `log.name` or `gcp.log_name`). After running the sample, view them in the [Google Cloud Logs Explorer](https://console.cloud.google.com/logs/query):

1. Select your project in Google Cloud Console.
2. In the query box, enter:
   ```
   logName="projects/YOUR_PROJECT_ID/logs/otlp"
   ```
   or filter by service name:
   ```
   otel.resource.attributes."service.name"="otlplog-example"
   ```

Or from the command line:

```shell
gcloud logging read 'logName="projects/YOUR_PROJECT_ID/logs/otlp"' --limit=10 --freshness=10m
```

### How the log record maps onto a Cloud Logging entry

| OpenTelemetry log record | Cloud Logging entry |
| --- | --- |
| Body | `textPayload` (for string/array bodies) or `jsonPayload` (for key-value list bodies) |
| Severity | `severity` |
| Trace / span ID | `trace`, `spanId`, `traceSampled` |
| Log record attributes | `labels` |
| Resource attributes | `otel.resource.attributes` |
| Instrumentation scope | `otel.scope` (e.g., `otel.scope.name`, `otel.scope.version`) |

With the appender settings used here, a single Log4j2 call produces labels such as:

- `code.namespace`, `code.function`, `code.filepath`, `code.lineno` — from `captureCodeAttributes`
- `log.body.template`, `log.body.parameters` — from `captureTemplate` / `captureArguments`
- `usecase.name` — from `CloseableThreadContext` (MDC)
- `log4j.map_message.work.description`, `log4j.map_message.work.duration_ms` — from the `StringMapMessage`
- `exception.type`, `exception.message`, `exception.stacktrace` — when a `Throwable` is logged


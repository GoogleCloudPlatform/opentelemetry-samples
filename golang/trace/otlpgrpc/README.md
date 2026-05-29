# OTLP Trace with Google Auth Example

Run this sample to connect to an endpoint that is protected by GCP authentication and export traces.

#### Prerequisites

Get Google credentials on your machine:

```sh
gcloud auth application-default login
```

#### Run the Sample

```sh
# export necessary OTEL environment variables
export PROJECT_ID=<project-id>
export OTEL_EXPORTER_OTLP_ENDPOINT=<endpoint>
export OTEL_RESOURCE_ATTRIBUTES="gcp.project_id=$PROJECT_ID,service.name=otlp-sample,service.instance.id=1"
export OTEL_EXPORTER_OTLP_HEADERS=X-Goog-User-Project=$PROJECT_ID

# from the golang/trace/otlpgrpc directory
go run .
```

#### Options

- `-keepRunning`: Set to `true` to generate spans at a fixed rate indefinitely. Default is `false`.

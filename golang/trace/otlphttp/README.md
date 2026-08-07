# OTLP Trace with Google Auth Example (HTTP)

Run this sample to connect to an endpoint that is protected by GCP authentication using HTTP and export traces.

#### Prerequisites

Get Google credentials on your machine:

```sh
gcloud auth application-default login
```

#### Run the Sample

```sh
# export necessary environment variables
export GCLOUD_PROJECT=<project-id>
export OTEL_EXPORTER_OTLP_ENDPOINT="https://telemetry.googleapis.com"
export OTEL_RESOURCE_ATTRIBUTES="gcp.project_id=$GCLOUD_PROJECT,service.name=otlp-sample,service.instance.id=1"

# from the golang/trace/otlphttp directory
go run .
```

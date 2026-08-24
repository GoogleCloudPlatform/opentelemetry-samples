# OpenTelemetry C++ instrumentation example

This sample is a C++ application instrumented with OpenTelemetry. It uses docker compose to run the application and send it requests.

This sample is configured to use standard OpenTelemetry OTLP/HTTP exporters to export traces and metrics to Google Cloud via the OTel Collector. Logs are written to stdout and shared files in Google Cloud structured JSON format (correlated with active OpenTelemetry trace context) and ingested via the OpenTelemetry Collector's filelog receiver. It does not rely on any custom Google Cloud exporter artifacts.

The C++ code is an HTTP application with two endpoints:
- `/multi` makes a few requests to `/single` on localhost
- `/single` sleeps for a short time to simulate work

Docker compose also runs the OpenTelemetry collector, set up to receive telemetry from the C++ application and parse its logs from a shared volume. Finally, a loadgen container sends requests to the C++ app.

## Permissions

This sample writes to Cloud Logging, Cloud Monitoring, and Cloud Trace. Grant yourself the following roles to run the example:
- `roles/logging.logWriter` – see https://cloud.google.com/logging/docs/access-control#permissions_and_roles
- `roles/monitoring.metricWriter` – see https://cloud.google.com/monitoring/access-control#predefined_roles
- `roles/cloudtrace.agent` – see https://cloud.google.com/trace/docs/iam#trace-roles

## Running the example

### Cloud Shell or GCE

```sh
cd cpp/instrumentation-quickstart
docker compose up --abort-on-container-exit
```

### Locally with Application Default Credentials

First create local credentials by running the following command:

```sh
gcloud auth application-default login
```

Executing this command will save your application credentials to:
- Linux, macOS: `$HOME/.config/gcloud/application_default_credentials.json`
- Windows: `%APPDATA%\gcloud\application_default_credentials.json`

Next, export the credentials path:

```sh
export GOOGLE_APPLICATION_CREDENTIALS=$HOME/.config/gcloud/application_default_credentials.json
```

Then run the example:

```sh
cd cpp/instrumentation-quickstart

# Lets collector read mounted config
export USERID="$(id -u)"
# Specify the project ID
export GOOGLE_CLOUD_PROJECT=<your project id>
docker compose -f docker-compose.yaml -f docker-compose.creds.yaml up --abort-on-container-exit
```

## Viewing the results

After a successful run of the example, you can see the generated metrics in the GCP console via Metrics Explorer. The generated metrics would be present under the `Prometheus Target` resource.

Similarly, to view the generated traces in the GCP console, use the Trace Explorer.

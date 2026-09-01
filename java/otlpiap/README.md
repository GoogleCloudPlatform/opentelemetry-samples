# OTLP Export to Cloud Run Collector behind Identity-Aware Proxy (IAP)

This sample demonstrates how to run an OpenTelemetry Collector inside a Cloud Run service secured with **direct Identity-Aware Proxy (IAP)**, and how to programmatically send telemetry (traces and metrics) from a Java application to this secured Collector. The Collector is configured to export all received telemetry to Google's Native Telemetry API (`telemetry.googleapis.com`).
This sample relies on the [gcp-auth-extension](https://github.com/open-telemetry/opentelemetry-java-contrib/tree/main/gcp-auth-extension) to work.

## Architecture & How It Works

1. **Client Application (Java)**: Generates traces and metrics. It leverages the OpenTelemetry SDK autoconfiguration alongside the [gcp-auth-extension](https://github.com/open-telemetry/opentelemetry-java-contrib/tree/main/gcp-auth-extension) to automatically fetch, cache, and attach Google OIDC ID tokens targeting the IAP OAuth Client ID (`google.auth.token.type=id_token` and `google.auth.id.token.audience` / `GOOGLE_AUTH_ID_TOKEN_AUDIENCE`). The extension injects the token into the `Authorization: Bearer <ID_TOKEN>` header for all OTLP/HTTP requests sent to the Collector without requiring manual token management in application code.
2. **Identity-Aware Proxy (IAP)**: Secured directly on the Cloud Run service (`otel-collector-iap`). It intercepts incoming requests, validates the Google-signed ID token, and verifies that the calling service account has the **IAP-secured Web App User** (`roles/iap.httpsResourceAccessor`) role.
3. **Cloud Run Service (`otel-collector-iap`)**: Runs the OpenTelemetry Collector Contrib container. It is configured to receive OTLP/HTTP traffic on the container port allocated by Cloud Run.
4. **OpenTelemetry Collector**:
   - **Receiver**: `otlp` (HTTP protocol) listening on `${PORT}`.
   - **Exporter**: `otlphttp` pointing to `https://telemetry.googleapis.com` using the `googleclientauth` extension for automatic credential management.
   - **GCP Backends**: Google Cloud Trace (for traces) and Cloud Monitoring (for metrics).

---

## Prerequisites

Before running the sample, ensure you have:
1. Installed the [Google Cloud CLI](https://cloud.google.com/sdk/gcloud).
2. Authenticated with gcloud:
   ```shell
   gcloud auth login
   ```
3. A Google Cloud project set as your active project:
   ```shell
   gcloud config set project YOUR_PROJECT_ID
   ```
---

## Setup & Deployment

The sample includes a shell script `deploy-otel-collector-iap.sh` that automates the provisioning of the Cloud Run service, setting up the Collector configuration, creating necessary service accounts, and enabling direct IAP.

Run the deployment script from this directory:

```shell
./deploy-otel-collector-iap.sh
```

Upon successful completion, this script will:
- Enable all necessary GCP APIs.
- Build the custom Collector image via Cloud Build and push it to Artifact Registry.
- Create two service accounts:
  - `otel-collector-sa` (with trace/monitoring agent permissions).
  - `otel-client-sa` (with IAP web app user and settings viewer/admin permissions).
- Deploy the Collector to Cloud Run with direct IAP enabled.
- Grant `otel-client-sa` permissions to invoke the service (via `roles/iap.httpsResourceAccessor`) and view settings (via `roles/iap.settingsAdmin`).
- Grant the active user permissions to impersonate `otel-client-sa` (via `roles/iam.serviceAccountTokenCreator`).
- Output the URL of the deployed Cloud Run Collector.

### Post-Deployment: Grant Cloud Run Invoker permission to IAP service account

After the deployment script completes, you must manually grant the Cloud Run Invoker role to the IAP service account:

1. Find your **Project Number** on the Google Cloud Console Dashboard.
2. Go to the **Cloud Run** page in the Google Cloud Console.
3. Click on the name of your service (`otel-collector-iap`).
4. Click on the **Permissions** tab (if not visible, click **Show Info Panel** in the top right, then select **Permissions**).
5. Click **Add Principal**.
6. In the **New principals** field, enter:
   `service-<PROJECT_NUMBER>@gcp-sa-iap.iam.gserviceaccount.com`
   *(Replace `<PROJECT_NUMBER>` with your actual project number).*
7. In the **Select a role** dropdown, select **Cloud Run > Cloud Run Invoker**.
8. Click **Save**.

---

## Manual Setup Requirements (GCP Console)

Some parts of the Google Cloud configuration cannot be automated via CLI and must be configured manually in the Cloud Console:

### 1. OAuth Consent Screen Configuration
If this is the first time you are using OAuth or enabling IAP in your GCP project, you must configure the OAuth Consent Screen:
1. Navigate to the [OAuth consent screen](https://console.cloud.google.com/apis/credentials/consent) in the Google Cloud Console.
2. Select **Internal** (if your project is in a Google Workspace organization) or **External** as the user type.
3. Fill in the required fields (App name, User support email, Developer contact information) and click **Save and Continue**.
4. You do not need to add any scopes or test users for this example. Click **Save** to complete.

### 2. Configure Custom OAuth and Retrieve the Client ID
By default, the deployed IAP service on Cloud Run uses a Google-managed OAuth client. To enable programmatic access from your client application, this must be changed to use a custom OAuth 2.0 client.

To configure this and auto-generate the Client ID:
1. Navigate to the [Identity-Aware Proxy page](https://console.cloud.google.com/security/iap) in the Google Cloud Console.
2. Under the **Applications** tab, locate your Cloud Run service (e.g., `otel-collector-iap`).
3. Click the three dots (Actions menu) at the end of the row for your service and select **Edit OAuth client**.
4. In the side panel, change the setting to use a **Custom OAuth client**. If you do not have one, you can choose to auto-generate the OAuth 2.0 Client ID.
5. Once configured, navigate to the [Credentials page](https://console.cloud.google.com/apis/credentials) in the Google Cloud Console.
6. Under the **OAuth 2.0 Client IDs** section, locate the client ID associated with your IAP service (typically named after your Cloud Run service or IAP configuration).
7. Copy the **Client ID** string (e.g., `568958109999-xxxxxx.apps.googleusercontent.com`). This is the value you will set as the `GOOGLE_AUTH_ID_TOKEN_AUDIENCE` environment variable when running the Java app.

### 3. Configure OAuth Client for programmatic access
To allow programmatic access using the specific Client ID, you must add it to the allowlist for programmatic access on the IAP resource.

1. Create a file named `iap_settings.yaml` with the following content (replace `YOUR_CLIENT_ID` with the copied Client ID):
   ```yaml
   access_settings:
     oauth_settings:
       programmatic_clients:
       - "YOUR_CLIENT_ID"
   ```
2. Apply these settings to your Cloud Run service (replace `YOUR_PROJECT_ID` and `YOUR_REGION` with your project and region):
   ```shell
   gcloud iap settings set iap_settings.yaml \
       --project="YOUR_PROJECT_ID" \
       --resource-type=cloud-run \
       --region="YOUR_REGION" \
       --service="otel-collector-iap"
   ```
   *(For details, see [IAP Programmatic Access documentation](https://cloud.google.com/iap/docs/sharing-oauth-clients#programmatic_access).)*

---

## Running the Sample Application

This sample uses **Service Account Impersonation (Keyless)** to authenticate the client application with the IAP-secured Collector. This method uses your local `gcloud` login to automatically impersonate the client service account under the hood, without needing to download key files or add custom impersonation logic to your code.

1. Authenticate application default credentials locally using the `--impersonate-service-account` flag:
   ```shell
   gcloud auth application-default login --impersonate-service-account="otel-client-sa@YOUR_PROJECT_ID.iam.gserviceaccount.com"
   ```

2. Retrieve the required values and set the environment variables:
   ```shell
   export OTEL_EXPORTER_OTLP_ENDPOINT="<YOUR_COLLECTOR_URL>"
   export GOOGLE_AUTH_ID_TOKEN_AUDIENCE="<YOUR_IAP_CLIENT_ID>"
   ```
   * **OTEL_EXPORTER_OTLP_ENDPOINT**: Use the `OTel Collector URL` value output at the end of the `deploy-otel-collector-iap.sh` execution.
   * **GOOGLE_AUTH_ID_TOKEN_AUDIENCE**: You must configure custom OAuth 2.0 and retrieve the Client ID manually from the Google Cloud Console:

     > [!IMPORTANT]
     > By default, the deployed IAP service uses Google Managed OAuth. This must be changed to custom OAuth 2.0 to enable programmatic access from your client application.

     To configure this and retrieve the ID:
     1. Go to **Security > Identity-Aware Proxy** in the Cloud Console.
     2. Find your service, click the three dots, select **Edit OAuth client**, and change it to use a **Custom OAuth client** (this will auto-generate the Client ID).
     3. Navigate to **APIs & Services > Credentials** in your project.
     4. Under the **OAuth 2.0 Client IDs** section, locate and copy the client ID associated with your IAP service (typically named matching your Cloud Run service or IAP configuration).

3. Run the Java application using Gradle from the `opentelemetry-samples/java`:
   ```shell
   cd ..
   ./gradlew :otlpiap:run
   ```

4. Verification:
   - Go to the Google Cloud Console.
   - Navigate to **Trace > Trace list** to verify your trace span `iap-test-span` was exported.
   - Navigate to **Monitoring > Metrics Explorer** and look for `iap_test_counter` metric.

---

## Clean Up

To avoid ongoing charges, clean up the GCP resources created for this sample. Run the cleanup script from this directory:

```shell
./cleanup.sh
```

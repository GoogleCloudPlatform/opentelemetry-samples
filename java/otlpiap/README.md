# OTLP Export to Cloud Run Collector behind Identity-Aware Proxy (IAP)

This sample demonstrates how to run an OpenTelemetry Collector inside a Cloud Run service secured with **direct Identity-Aware Proxy (IAP)**, and how to programmatically send telemetry (traces and metrics) from a Java application to this secured Collector. The Collector is configured to export all received telemetry to Google's Native Telemetry API (`telemetry.googleapis.com`).

## Architecture & How It Works

1. **Client Application (Java)**: Generates traces and metrics. It uses the `google-auth-library-java` to obtain a Google OIDC ID token targeting the IAP OAuth Client ID. It passes this token dynamically in the `Authorization: Bearer <ID_TOKEN>` header for all OTLP/HTTP requests to the Collector.
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

## Manual Setup Requirements (GCP Console)

Some parts of the Google Cloud configuration cannot be automated via CLI and must be configured manually in the Cloud Console:

### 1. OAuth Consent Screen Configuration
If this is the first time you are using OAuth or enabling IAP in your GCP project, you must configure the OAuth Consent Screen:
1. Navigate to the [OAuth consent screen](https://console.cloud.google.com/apis/credentials/consent) in the Google Cloud Console.
2. Select **Internal** (if your project is in a Google Workspace organization) or **External** as the user type.
3. Fill in the required fields (App name, User support email, Developer contact information) and click **Save and Continue**.
4. You do not need to add any scopes or test users for this example. Click **Save** to complete.

### 2. Retrieve the Google-Managed OAuth Client ID
Since Direct IAP on Cloud Run uses a Google-managed OAuth client, its Client ID is managed automatically under the hood and cannot be retrieved programmatically by the deployment script. You must copy it manually:
1. Navigate to the [Credentials page](https://console.cloud.google.com/apis/credentials) in the Google Cloud Console.
2. Under the **OAuth 2.0 Client IDs** section, locate the Client ID associated with your IAP service (often named `IAP-App-Engine-app` or similar, or matching your Cloud Run service name).
3. Copy the **Client ID** string (e.g. `568958109999-xxxxxx.apps.googleusercontent.com`). This is the value you will set as the `IAP_CLIENT_ID` environment variable when running the Java app.

---

## Setup & Deployment

The sample includes a shell script `deploy-otel-collector-iap.sh` that automates the provisioning of the Cloud Run service, setting up the Collector configuration, creating necessary service accounts, enabling direct IAP, and downloading the client service account credentials.

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
- Generate and download `client-sa-key.json` to the current directory (*Optional, will require uncommenting relevant steps from `deploy-otel-collector-iap.sh` script*).
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


## Running the Sample Application

You can run the sample application using either **Service Account Impersonation (Keyless)** or using the **Service Account Key File**.

### Option A: Service Account Impersonation (Recommended, Keyless)

This method uses your local `gcloud` login to automatically impersonate the client service account under the hood, without needing to download any key files or add custom impersonation logic to your code.

1. Authenticate application default credentials locally using the `--impersonate-service-account` flag:
   ```shell
   gcloud auth application-default login --impersonate-service-account="otel-client-sa@YOUR_PROJECT_ID.iam.gserviceaccount.com"
   ```

2. Retrieve the required values and set the environment variables:
   ```shell
   export COLLECTOR_URL="<YOUR_COLLECTOR_URL>"
   export IAP_CLIENT_ID="<YOUR_IAP_CLIENT_ID>"
   ```
   * **COLLECTOR_URL**: Use the `OTel Collector URL` value output at the end of the `deploy-otel-collector-iap.sh` execution.
   * **IAP_CLIENT_ID**: Since Direct IAP on Cloud Run uses a Google-managed OAuth client, its Client ID cannot be retrieved programmatically. You must retrieve it manually from the Google Cloud Console:
     1. Navigate to **APIs & Services > Credentials** in your project.
     2. Under the **OAuth 2.0 Client IDs** section, locate and copy the client ID associated with your IAP service (typically named matching your Cloud Run service or IAP configuration).

3. Run the Java application using Gradle from the repository root:
   ```shell
   cd ../..
   ./gradlew :otlpiap:run
   ```

### Option B: Using the Service Account Key File

This method uses the downloaded JSON key file to authenticate directly as the client service account.
Note: You need to update the `deploy-otel-collector-iap.sh` script to uncomment the steps for creating the client service account key.

1. Set the environment variables pointing to your downloaded key and IAP configuration:
   ```shell
   export GOOGLE_APPLICATION_CREDENTIALS=client-sa-key.json
   export COLLECTOR_URL="<YOUR_COLLECTOR_URL>"
   export IAP_CLIENT_ID="<YOUR_IAP_CLIENT_ID>"
   ```
   *(See step 2 of Option A for instructions on how to find the `COLLECTOR_URL` and `IAP_CLIENT_ID` values)*


2. Run the Java application using Gradle from the repository root:
   ```shell
   cd ../..
   ./gradlew :otlpiap:run
   ```

3. Verification:
   - Go to the Google Cloud Console.
   - Navigate to **Trace > Trace list** to verify your trace span `iap-test-span` was exported.
   - Navigate to **Monitoring > Metrics Explorer** and look for `iap_test_counter` metric.

---

## Clean Up

To avoid ongoing charges, clean up the GCP resources created for this sample. Run the cleanup script from this directory:

```shell
./cleanup.sh
```

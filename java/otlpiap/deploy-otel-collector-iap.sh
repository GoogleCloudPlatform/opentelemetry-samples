#!/bin/bash
# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e

# Configuration
REGION=${REGION:-us-central1}
SERVICE_NAME="otel-collector-iap"

# Verify active project
PROJECT_ID=$(gcloud config get-value project 2>/dev/null)
if [ -z "$PROJECT_ID" ]; then
    echo "Error: No active Google Cloud project set. Run 'gcloud config set project <PROJECT_ID>' first." >&2
    exit 1
fi

PROJECT_NUMBER=$(gcloud projects describe "$PROJECT_ID" --format="value(projectNumber)")

echo "Deploying to Project: $PROJECT_ID (Number: $PROJECT_NUMBER), Region: $REGION"

# 1. Enable APIs
echo "Enabling required APIs..."
gcloud services enable \
    run.googleapis.com \
    iap.googleapis.com \
    artifactregistry.googleapis.com \
    cloudbuild.googleapis.com \
    cloudtrace.googleapis.com \
    monitoring.googleapis.com \
    iamcredentials.googleapis.com

# 2. Create Artifact Registry Repository if it doesn't exist
echo "Checking/creating Artifact Registry repository..."
if ! gcloud artifacts repositories describe otel-iap-repo --location="$REGION" &>/dev/null; then
    gcloud artifacts repositories create otel-iap-repo \
        --repository-format=docker \
        --location="$REGION" \
        --description="Repository for OTel Collector IAP sample"
fi

# 3. Create Service Accounts
echo "Checking/creating service accounts..."
COLLECTOR_SA="otel-collector-sa@$PROJECT_ID.iam.gserviceaccount.com"
CLIENT_SA="otel-client-sa@$PROJECT_ID.iam.gserviceaccount.com"

SA_CREATED=false
if ! gcloud iam service-accounts describe "$COLLECTOR_SA" &>/dev/null; then
    gcloud iam service-accounts create otel-collector-sa --display-name="OTel Collector IAP Service Account"
    SA_CREATED=true
fi

if ! gcloud iam service-accounts describe "$CLIENT_SA" &>/dev/null; then
    gcloud iam service-accounts create otel-client-sa --display-name="OTel Client IAP Service Account"
    SA_CREATED=true
fi

if [ "$SA_CREATED" = true ]; then
    echo "Waiting for service account propagation..."
    sleep 10
fi

# 4. Grant roles to Collector Service Account
echo "Granting roles to collector service account..."
gcloud projects add-iam-policy-binding "$PROJECT_ID" \
    --member="serviceAccount:$COLLECTOR_SA" \
    --role="roles/cloudtrace.agent" \
    --condition=None \
    --quiet
gcloud projects add-iam-policy-binding "$PROJECT_ID" \
    --member="serviceAccount:$COLLECTOR_SA" \
    --role="roles/monitoring.metricWriter" \
    --condition=None \
    --quiet

echo "Waiting for IAM role changes to propagate..."
sleep 10

# 5. Build and Push Collector Image
echo "Building and pushing collector image..."
gcloud builds submit --tag "$REGION-docker.pkg.dev/$PROJECT_ID/otel-iap-repo/otel-collector-iap:latest" .

# 6. Deploy to Cloud Run with IAP enabled
echo "Deploying OTel Collector to Cloud Run with IAP..."
gcloud beta run deploy "$SERVICE_NAME" \
    --image="$REGION-docker.pkg.dev/$PROJECT_ID/otel-iap-repo/otel-collector-iap:latest" \
    --service-account="$COLLECTOR_SA" \
    --iap \
    --no-allow-unauthenticated \
    --region="$REGION" \
    --port=4318 \
    --set-env-vars="GOOGLE_CLOUD_PROJECT=$PROJECT_ID" \
    --quiet

# 7. Grant access to client service account
echo "Granting invoker permissions to client service account on IAP..."
gcloud iap web add-iam-policy-binding \
    --resource-type=cloud-run \
    --service="$SERVICE_NAME" \
    --region="$REGION" \
    --member="serviceAccount:$CLIENT_SA" \
    --role="roles/iap.httpsResourceAccessor" \
    --quiet

# 8. Grant Token Creator role to active user for impersonation
GCLOUD_USER=$(gcloud config get-value account 2>/dev/null)
echo "Granting Token Creator role to $GCLOUD_USER on client service account..."
gcloud iam service-accounts add-iam-policy-binding "$CLIENT_SA" \
    --member="user:$GCLOUD_USER" \
    --role="roles/iam.serviceAccountTokenCreator" \
    --condition=None \
    --quiet

# Grant Settings Admin role to client service account to fetch settings
echo "Granting IAP Settings Admin role to client service account..."
gcloud projects add-iam-policy-binding "$PROJECT_ID" \
    --member="serviceAccount:$CLIENT_SA" \
    --role="roles/iap.settingsAdmin" \
    --condition=None \
    --quiet

# 9. Create Client SA Key (Optional, for key-based authentication)
# echo "Generating client service account key (optional)..."
# gcloud iam service-accounts keys create client-sa-key.json \
#     --iam-account="$CLIENT_SA"

# 10. Get Output Values
COLLECTOR_URL=$(gcloud run services describe "$SERVICE_NAME" --region="$REGION" --format="value(status.url)")

echo "=========================================================="
echo "Deployment completed successfully!"
echo "OTel Collector URL: $COLLECTOR_URL"
echo "=========================================================="

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

echo "Cleaning up resources in Project: $PROJECT_ID, Region: $REGION"

# 1. Delete Cloud Run service
echo "Deleting Cloud Run service..."
if gcloud run services describe "$SERVICE_NAME" --region="$REGION" &>/dev/null; then
    gcloud run services delete "$SERVICE_NAME" --region="$REGION" --quiet
else
    echo "Cloud Run service $SERVICE_NAME not found."
fi

# 2. Delete Artifact Registry Repository
echo "Deleting Artifact Registry repository..."
if gcloud artifacts repositories describe otel-iap-repo --location="$REGION" &>/dev/null; then
    gcloud artifacts repositories delete otel-iap-repo --location="$REGION" --quiet
else
    echo "Artifact Registry repository otel-iap-repo not found."
fi

# 3. Delete Service Accounts
COLLECTOR_SA="otel-collector-sa@$PROJECT_ID.iam.gserviceaccount.com"
CLIENT_SA="otel-client-sa@$PROJECT_ID.iam.gserviceaccount.com"

echo "Deleting service accounts..."
if gcloud iam service-accounts describe "$COLLECTOR_SA" &>/dev/null; then
    gcloud iam service-accounts delete "$COLLECTOR_SA" --quiet
fi

if gcloud iam service-accounts describe "$CLIENT_SA" &>/dev/null; then
    gcloud iam service-accounts delete "$CLIENT_SA" --quiet
fi

echo "Cleanup completed successfully!"

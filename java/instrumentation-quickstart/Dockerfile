# Copyright 2023 Google LLC
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
FROM eclipse-temurin:17-jdk-alpine
VOLUME /tmp
COPY build/libs/*-SNAPSHOT.jar app.jar
# [START opentelemetry_instrumentation_agent_dockerfile]
RUN wget -O /opentelemetry-javaagent.jar https://github.com/open-telemetry/opentelemetry-java-instrumentation/releases/download/v2.29.0/opentelemetry-javaagent.jar
CMD sh -c "java -javaagent:/opentelemetry-javaagent.jar -jar app.jar \
	2>&1 | tee /var/log/app.log"
# [END opentelemetry_instrumentation_agent_dockerfile]

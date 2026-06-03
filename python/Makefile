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

VENV = .venv
PIP = $(VENV)/bin/pip
RUFF = $(VENV)/bin/ruff

export PIP_EXTRA_INDEX_URL=https://pypi.org/simple

.PHONY: all lint clean setup format

all: lint

$(VENV)/bin/activate:
	python3 -m venv $(VENV)

setup: $(VENV)/bin/activate
	$(PIP) install -r otlpmetric/requirements.txt
	$(PIP) install -r otlptrace/requirements.txt
	$(PIP) install -r adk-sql-agent/requirements.txt
	$(PIP) install ruff

lint: setup
	$(RUFF) check otlpmetric otlptrace adk-sql-agent
	$(RUFF) format --check otlpmetric otlptrace adk-sql-agent

format: setup
	$(RUFF) format otlpmetric otlptrace adk-sql-agent

clean:
	rm -rf $(VENV)

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
FLAKE8 = $(VENV)/bin/flake8
AUTOPEP8 = $(VENV)/bin/autopep8

.PHONY: all lint clean setup format

all: lint

$(VENV)/bin/activate:
	python3 -m venv $(VENV)

setup: $(VENV)/bin/activate
	$(PIP) install -r otlpmetric/requirements.txt
	$(PIP) install -r otlptrace/requirements.txt
	$(PIP) install -r langgraph-sql-agent/requirements.txt
	$(PIP) install flake8 autopep8

lint: setup
	$(FLAKE8) --exclude=.venv otlpmetric otlptrace langgraph-sql-agent

format: setup
	$(AUTOPEP8) --in-place --recursive otlpmetric otlptrace langgraph-sql-agent

clean:
	rm -rf $(VENV)

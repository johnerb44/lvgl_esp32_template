#!/bin/bash
ollama run qwen-copilot:latest --keepalive 8h
export COPILOT_PROVIDER_BASE_URL="http://localhost:11434/v1"
export COPILOT_OFFLINE="true"
export COPILOT_MODEL="qwen-copilot"
export COPILOT_PROVIDER_TYPE="openai"
sleep 10
copilot

#!/usr/bin/env bash
set -u

LOG_DIR="${LOG_DIR:-./logs}"
mkdir -p "$LOG_DIR"

TS="$(date +%Y%m%d-%H%M%S)"
LOG_FILE="$LOG_DIR/idf-build-$TS.log"

echo "Logging build output to: $LOG_FILE"

idf.py build 2>&1 | tee "$LOG_FILE"
exit ${PIPESTATUS[0]}
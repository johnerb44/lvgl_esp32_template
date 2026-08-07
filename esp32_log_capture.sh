#!/bin/bash

# ESP32 Log Capture Script
# Captures UART/Terminal output from ESP32 and saves to timestamped log file

set -e  # Exit on any error

# Get current timestamp for unique filename
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="esp32_capture_${TIMESTAMP}.log"

echo "Starting ESP32 log capture..."
echo "Log file: $LOG_FILE"
echo "Press Ctrl+C to stop capture"

# Check if idf.py is available
if ! command -v idf.py &> /dev/null; then
    echo "Error: idf.py not found. Please ensure ESP-IDF is properly installed and configured."
    exit 1
fi

# Capture serial output using idf.py monitor
# This will continue until manually stopped with Ctrl+C
idf.py monitor > "$LOG_FILE" 2>&1

echo "Capture completed. Output saved to $LOG_FILE"
#!/bin/bash
# ESP-IDF Build Script for lvgl_esp32_template
# This script uses ESP-IDF v5.1.6

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "================================================"
echo "Secure Lockbox ESP32-S3 Build Script"
echo "================================================"
echo ""

# ESP-IDF location
export IDF_PATH="/home/erbj/esp/v5.1.6/esp-idf"

if [ ! -d "$IDF_PATH" ]; then
    echo "✗ ESP-IDF not found at: $IDF_PATH"
    exit 1
fi

echo "✓ Using ESP-IDF: $IDF_PATH"
echo ""

# Install ESP-IDF tools if not already installed
if [ ! -f "$IDF_PATH/tools/idf.py" ]; then
    echo "Installing ESP-IDF tools (this may take a few minutes)..."
    cd "$IDF_PATH"
    bash install.sh esp32s3
    cd "$SCRIPT_DIR"
fi

echo "Sourcing ESP-IDF environment..."
source "$IDF_PATH/export.sh"

echo ""
echo "Setting ESP32-S3 as build target..."
idf.py set-target esp32s3

echo ""
echo "Building project with idf.py..."
echo ""

# Run the build with any arguments passed to this script
idf.py "$@" build

echo ""
echo "✓ Build complete!"


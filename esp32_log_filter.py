#!/usr/bin/env python3
"""
ESP32 Log Filter Script
Filters ESP32 log output to extract errors, warnings, and other relevant information
in multiple formats (JSON, CSV, Plain text).
"""

import sys
import re
import json
import csv
from datetime import datetime

def parse_log_file(log_file_path):
    """Parse the log file and extract relevant entries"""
    entries = []

    try:
        with open(log_file_path, 'r') as f:
            lines = f.readlines()
    except FileNotFoundError:
        print(f"Error: Log file '{log_file_path}' not found.")
        sys.exit(1)
    except Exception as e:
        print(f"Error reading log file: {e}")
        sys.exit(1)

    # Regular expressions for matching different types of entries
    error_pattern = re.compile(r'(error|Error|ERROR):', re.IGNORECASE)
    warning_pattern = re.compile(r'(warning|Warning|WARNING):', re.IGNORECASE)
    panic_pattern = re.compile(r'(panic|PANIC)', re.IGNORECASE)
    guru_meditation_pattern = re.compile(r'(Guru Meditation)', re.IGNORECASE)
    backtrace_pattern = re.compile(r'(Backtrace:)', re.IGNORECASE)
    stack_trace_pattern = re.compile(r'#\d+\s+0x[0-9a-fA-F]+', re.IGNORECASE)

    current_entry = None
    first_error_found = False

    for i, line in enumerate(lines):
        # Skip empty lines
        if not line.strip():
            continue

        # Check for different types of entries
        is_error = error_pattern.search(line)
        is_warning = warning_pattern.search(line)
        is_panic = panic_pattern.search(line)
        is_guru_meditation = guru_meditation_pattern.search(line)
        is_backtrace = backtrace_pattern.search(line)
        is_stack_trace = stack_trace_pattern.search(line)

        # If we found a significant log entry
        if is_error or is_warning or is_panic or is_guru_meditation or is_backtrace or is_stack_trace:
            entry_type = "error" if is_error else \
                        "warning" if is_warning else \
                        "panic" if is_panic else \
                        "guru_meditation" if is_guru_meditation else \
                        "backtrace" if is_backtrace else \
                        "stack_trace"

            # For first error, save it as special case
            if not first_error_found and (is_error or is_panic or is_guru_meditation):
                first_error_found = True
                entry_type = "first_error"

            entry = {
                'line_number': i + 1,
                'type': entry_type,
                'content': line.strip(),
                'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            }

            entries.append(entry)

    return entries

def write_json_output(entries, output_file):
    """Write filtered entries to JSON file"""
    try:
        with open(output_file, 'w') as f:
            json.dump(entries, f, indent=2)
        print(f"JSON output written to {output_file}")
    except Exception as e:
        print(f"Error writing JSON file: {e}")

def write_csv_output(entries, output_file):
    """Write filtered entries to CSV file"""
    try:
        with open(output_file, 'w', newline='', encoding='utf-8') as f:
            if entries:
                writer = csv.DictWriter(f, fieldnames=['line_number', 'type', 'content'])
                writer.writeheader()
                for entry in entries:
                    # Clean content for CSV
                    clean_content = entry['content'].replace('\n', ' ').replace('\r', ' ')
                    writer.writerow({
                        'line_number': entry['line_number'],
                        'type': entry['type'],
                        'content': clean_content
                    })
        print(f"CSV output written to {output_file}")
    except Exception as e:
        print(f"Error writing CSV file: {e}")

def write_text_output(entries, output_file):
    """Write filtered entries to plain text file"""
    try:
        with open(output_file, 'w') as f:
            for entry in entries:
                f.write(f"[{entry['type']}] Line {entry['line_number']}: {entry['content']}\n")
        print(f"Text output written to {output_file}")
    except Exception as e:
        print(f"Error writing text file: {e}")

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 esp32_log_filter.py <log_file>")
        sys.exit(1)

    log_file = sys.argv[1]

    print(f"Filtering log file: {log_file}")

    # Parse the log file
    entries = parse_log_file(log_file)

    if not entries:
        print("No relevant entries found in the log file.")
        return

    print(f"Found {len(entries)} relevant entries")

    # Generate output filenames
    base_name = log_file.rsplit('.', 1)[0] if '.' in log_file else log_file
    json_output = f"{base_name}_filtered.json"
    csv_output = f"{base_name}_filtered.csv"
    text_output = f"{base_name}_filtered.txt"

    # Write outputs in different formats
    write_json_output(entries, json_output)
    write_csv_output(entries, csv_output)
    write_text_output(entries, text_output)

    print("\nFiltering completed successfully!")
    print("Output files created:")
    print(f"  - {json_output} (JSON)")
    print(f"  - {csv_output} (CSV)")
    print(f"  - {text_output} (Plain text)")

if __name__ == "__main__":
    main()
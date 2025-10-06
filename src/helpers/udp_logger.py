#!/usr/bin/env python3
"""
UDP Logger Receiver for Poseidon2

Listens for UDP broadcast messages on port 4444 from the ESP32 device.
Displays formatted JSON log messages with color-coded log levels.

Usage:
    python3 udp_logger.py [--port PORT] [--filter LEVEL]

Options:
    --port PORT       UDP port to listen on (default: 4444)
    --filter LEVEL    Only show logs at or above level: DEBUG, INFO, WARN, ERROR, FATAL
    --json            Output raw JSON (no formatting)
    --no-color        Disable colored output

Examples:
    python3 udp_logger.py                    # Listen on port 4444, show all logs
    python3 udp_logger.py --filter WARN      # Only show WARN, ERROR, FATAL
    python3 udp_logger.py --json             # Raw JSON output
"""

import socket
import json
import sys
import argparse

# ANSI color codes
class Colors:
    RESET = '\033[0m'
    BOLD = '\033[1m'

    # Log levels
    DEBUG = '\033[36m'      # Cyan
    INFO = '\033[32m'       # Green
    WARN = '\033[33m'       # Yellow
    ERROR = '\033[31m'      # Red
    FATAL = '\033[35m'      # Magenta

    # Components
    COMPONENT = '\033[94m'  # Blue
    EVENT = '\033[96m'      # Bright cyan
    TIMESTAMP = '\033[90m'  # Gray

def colorize(text, color, use_color=True):
    """Apply color to text if color is enabled"""
    if not use_color:
        return text
    return f"{color}{text}{Colors.RESET}"

def format_log_message(log_data, use_color=True):
    """Format a log message for display"""
    # Extract fields
    timestamp_ms = log_data.get('timestamp', 0)
    level = log_data.get('level', 'UNKNOWN')
    component = log_data.get('component', 'Unknown')
    event = log_data.get('event', 'Unknown')
    data = log_data.get('data', {})

    # Convert timestamp to readable format
    timestamp_sec = timestamp_ms / 1000.0
    time_str = f"{timestamp_sec:>10.3f}s"

    # Color-code log level
    level_colors = {
        'DEBUG': Colors.DEBUG,
        'INFO': Colors.INFO,
        'WARN': Colors.WARN,
        'ERROR': Colors.ERROR,
        'FATAL': Colors.FATAL
    }
    level_color = level_colors.get(level, Colors.RESET)

    # Format output
    formatted = (
        f"{colorize(time_str, Colors.TIMESTAMP, use_color)} "
        f"[{colorize(level, level_color, use_color)}] "
        f"{colorize(component, Colors.COMPONENT, use_color)}:"
        f"{colorize(event, Colors.EVENT, use_color)}"
    )

    # Add data if present
    if data:
        data_str = json.dumps(data, separators=(',', ':'))
        formatted += f" {data_str}"

    return formatted

def get_log_level_priority(level):
    """Get numeric priority for log level"""
    priorities = {
        'DEBUG': 0,
        'INFO': 1,
        'WARN': 2,
        'ERROR': 3,
        'FATAL': 4
    }
    return priorities.get(level, -1)

def main():
    parser = argparse.ArgumentParser(
        description='UDP Logger Receiver for Poseidon2',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument('--port', type=int, default=4444,
                        help='UDP port to listen on (default: 4444)')
    parser.add_argument('--filter', type=str, default='DEBUG',
                        choices=['DEBUG', 'INFO', 'WARN', 'ERROR', 'FATAL'],
                        help='Minimum log level to display')
    parser.add_argument('--json', action='store_true',
                        help='Output raw JSON (no formatting)')
    parser.add_argument('--no-color', action='store_true',
                        help='Disable colored output')

    args = parser.parse_args()

    use_color = not args.no_color and sys.stdout.isatty()
    min_priority = get_log_level_priority(args.filter)

    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    # Increase receive buffer size to prevent packet loss (default is often 64KB)
    # Set to 1MB to handle bursts of messages
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024 * 1024)

    try:
        sock.bind(('', args.port))  # Empty string binds to all interfaces
    except OSError as e:
        print(f"Error binding to port {args.port}: {e}", file=sys.stderr)
        print(f"Is another process using port {args.port}?", file=sys.stderr)
        return 1

    # Get actual buffer size (OS may limit it)
    actual_buffer = sock.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)

    print(f"{colorize('UDP Logger - Listening on port', Colors.BOLD, use_color)} {args.port}")
    print(f"{colorize('Filter level:', Colors.BOLD, use_color)} {args.filter}+")
    print(f"{colorize('RX Buffer size:', Colors.BOLD, use_color)} {actual_buffer:,} bytes")
    print(f"{colorize('Press Ctrl+C to exit', Colors.BOLD, use_color)}\n")

    last_timestamp = 0
    packets_received = 0
    packets_dropped = 0

    try:
        while True:
            # Receive UDP packet
            data, _ = sock.recvfrom(4096)  # Increased buffer per packet
            packets_received += 1

            try:
                # Decode and parse JSON
                message = data.decode('utf-8').strip()
                log_data = json.loads(message)

                # Detect packet loss by checking timestamp gaps
                timestamp = log_data.get('timestamp', 0)
                if last_timestamp > 0 and timestamp > last_timestamp + 10000:  # More than 10 seconds gap
                    gap_seconds = (timestamp - last_timestamp) / 1000.0
                    packets_dropped += 1
                    if not args.json:
                        print(f"{colorize('WARNING: Packet loss detected!', Colors.WARN, use_color)} "
                              f"Gap of {gap_seconds:.1f}s (last: {last_timestamp}ms, now: {timestamp}ms)")
                last_timestamp = timestamp

                # Filter by log level
                log_level = log_data.get('level', 'UNKNOWN')
                log_priority = get_log_level_priority(log_level)

                if log_priority < min_priority:
                    continue  # Skip logs below filter level

                # Output (buffer output to reduce blocking)
                if args.json:
                    print(message, flush=True)
                else:
                    formatted = format_log_message(log_data, use_color)
                    print(formatted, flush=True)

            except json.JSONDecodeError:
                # Not valid JSON, print raw
                print(f"{colorize('RAW:', Colors.ERROR, use_color)} {message}", flush=True)
            except UnicodeDecodeError:
                # Can't decode as UTF-8
                print(f"{colorize('BINARY:', Colors.ERROR, use_color)} {data.hex()}", flush=True)

    except KeyboardInterrupt:
        print(f"\n{colorize('Shutting down...', Colors.BOLD, use_color)}")
        print(f"{colorize('Packets received:', Colors.BOLD, use_color)} {packets_received}")
        if packets_dropped > 0:
            print(f"{colorize('Packet loss events:', Colors.WARN, use_color)} {packets_dropped}")
        return 0
    finally:
        sock.close()

if __name__ == '__main__':
    sys.exit(main())

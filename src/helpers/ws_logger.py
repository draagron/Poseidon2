#!/usr/bin/env python3
"""
WebSocket Logger Client for Poseidon2

Connects to ESP32 WebSocket endpoint and receives log messages reliably via TCP.
Uses WebSocket protocol for guaranteed delivery (no packet loss like UDP).

Usage:
    python3 ws_logger.py <ESP32_IP> [--port PORT] [--filter LEVEL]

Options:
    --port PORT       HTTP port (default: 80)
    --path PATH       WebSocket path (default: /logs)
    --filter LEVEL    Only show logs at or above level: DEBUG, INFO, WARN, ERROR, FATAL
    --json            Output raw JSON (no formatting)
    --no-color        Disable colored output
    --reconnect       Auto-reconnect on disconnect (default: exit on disconnect)

Examples:
    python3 ws_logger.py 192.168.0.94                  # Connect to ESP32
    python3 ws_logger.py 192.168.0.94 --filter WARN    # Only warnings/errors
    python3 ws_logger.py 192.168.0.94 --reconnect      # Keep reconnecting
    python3 ws_logger.py 192.168.0.94 --json           # Raw JSON output
"""

import asyncio
import websockets
import json
import sys
import argparse
import signal

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

async def receive_logs(uri, args, use_color, min_priority):
    """Connect to WebSocket and receive log messages"""
    packets_received = 0

    try:
        # Connect with increased timeouts and ping interval
        async with websockets.connect(
            uri,
            ping_interval=20,  # Send ping every 20 seconds
            ping_timeout=60,   # Wait 60 seconds for pong
            close_timeout=10   # Wait 10 seconds for close handshake
        ) as websocket:
            print(f"{colorize('Connected to', Colors.BOLD, use_color)} {uri}")
            print(f"{colorize('Filter level:', Colors.BOLD, use_color)} {args.filter}+")
            print(f"{colorize('Press Ctrl+C to exit', Colors.BOLD, use_color)}\n")

            async for message in websocket:
                packets_received += 1

                try:
                    # Parse JSON message
                    log_data = json.loads(message.strip())

                    # Handle connection status message
                    if 'status' in log_data and log_data['status'] == 'connected':
                        if not args.json:
                            print(f"{colorize('WebSocket handshake complete', Colors.BOLD, use_color)}\n")
                        continue

                    # Filter by log level
                    log_level = log_data.get('level', 'UNKNOWN')
                    log_priority = get_log_level_priority(log_level)

                    if log_priority < min_priority:
                        continue  # Skip logs below filter level

                    # Output
                    if args.json:
                        print(message.strip(), flush=True)
                    else:
                        formatted = format_log_message(log_data, use_color)
                        print(formatted, flush=True)

                except json.JSONDecodeError:
                    # Not valid JSON, print raw
                    print(f"{colorize('RAW:', Colors.ERROR, use_color)} {message}", flush=True)

    except websockets.exceptions.ConnectionClosed:
        print(f"\n{colorize('Connection closed by server', Colors.WARN, use_color)}")
        print(f"{colorize('Packets received:', Colors.BOLD, use_color)} {packets_received}")
        return packets_received

    except Exception as e:
        print(f"\n{colorize('Error:', Colors.ERROR, use_color)} {e}", file=sys.stderr)
        return packets_received

    return packets_received

async def main_async(args):
    """Main async function with reconnection logic"""
    use_color = not args.no_color and sys.stdout.isatty()
    min_priority = get_log_level_priority(args.filter)

    # Build WebSocket URI
    uri = f"ws://{args.host}:{args.port}{args.path}"

    total_packets = 0

    while True:
        try:
            packets = await receive_logs(uri, args, use_color, min_priority)
            total_packets += packets

            if not args.reconnect:
                break

            # Wait before reconnecting
            print(f"{colorize('Reconnecting in 5 seconds...', Colors.WARN, use_color)}")
            await asyncio.sleep(5)

        except KeyboardInterrupt:
            print(f"\n{colorize('Shutting down...', Colors.BOLD, use_color)}")
            print(f"{colorize('Total packets received:', Colors.BOLD, use_color)} {total_packets}")
            break

        except Exception as e:
            print(f"{colorize('Connection failed:', Colors.ERROR, use_color)} {e}", file=sys.stderr)

            if not args.reconnect:
                return 1

            print(f"{colorize('Retrying in 5 seconds...', Colors.WARN, use_color)}")
            await asyncio.sleep(5)

    return 0

def main():
    parser = argparse.ArgumentParser(
        description='WebSocket Logger Client for Poseidon2',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument('host', help='ESP32 IP address (e.g., 192.168.0.94)')
    parser.add_argument('--port', type=int, default=80,
                        help='HTTP port (default: 80)')
    parser.add_argument('--path', type=str, default='/logs',
                        help='WebSocket path (default: /logs)')
    parser.add_argument('--filter', type=str, default='DEBUG',
                        choices=['DEBUG', 'INFO', 'WARN', 'ERROR', 'FATAL'],
                        help='Minimum log level to display')
    parser.add_argument('--json', action='store_true',
                        help='Output raw JSON (no formatting)')
    parser.add_argument('--no-color', action='store_true',
                        help='Disable colored output')
    parser.add_argument('--reconnect', action='store_true',
                        help='Auto-reconnect on disconnect')

    args = parser.parse_args()

    # Run async main
    try:
        exit_code = asyncio.run(main_async(args))
        sys.exit(exit_code)
    except KeyboardInterrupt:
        print("\nInterrupted")
        sys.exit(0)

if __name__ == '__main__':
    main()

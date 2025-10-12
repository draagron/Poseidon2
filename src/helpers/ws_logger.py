#!/usr/bin/env python3
"""
WebSocket Logger Client for Poseidon2

Connects to ESP32 WebSocket endpoint and receives log messages reliably via TCP.
Uses WebSocket protocol for guaranteed delivery (no packet loss like UDP).

Usage:
    python3 ws_logger.py <ESP32_IP> [OPTIONS]

Options:
    --port PORT           HTTP port (default: 80)
    --path PATH           WebSocket path (default: /logs)
    --level LEVEL         Set server-side log level filter: DEBUG, INFO, WARN, ERROR, FATAL
    --components COMP     Set server-side component filter (comma-separated, e.g., "NMEA2000,GPS")
    --events EVENTS       Set server-side event prefix filter (comma-separated, e.g., "PGN130306_,ERROR")
    --filter LEVEL        Client-side log level filter (deprecated, use --level for server-side)
    --json                Output raw JSON (no formatting)
    --no-color            Disable colored output
    --reconnect           Auto-reconnect on disconnect (default: exit on disconnect)
    --no-server-filter    Skip setting server-side filter (use existing server filter)

Examples:
    # Connect and display current server filter (no changes)
    python3 ws_logger.py 192.168.0.94

    # Set server filter to DEBUG level for NMEA2000 component only
    python3 ws_logger.py 192.168.0.94 --level DEBUG --components NMEA2000

    # Filter specific PGN events
    python3 ws_logger.py 192.168.0.94 --level DEBUG --events PGN130306_

    # Combine filters (AND logic)
    python3 ws_logger.py 192.168.0.94 --level DEBUG --components NMEA2000 --events PGN

    # Use existing server filter (don't fetch or change it)
    python3 ws_logger.py 192.168.0.94 --no-server-filter

    # Reset to defaults (INFO level, all components/events)
    python3 ws_logger.py 192.168.0.94 --level INFO --components "" --events ""

Note:
    - If no filter args provided, fetches and displays current server filter
    - Server filter persists across ESP32 reboots (saved to /log-filter.json)
    - Client-side --filter provides additional filtering on top of server filter
"""

import asyncio
import websockets
import json
import sys
import argparse
import signal
import urllib.request
import urllib.parse
import urllib.error

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

def get_server_filter(host, port, use_color=True):
    """
    Get current server-side log filter via HTTP GET to /log-filter endpoint

    Args:
        host: ESP32 IP address
        port: HTTP port
        use_color: Whether to use colored output

    Returns:
        dict with filter config or None if failed
    """
    url = f"http://{host}:{port}/log-filter"

    try:
        req = urllib.request.Request(url, method='GET')
        with urllib.request.urlopen(req, timeout=5) as response:
            response_data = response.read().decode('utf-8')
            filter_config = json.loads(response_data)

            print(f"{colorize('Current server-side filter:', Colors.BOLD, use_color)}")
            print(f"  Level: {colorize(filter_config.get('level', 'INFO'), Colors.BOLD, use_color)}")

            components = filter_config.get('components', '')
            if components:
                print(f"  Components: {colorize(components, Colors.COMPONENT, use_color)}")
            else:
                print(f"  Components: {colorize('(all)', Colors.TIMESTAMP, use_color)}")

            events = filter_config.get('events', '')
            if events:
                print(f"  Events: {colorize(events, Colors.EVENT, use_color)}")
            else:
                print(f"  Events: {colorize('(all)', Colors.TIMESTAMP, use_color)}")

            print()  # Empty line
            return filter_config

    except urllib.error.HTTPError as e:
        print(f"{colorize('✗ HTTP Error:', Colors.ERROR, use_color)} {e.code} {e.reason}")
        return None
    except urllib.error.URLError as e:
        print(f"{colorize('✗ Connection Error:', Colors.ERROR, use_color)} {e.reason}")
        print(f"{colorize('  Make sure ESP32 is reachable and web server is running', Colors.WARN, use_color)}\n")
        return None
    except Exception as e:
        print(f"{colorize('✗ Error fetching filter:', Colors.ERROR, use_color)} {e}\n")
        return None

def set_server_filter(host, port, level=None, components=None, events=None, use_color=True):
    """
    Set server-side log filter via HTTP POST to /log-filter endpoint

    Args:
        host: ESP32 IP address
        port: HTTP port
        level: Log level (DEBUG, INFO, WARN, ERROR, FATAL) or None to skip
        components: Comma-separated component list or None to skip
        events: Comma-separated event prefix list or None to skip
        use_color: Whether to use colored output

    Returns:
        True if successful, False otherwise
    """
    # Build query parameters
    params = {}
    if level is not None:
        params['level'] = level
    if components is not None:
        params['components'] = components
    if events is not None:
        params['events'] = events

    # If no parameters, nothing to set
    if not params:
        return True

    # Build URL
    query_string = urllib.parse.urlencode(params)
    url = f"http://{host}:{port}/log-filter?{query_string}"

    try:
        print(f"{colorize('Setting server-side filter...', Colors.BOLD, use_color)}")
        if level:
            print(f"  Level: {colorize(level, Colors.BOLD, use_color)}")
        if components:
            print(f"  Components: {colorize(components, Colors.COMPONENT, use_color)}")
        if events:
            print(f"  Events: {colorize(events, Colors.EVENT, use_color)}")

        # Make HTTP POST request
        req = urllib.request.Request(url, method='POST')
        with urllib.request.urlopen(req, timeout=5) as response:
            response_data = response.read().decode('utf-8')
            result = json.loads(response_data)

            if result.get('status') == 'ok':
                print(f"{colorize('✓ Server filter updated successfully', Colors.INFO, use_color)}\n")
                return True
            else:
                print(f"{colorize('✗ Server returned unexpected response', Colors.WARN, use_color)}")
                print(f"  Response: {response_data}\n")
                return False

    except urllib.error.HTTPError as e:
        print(f"{colorize('✗ HTTP Error:', Colors.ERROR, use_color)} {e.code} {e.reason}")
        return False
    except urllib.error.URLError as e:
        print(f"{colorize('✗ Connection Error:', Colors.ERROR, use_color)} {e.reason}")
        print(f"{colorize('  Make sure ESP32 is reachable and web server is running', Colors.WARN, use_color)}\n")
        return False
    except Exception as e:
        print(f"{colorize('✗ Error setting filter:', Colors.ERROR, use_color)} {e}\n")
        return False

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
            if args.level or args.components or args.events:
                print(f"{colorize('Server filter active', Colors.BOLD, use_color)} (see configuration above)")
            print(f"{colorize('Client-side filter:', Colors.BOLD, use_color)} {args.filter}+ (additional filtering)")
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

    # Handle server-side filter (unless --no-server-filter specified)
    if not args.no_server_filter:
        # If --level, --components, or --events specified, set server filter
        if args.level is not None or args.components is not None or args.events is not None:
            success = set_server_filter(
                args.host,
                args.port,
                level=args.level,
                components=args.components,
                events=args.events,
                use_color=use_color
            )
            if not success:
                print(f"{colorize('Warning: Failed to set server filter, continuing anyway...', Colors.WARN, use_color)}\n")
        else:
            # No filter args provided - fetch and display current server filter
            get_server_filter(args.host, args.port, use_color=use_color)

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

    # Server-side filter options (new)
    parser.add_argument('--level', type=str, default=None,
                        choices=['DEBUG', 'INFO', 'WARN', 'ERROR', 'FATAL'],
                        help='Set server-side log level filter')
    parser.add_argument('--components', type=str, default=None,
                        help='Set server-side component filter (comma-separated)')
    parser.add_argument('--events', type=str, default=None,
                        help='Set server-side event prefix filter (comma-separated)')
    parser.add_argument('--no-server-filter', action='store_true',
                        help='Skip setting server-side filter (use existing server filter)')

    # Client-side filter (deprecated, for backward compatibility)
    parser.add_argument('--filter', type=str, default='DEBUG',
                        choices=['DEBUG', 'INFO', 'WARN', 'ERROR', 'FATAL'],
                        help='Client-side log level filter (deprecated, use --level)')

    # Output options
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

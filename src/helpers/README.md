# Poseidon2 Helper Scripts

Network-based logging tools for ESP32 debugging.

## WebSocket Logger (`ws_logger.py`) - **RECOMMENDED**

**Reliable TCP-based logging with guaranteed delivery** - no packet loss like UDP.

### Quick Start

```bash
# Replace with your ESP32's IP address
python3 src/helpers/ws_logger.py 192.168.0.94
```

### Features

- **Zero packet loss**: TCP-based WebSocket protocol ensures reliable delivery
- **Real-time streaming**: Same speed as UDP but guaranteed delivery
- **Color-coded log levels**: DEBUG (cyan), INFO (green), WARN (yellow), ERROR (red), FATAL (magenta)
- **Auto-reconnect option**: Keeps trying if connection drops
- **Log filtering**: Show only logs at or above a certain level

### Usage Examples

```bash
# Connect to ESP32 (replace IP with yours)
python3 src/helpers/ws_logger.py 192.168.0.94

# Only show warnings and errors
python3 src/helpers/ws_logger.py 192.168.0.94 --filter WARN

# Auto-reconnect on disconnect
python3 src/helpers/ws_logger.py 192.168.0.94 --reconnect

# Raw JSON output (for piping to jq or logging)
python3 src/helpers/ws_logger.py 192.168.0.94 --json

# Save to file
python3 src/helpers/ws_logger.py 192.168.0.94 --json > logs.jsonl

# Disable colors (for redirecting to file)
python3 src/helpers/ws_logger.py 192.168.0.94 --no-color
```

### Sample Output

```
Connected to ws://192.168.0.94:80/logs
Filter level: DEBUG+
Press Ctrl+C to exit

WebSocket handshake complete

    30.404s [INFO] WiFiManager:CONNECTION_SUCCESS {"ssid":"5cwifi"}
    30.409s [INFO] WebServer:STARTED {"ip":"192.168.0.94","port":80}
    35.300s [INFO] KeepAlive:HEARTBEAT {"uptime":35,"ssid":"5cwifi"}
    40.304s [INFO] KeepAlive:HEARTBEAT {"uptime":40,"ssid":"5cwifi"}
```

### Requirements

```bash
# Install websockets library (required)
pip3 install websockets
```

---

## UDP Logger (`udp_logger.py`) - LEGACY

**Unreliable UDP broadcast logging** - may experience packet loss on busy networks.

> ⚠️ **Deprecated**: Use WebSocket logger (`ws_logger.py`) instead for reliable logging.

Listens for UDP broadcast debug messages from the ESP32 device on port 4444.

### Quick Start

```bash
# From project root
python3 src/helpers/udp_logger.py
```

### Features

- **Color-coded log levels**: DEBUG (cyan), INFO (green), WARN (yellow), ERROR (red), FATAL (magenta)
- **Formatted output**: Timestamp, level, component, event, and JSON data
- **Log filtering**: Show only logs at or above a certain level
- **Raw JSON mode**: For parsing or logging to file

### Usage Examples

```bash
# Show all logs (default)
python3 src/helpers/udp_logger.py

# Only show warnings and errors
python3 src/helpers/udp_logger.py --filter WARN

# Raw JSON output (for piping to jq or logging)
python3 src/helpers/udp_logger.py --json

# Save to file
python3 src/helpers/udp_logger.py --json > logs.jsonl

# Use different port
python3 src/helpers/udp_logger.py --port 5555

# Disable colors (for redirecting to file)
python3 src/helpers/udp_logger.py --no-color
```

### Sample Output

```
UDP Logger - Listening on port 4444
Filter level: DEBUG+
Press Ctrl+C to exit

    30.404s [INFO] WiFiManager:CONNECTION_SUCCESS {"ssid":"5cwifi"}
    30.409s [INFO] WebServer:STARTED {"ip":"192.168.0.94","port":80}
    35.300s [INFO] KeepAlive:HEARTBEAT {"uptime":35,"ssid":"5cwifi"}
    40.304s [INFO] KeepAlive:HEARTBEAT {"uptime":40,"ssid":"5cwifi"}
```

### Requirements

- Python 3.6 or higher (included in macOS)
- No external dependencies required

### Troubleshooting

**"Address already in use" error:**
- Another process is using port 4444
- Check with: `lsof -i :4444`
- Kill process or use `--port` to specify different port

**No messages received:**
- Verify ESP32 is connected to WiFi (check serial output)
- Ensure ESP32 and computer are on same network
- Check firewall isn't blocking UDP port 4444
- Verify ESP32 IP address with: `arp -a | grep 192.168`

**Messages stop after first packet (using `nc`):**
- Use this Python script instead - macOS `nc` has issues with continuous UDP

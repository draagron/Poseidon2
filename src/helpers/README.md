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


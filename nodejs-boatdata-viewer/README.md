# BoatData WebSocket Proxy Server

Node.js WebSocket proxy server for ESP32 Poseidon2 BoatData streaming. Connects to ESP32 WebSocket endpoint and relays real-time sensor data to multiple browser clients.

## Features

- ✅ **WebSocket Proxy**: Relays data from ESP32 to multiple browsers
- ✅ **Auto-Reconnect**: Automatically reconnects to ESP32 on disconnect (5s delay)
- ✅ **Multi-Client**: Handles multiple browser connections simultaneously
- ✅ **Connection Status**: Shows both proxy and ESP32 connection states
- ✅ **Responsive Dashboard**: Mobile-friendly marine-themed UI
- ✅ **Zero ESP32 Load**: Only one connection to ESP32 regardless of browser count

## Architecture

```
┌─────────────────┐         ┌──────────────────┐         ┌─────────────────┐
│   ESP32 Device  │ WS      │   Node.js Server │  WS     │  Browser Client │
│  /boatdata      │◄────────│   (Proxy/Relay)  │────────►│   /stream       │
│  ws://IP:80     │         │   http://IP:3000 │         │                 │
└─────────────────┘         └──────────────────┘         └─────────────────┘
```

## Quick Start

### 1. Install Dependencies

```bash
cd nodejs-boatdata-viewer
npm install
```

### 2. Configure ESP32 IP Address

Edit `config.json`:

```json
{
  "esp32": {
    "ip": "192.168.1.100",    ← Change to your ESP32's IP address
    "port": 80,
    "wsPath": "/boatdata"
  },
  "server": {
    "port": 3000,
    "reconnectInterval": 5000
  }
}
```

**Tip**: Create `config.local.json` to override settings without modifying `config.json` (useful for version control).

### 3. Start Server

```bash
npm start
# Or: node server.js
```

### 4. Open Dashboard

Open your browser to:
```
http://localhost:3000/stream.html
```

## Configuration Options

### Configuration File

**Primary config**: `config.json`
**Local override**: `config.local.json` (optional, takes precedence)

```json
{
  "esp32": {
    "ip": "192.168.1.100",       // ESP32 IP address
    "port": 80,                   // ESP32 HTTP port
    "wsPath": "/boatdata"         // WebSocket endpoint path
  },
  "server": {
    "port": 3000,                 // Node.js server port
    "reconnectInterval": 5000     // Auto-reconnect delay (ms)
  }
}
```

### Environment Variables

Override configuration via environment variables:

```bash
# Change ESP32 IP
ESP32_IP=192.168.1.100 node server.js

# Change server port
PORT=8080 node server.js

# Combine multiple variables
ESP32_IP=192.168.1.100 PORT=8080 node server.js
```

## API Endpoints

### GET /stream.html
Serves the BoatData dashboard HTML page.

**Example:**
```
http://localhost:3000/stream.html
```

### GET /api/config
Returns current configuration and connection status (JSON).

**Example:**
```bash
curl http://localhost:3000/api/config
```

**Response:**
```json
{
  "esp32": {
    "ip": "192.168.1.100",
    "port": 80,
    "connected": true,
    "lastMessageTime": "2025-10-13T12:34:56.789Z"
  },
  "server": {
    "port": 3000,
    "connectedClients": 2,
    "uptime": 3600
  }
}
```

### WebSocket /boatdata
WebSocket endpoint for real-time data streaming.

**Browser connection:**
```javascript
const ws = new WebSocket('ws://localhost:3000/boatdata');
```

## Dashboard Features

### Connection Status Indicators

- **Proxy Status**: Connection between browser and Node.js server
  - 🟢 Green = Connected
  - 🟠 Orange = Connecting
  - 🔴 Red = Disconnected

- **ESP32 Status**: Connection between Node.js server and ESP32
  - 🟢 Green = Connected (data flowing)
  - 🔴 Red = Disconnected (no data)

### Sensor Data Cards

9 sensor groups displayed:
1. **GPS Navigation** - Lat/Lon, COG, SOG, Variation
2. **Compass & Attitude** - Heading, Rate of Turn, Heel, Pitch, Heave
3. **Wind Data** - Apparent wind angle and speed
4. **DST** - Depth, Speed (water), Temperature
5. **Rudder Position** - Steering angle
6. **Engine Monitoring** - RPM, Oil temp, Alternator voltage
7. **Saildrive Status** - Engaged/Retracted
8. **Battery Banks** - Voltage, Current, SOC (A & B)
9. **Shore Power** - Connection status, Power draw

### Auto-Reconnect

Both connections have automatic reconnection:
- **Proxy ↔ Browser**: 5 seconds (handled by browser)
- **Node.js ↔ ESP32**: 5 seconds (handled by server)

## Usage Examples

### Local Development

```bash
# Start server with default config
npm start

# Access dashboard
open http://localhost:3000/stream.html
```

### Change Port

```bash
# Run on port 8080 instead of 3000
PORT=8080 npm start

# Access dashboard
open http://localhost:8080/stream.html
```

### Multiple ESP32 Devices

Run multiple server instances on different ports:

```bash
# Terminal 1 - Poseidon2 Device 1
ESP32_IP=192.168.1.100 PORT=3001 node server.js

# Terminal 2 - Poseidon2 Device 2
ESP32_IP=192.168.1.101 PORT=3002 node server.js
```

Access dashboards:
- Device 1: http://localhost:3001/stream.html
- Device 2: http://localhost:3002/stream.html

### Raspberry Pi Deployment

```bash
# Install Node.js on RPi
sudo apt update
sudo apt install nodejs npm

# Copy project to RPi
scp -r nodejs-boatdata-viewer pi@raspberrypi.local:~/

# SSH to RPi and start server
ssh pi@raspberrypi.local
cd nodejs-boatdata-viewer
npm install
npm start
```

Access from any device on boat network:
```
http://raspberrypi.local:3000/stream.html
```

### Run as Background Service (systemd)

Create `/etc/systemd/system/boatdata-proxy.service`:

```ini
[Unit]
Description=BoatData WebSocket Proxy
After=network.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/nodejs-boatdata-viewer
Environment="ESP32_IP=192.168.1.100"
ExecStart=/usr/bin/node server.js
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable and start service:
```bash
sudo systemctl enable boatdata-proxy
sudo systemctl start boatdata-proxy
sudo systemctl status boatdata-proxy
```

## Troubleshooting

### Port Already in Use

**Error:**
```
✗ Port 3000 is already in use. Try a different port:
```

**Solution:**
```bash
# Try a different port
PORT=3001 node server.js
```

### Cannot Connect to ESP32

**Symptoms:**
- "ESP32: Disconnected" status
- Console shows connection errors

**Solutions:**

1. **Check ESP32 IP address:**
   ```bash
   ping 192.168.1.100
   ```

2. **Verify ESP32 is on network:**
   - Check WiFi connection on ESP32
   - Look at ESP32 serial output for IP address

3. **Test WebSocket endpoint directly:**
   ```bash
   curl http://192.168.1.100/stream
   ```

4. **Check firewall:**
   - Ensure port 80 is not blocked
   - Disable firewall temporarily for testing

5. **Update config.json:**
   - Verify IP address is correct
   - Check port number (default: 80)

### Dashboard Not Loading

**Symptoms:**
- Browser shows "Cannot GET /stream.html"

**Solutions:**

1. **Check file exists:**
   ```bash
   ls -la public/stream.html
   ```

2. **Verify server is running:**
   ```bash
   curl http://localhost:3000/api/config
   ```

3. **Check browser console (F12):**
   - Look for JavaScript errors
   - Check Network tab for failed requests

### No Data on Dashboard

**Symptoms:**
- Dashboard loads but shows all "--" values
- "ESP32: Disconnected" indicator

**Solutions:**

1. **Check ESP32 connection:**
   - Server must connect to ESP32 first
   - Wait for "ESP32: Connected" indicator

2. **Verify ESP32 is broadcasting:**
   - Check ESP32 logs via `ws_logger.py`
   - Ensure BoatData broadcast timer is running

3. **Check server logs:**
   - Look for "[ESP32] Connected" message
   - Check for message relay logs

## Development

### Project Structure

```
nodejs-boatdata-viewer/
├── package.json          # Dependencies and scripts
├── server.js             # Main server (WebSocket proxy)
├── config.json           # Configuration
├── config.local.json     # Local overrides (optional)
├── public/
│   └── stream.html       # Dashboard HTML
└── README.md             # This file
```

### Adding Features

#### Data Logging

Add to `server.js` after line where ESP32 client receives messages:

```javascript
const fs = require('fs');
const logStream = fs.createWriteStream('boatdata.log', { flags: 'a' });

esp32Client.on('message', (data) => {
    // Log to file
    logStream.write(JSON.stringify({
        timestamp: Date.now(),
        data: JSON.parse(data)
    }) + '\n');

    // Relay to browsers
    broadcastToBrowsers(data.toString());
});
```

#### Authentication

Add basic authentication:

```javascript
const basicAuth = require('express-basic-auth');

app.use(basicAuth({
    users: { 'admin': 'password123' },
    challenge: true
}));
```

## Benefits

### vs. Direct ESP32 Connection

| Feature | Node.js Proxy | Direct ESP32 |
|---------|--------------|--------------|
| ESP32 WebSocket connections | 1 | N (one per browser) |
| Multi-client performance | Excellent | Limited |
| Data logging | Easy | Not available |
| Network flexibility | Any network | Same WiFi only |
| Development/testing | No ESP32 needed | Requires hardware |

### Use Cases

1. **Development**: Test dashboard without ESP32 hardware
2. **Multiple Displays**: Show dashboard on multiple screens/devices
3. **Remote Monitoring**: Access data from different network (with routing)
4. **Data Logging**: Record all sensor data for later analysis
5. **Load Reduction**: Reduce WebSocket load on ESP32

## License

MIT

## Support

For issues or questions, check:
- ESP32 firmware: `/home/niels/Dev/Poseidon2/`
- WebSocket endpoint: `http://<ESP32_IP>/stream`
- WebSocket logs: `python3 src/helpers/ws_logger.py <ESP32_IP>`

---

**Version**: 1.0.0
**Last Updated**: 2025-10-13

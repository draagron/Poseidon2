# Contract: HTML Dashboard (Client-Side)

**Component**: HTML Dashboard
**Type**: Single-page web application
**Purpose**: Display real-time BoatData in browser via WebSocket connection
**Location**: `data/stream.html` (uploaded to LittleFS via `pio run --target uploadfs`)

## Architecture Overview

**Technology Stack**:
- HTML5 (semantic markup)
- CSS3 (inline styles, no external stylesheets)
- Vanilla JavaScript ES6 (no frameworks)
- WebSocket API (native browser support)

**Design Pattern**: Single-file application with inline CSS and JavaScript

**Why Single File**:
- Minimizes HTTP requests (1 request instead of 3)
- Simplifies LittleFS upload process
- Reduces total file size (no HTTP overhead for separate files)
- Easier cache management

## HTML Structure Requirements

### Document Structure

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Poseidon2 BoatData Dashboard</title>
    <style>
        /* Inline CSS (see CSS Requirements section) */
    </style>
</head>
<body>
    <header>
        <!-- Connection status indicator -->
    </header>
    <main>
        <!-- 9 sensor group cards -->
    </main>
    <footer>
        <!-- Timestamp, version info -->
    </footer>
    <script>
        /* Inline JavaScript (see JavaScript Requirements section) */
    </script>
</body>
</html>
```

### Required HTML Elements

#### Connection Status Indicator

```html
<header>
    <div id="connection-status" class="status-disconnected">
        <span id="status-icon">●</span>
        <span id="status-text">Disconnected</span>
    </div>
</header>
```

**States**:
- **Disconnected**: Red indicator, "Disconnected" text
- **Connecting**: Yellow indicator, "Connecting..." text
- **Connected**: Green indicator, "Connected" text

#### Sensor Group Cards (9 Required)

Each sensor group displayed as a card with:
- **Title**: Sensor group name (e.g., "GPS", "Compass", "Wind")
- **Data rows**: Field name + value + unit
- **Availability indicator**: Visual indicator if data unavailable
- **Last update**: Timestamp of last data update

**Example Card Structure**:
```html
<div class="sensor-card" id="gps-card">
    <h2>GPS</h2>
    <div class="availability-indicator" id="gps-availability">
        <span class="indicator-dot"></span>
        <span class="indicator-text">Available</span>
    </div>
    <div class="data-row">
        <span class="data-label">Latitude:</span>
        <span class="data-value" id="gps-latitude">--</span>
        <span class="data-unit">°</span>
    </div>
    <div class="data-row">
        <span class="data-label">Longitude:</span>
        <span class="data-value" id="gps-longitude">--</span>
        <span class="data-unit">°</span>
    </div>
    <!-- More data rows... -->
    <div class="last-update">
        Last update: <span id="gps-lastUpdate">Never</span>
    </div>
</div>
```

**All 9 Required Cards**:
1. GPS (7 fields: latitude, longitude, COG, SOG, variation, available, lastUpdate)
2. Compass (8 fields: true heading, magnetic heading, rate of turn, heel, pitch, heave, available, lastUpdate)
3. Wind (4 fields: apparent angle, apparent speed, available, lastUpdate)
4. DST (5 fields: depth, boat speed, sea temp, available, lastUpdate)
5. Rudder (3 fields: steering angle, available, lastUpdate)
6. Engine (5 fields: RPM, oil temp, alternator voltage, available, lastUpdate)
7. Saildrive (3 fields: engaged status, available, lastUpdate)
8. Battery (12 fields: voltage A/B, amperage A/B, SoC A/B, shore charger A/B, engine charger A/B, available, lastUpdate)
9. Shore Power (4 fields: power on, power draw, available, lastUpdate)

## CSS Requirements

### Layout

**Responsive Grid**:
- Desktop (≥1024px): 3 columns of sensor cards
- Tablet (768-1023px): 2 columns
- Mobile (<768px): 1 column, stacked

**CSS Grid Pattern**:
```css
main {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    gap: 20px;
    padding: 20px;
}
```

### Visual Design

**Color Scheme** (Marine Theme):
- Background: Dark blue (#0a1929)
- Card background: Navy (#132f4c)
- Text: Light gray (#e3f2fd)
- Accent: Cyan (#00bcd4)
- Status indicators: Green (#4caf50) / Yellow (#ffeb3b) / Red (#f44336)

**Typography**:
- Font family: system fonts (Arial, Helvetica, sans-serif)
- Header: 24px bold
- Card title: 18px bold
- Data labels: 14px regular
- Data values: 16px monospace (for number alignment)

**Card Design**:
- Border radius: 8px
- Box shadow: 0 2px 8px rgba(0,0,0,0.3)
- Padding: 16px
- Margin: 10px

### Status Indicators

**Connection Status** (in header):
```css
.status-connected { color: #4caf50; }    /* Green */
.status-connecting { color: #ffeb3b; }   /* Yellow */
.status-disconnected { color: #f44336; } /* Red */
```

**Availability Indicator** (per sensor card):
```css
.indicator-dot {
    display: inline-block;
    width: 10px;
    height: 10px;
    border-radius: 50%;
    margin-right: 5px;
}

.indicator-available { background-color: #4caf50; }   /* Green */
.indicator-unavailable { background-color: #f44336; } /* Red */
```

### Data Value States

**Unavailable Data**:
- Display: "--" (double dash)
- Color: Dim gray (#757575)
- Font style: italic

**Available Data**:
- Display: Actual value
- Color: Light gray (#e3f2fd)
- Font style: normal

## JavaScript Requirements

### WebSocket Connection Management

#### Connection Establishment

**On Page Load**:
```javascript
window.addEventListener('DOMContentLoaded', () => {
    connectWebSocket();
});

function connectWebSocket() {
    const wsUrl = 'ws://' + location.host + '/boatdata';
    ws = new WebSocket(wsUrl);

    updateConnectionStatus('connecting');

    ws.onopen = handleConnect;
    ws.onmessage = handleMessage;
    ws.onerror = handleError;
    ws.onclose = handleClose;
}
```

#### Connection Event Handlers

**onopen (connection established)**:
```javascript
function handleConnect(event) {
    console.log('WebSocket connected');
    updateConnectionStatus('connected');
}
```

**onmessage (data received)**:
```javascript
function handleMessage(event) {
    try {
        const data = JSON.parse(event.data);
        updateDashboard(data);
    } catch (error) {
        console.error('JSON parse error:', error);
    }
}
```

**onerror (connection error)**:
```javascript
function handleError(error) {
    console.error('WebSocket error:', error);
    updateConnectionStatus('disconnected');
}
```

**onclose (connection closed)**:
```javascript
function handleClose(event) {
    console.log('WebSocket disconnected');
    updateConnectionStatus('disconnected');

    // Auto-reconnect after 5 seconds
    setTimeout(() => {
        console.log('Reconnecting...');
        connectWebSocket();
    }, 5000);
}
```

### Data Processing

#### Update Dashboard Function

**Main Update Function**:
```javascript
function updateDashboard(data) {
    // Update timestamp
    document.getElementById('dashboard-timestamp').textContent =
        new Date(data.timestamp).toLocaleTimeString();

    // Update each sensor group
    updateGPS(data.gps);
    updateCompass(data.compass);
    updateWind(data.wind);
    updateDST(data.dst);
    updateRudder(data.rudder);
    updateEngine(data.engine);
    updateSaildrive(data.saildrive);
    updateBattery(data.battery);
    updateShorePower(data.shorePower);
}
```

#### Unit Conversion Functions

**Required Conversions** (server sends raw values, client converts for display):

```javascript
// Radians to degrees (0-360)
function radToDeg(rad) {
    if (rad === null || rad === undefined) return null;
    let deg = rad * 180.0 / Math.PI;
    if (deg < 0) deg += 360;
    return deg.toFixed(1);
}

// Radians to signed degrees (-180 to 180)
function radToSignedDeg(rad) {
    if (rad === null || rad === undefined) return null;
    let deg = rad * 180.0 / Math.PI;
    if (deg > 180) deg -= 360;
    if (deg < -180) deg += 360;
    return deg.toFixed(1);
}

// Meters per second to knots
function msToKnots(ms) {
    if (ms === null || ms === undefined) return null;
    return (ms * 1.94384).toFixed(1);
}

// Kelvin to Celsius (should not be needed, server converts)
function kelvinToCelsius(k) {
    if (k === null || k === undefined) return null;
    return (k - 273.15).toFixed(1);
}

// Timestamp to relative time ("5s ago", "2m ago")
function formatLastUpdate(timestamp) {
    if (!timestamp) return 'Never';
    const now = Date.now();
    const delta = Math.floor((now - timestamp) / 1000); // seconds

    if (delta < 5) return 'Just now';
    if (delta < 60) return delta + 's ago';
    if (delta < 3600) return Math.floor(delta / 60) + 'm ago';
    return Math.floor(delta / 3600) + 'h ago';
}
```

#### Sensor Update Functions

**Example: GPS Update Function**:
```javascript
function updateGPS(gps) {
    // Update availability indicator
    const availabilityElem = document.getElementById('gps-availability');
    if (gps.available) {
        availabilityElem.querySelector('.indicator-dot').className = 'indicator-dot indicator-available';
        availabilityElem.querySelector('.indicator-text').textContent = 'Available';
    } else {
        availabilityElem.querySelector('.indicator-dot').className = 'indicator-dot indicator-unavailable';
        availabilityElem.querySelector('.indicator-text').textContent = 'Unavailable';
    }

    // Update data values
    updateValue('gps-latitude', gps.latitude, val => val.toFixed(6), '°');
    updateValue('gps-longitude', gps.longitude, val => val.toFixed(6), '°');
    updateValue('gps-cog', gps.cog, radToDeg, '°');
    updateValue('gps-sog', gps.sog, val => val.toFixed(1), 'kts');
    updateValue('gps-variation', gps.variation, radToSignedDeg, '°');

    // Update last update timestamp
    document.getElementById('gps-lastUpdate').textContent = formatLastUpdate(gps.lastUpdate);
}

// Reusable value update helper
function updateValue(elementId, value, formatter, unit) {
    const elem = document.getElementById(elementId);
    if (value === null || value === undefined || !isFinite(value)) {
        elem.textContent = '--';
        elem.classList.add('unavailable');
    } else {
        elem.textContent = formatter ? formatter(value) : value;
        elem.classList.remove('unavailable');
    }
}
```

**All 9 Sensor Update Functions Required**:
1. `updateGPS(gps)`
2. `updateCompass(compass)`
3. `updateWind(wind)`
4. `updateDST(dst)`
5. `updateRudder(rudder)`
6. `updateEngine(engine)`
7. `updateSaildrive(saildrive)`
8. `updateBattery(battery)`
9. `updateShorePower(shorePower)`

### Connection Status Management

```javascript
function updateConnectionStatus(status) {
    const statusElem = document.getElementById('connection-status');
    const iconElem = document.getElementById('status-icon');
    const textElem = document.getElementById('status-text');

    // Remove all status classes
    statusElem.classList.remove('status-connected', 'status-connecting', 'status-disconnected');

    switch (status) {
        case 'connected':
            statusElem.classList.add('status-connected');
            textElem.textContent = 'Connected';
            break;
        case 'connecting':
            statusElem.classList.add('status-connecting');
            textElem.textContent = 'Connecting...';
            break;
        case 'disconnected':
            statusElem.classList.add('status-disconnected');
            textElem.textContent = 'Disconnected';
            break;
    }
}
```

## File Size Requirements

**Target Size**: 15-20 KB total (single HTML file with inline CSS and JavaScript)

**Size Breakdown**:
- HTML structure: ~5 KB (9 sensor cards with all fields)
- CSS styles: ~3 KB (responsive grid, marine theme, status indicators)
- JavaScript: ~10 KB (WebSocket, parsing, 9 update functions, unit conversions)
- Comments: ~2 KB (code documentation)

**Why Size Matters**:
- LittleFS partition size limited
- Faster page load on slow WiFi connections
- Lower memory usage during HTTP serving

**Size Optimization Techniques**:
- Minify whitespace (but keep readable for maintenance)
- Use short CSS class names
- Avoid duplicate code (use helper functions)
- No external dependencies (no jQuery, no Bootstrap)

## Browser Compatibility

**Minimum Requirements**:
- ES6 JavaScript support (2015+ browsers)
- WebSocket API support (all modern browsers since 2012)
- CSS Grid support (2017+ browsers)
- Flexbox support (2015+ browsers)

**Supported Browsers**:
- Chrome/Edge: Version 88+ (2021+)
- Firefox: Version 78+ (2020+)
- Safari: Version 14+ (2020+)
- Mobile Safari (iOS): 14+ (2020+)
- Chrome Android: Version 88+ (2021+)

**No Support For**:
- Internet Explorer (any version)
- Legacy mobile browsers (Android WebView <5.0)

**Why This Baseline**:
- Modern boat electronics run current OS versions
- Tablets/phones used for marine navigation are typically recent models
- No need to support legacy desktop browsers on boats

## Performance Requirements

### Initial Page Load

**Target**: <2 seconds on typical boat WiFi

**Breakdown**:
- HTTP request: <10 ms
- LittleFS read: <50 ms
- Network transmission (20 KB @ 5 Mbps): <40 ms
- Browser parsing: <500 ms
- WebSocket connection: <100 ms
- First data message: <1000 ms (1 Hz broadcast)
- **Total**: ~1.7 seconds

### Runtime Performance

**DOM Update Frequency**: 1 Hz (matches WebSocket broadcast rate)

**Update Performance**:
- JSON.parse(): <1 ms (for ~1.5 KB message)
- updateDashboard() execution: <5 ms (9 sensor updates)
- Browser reflow/repaint: <10 ms
- **Total per update**: <20 ms (well under 1000 ms budget)

**No Performance Optimization Needed**:
- 1 Hz update rate is very low (not 60 FPS)
- Small DOM tree (~100 elements)
- No animations or transitions
- No complex calculations

## Error Handling

### JSON Parse Errors

**Behavior**: Log error to console, skip update, continue listening

```javascript
ws.onmessage = (event) => {
    try {
        const data = JSON.parse(event.data);
        updateDashboard(data);
    } catch (error) {
        console.error('JSON parse error:', error);
        // Do not disconnect, just skip this message
    }
};
```

### WebSocket Connection Errors

**Behavior**: Show "Disconnected" status, auto-reconnect after 5 seconds

```javascript
ws.onclose = () => {
    updateConnectionStatus('disconnected');
    setTimeout(connectWebSocket, 5000); // Auto-reconnect
};
```

### Missing Fields in JSON

**Behavior**: Display "--" for missing fields, no error logged

```javascript
function updateValue(elementId, value, formatter, unit) {
    const elem = document.getElementById(elementId);
    if (value === null || value === undefined || !isFinite(value)) {
        elem.textContent = '--';  // Graceful degradation
        elem.classList.add('unavailable');
    } else {
        elem.textContent = formatter ? formatter(value) : value;
        elem.classList.remove('unavailable');
    }
}
```

### Stale Data Detection

**Requirement**: Show visual indicator if data is stale (>5 seconds old)

**Implementation**:
```javascript
function updateLastUpdate(elementId, timestamp) {
    const elem = document.getElementById(elementId);
    const age = Date.now() - timestamp;

    elem.textContent = formatLastUpdate(timestamp);

    if (age > 5000) {
        elem.classList.add('stale-data');  // Red text or warning icon
    } else {
        elem.classList.remove('stale-data');
    }
}
```

## Testing Requirements

### Manual Browser Tests

**Test Cases** (see HTTPFileServerContract.md for full list):
1. **MT-HTML-001**: Load dashboard in Chrome, verify all 9 sensor cards display
2. **MT-HTML-002**: Load in Firefox, verify layout correct
3. **MT-HTML-003**: Load in Safari, verify WebSocket connects
4. **MT-HTML-004**: Load on mobile (iOS Safari), verify responsive layout (1 column)
5. **MT-HTML-005**: Load on tablet, verify 2-column layout
6. **MT-HTML-006**: Disconnect WiFi, verify "Disconnected" status shows
7. **MT-HTML-007**: Reconnect WiFi, verify auto-reconnect within 5 seconds
8. **MT-HTML-008**: Verify data updates at ~1 Hz (watch timestamp)
9. **MT-HTML-009**: Verify unit conversions correct (radians→degrees, m/s→knots)
10. **MT-HTML-010**: Verify "Last update" timestamps update correctly

### Automated Tests (JavaScript Unit Tests)

**Test Framework**: Not required for MVP (manual testing sufficient)

**Future Enhancement**: Add Jest or Mocha unit tests for:
- Unit conversion functions
- formatLastUpdate() edge cases
- updateValue() null handling
- JSON schema validation

## Development Workflow

### File Location

**Source File**: `data/stream.html`

**Why `data/` directory**: PlatformIO convention for LittleFS uploads

### Upload to ESP32

**Command**:
```bash
cd /home/niels/Dev/Poseidon2
pio run --target uploadfs
```

**What Happens**:
1. PlatformIO reads all files in `data/` directory
2. Creates LittleFS filesystem image
3. Uploads image to ESP32 LittleFS partition (separate from firmware)
4. Files persist across firmware updates

### Update Workflow (No Firmware Recompile)

**Steps**:
1. Edit `data/stream.html` on development machine
2. Run `pio run --target uploadfs` (30-60 seconds)
3. ESP32 serves new version immediately (no reboot required)
4. Refresh browser to see changes

**Key Benefit**: UI development cycle independent of firmware compilation (2-3 minutes vs 30-60 seconds)

## Security Considerations

### No Authentication

**Current Design**: No login, no password, no API keys

**Rationale**:
- Private boat network (not internet-exposed)
- Consistent with WebSocketLogger (also unauthenticated)
- Simplicity for MVP

**Future Enhancement**: Add basic auth if needed

### No HTTPS

**Current Design**: Plain HTTP (no TLS/SSL)

**Rationale**:
- Local network only
- ESP32 TLS overhead significant
- No sensitive data (telemetry only)

**Future Enhancement**: Add HTTPS if deploying on public networks

### No Input Validation

**Current Design**: Dashboard is read-only (no user inputs)

**Security**: No injection risks (no forms, no POST requests)

## Constitutional Compliance

### Principle I: Hardware Abstraction Layer (HAL)

- ✅ WebSocket API abstracts network layer
- ✅ No direct TCP/IP socket operations
- ✅ Browser provides HAL for DOM manipulation

### Principle II: Resource-Aware Development

- ✅ File size limited to 15-20 KB
- ✅ No external dependencies (no CDN requests)
- ✅ Minimal memory footprint in browser (~2 MB for page)

### Principle V: Network-Based Debugging

- ✅ Browser console.log() for errors
- ✅ Connection status visible to user
- ✅ No silent failures

### Principle VI: Always-On Operation

- ✅ Auto-reconnect on disconnect (5s delay)
- ✅ No blocking operations in JavaScript
- ✅ Graceful degradation on stale data

### Principle VII: Fail-Safe Operation

- ✅ JSON parse errors handled gracefully
- ✅ Missing fields display as "--"
- ✅ Connection errors trigger reconnect
- ✅ Dashboard remains functional with partial data

## Version History

- **v1.0.0** (2025-10-13): Initial contract definition

# Contract: HTTP File Server Endpoint

**Endpoint**: `/stream`
**Protocol**: HTTP/1.1
**Method**: GET
**Purpose**: Serve static HTML dashboard from LittleFS storage
**Location**: `src/main.cpp` (HTTP endpoint handler)

## Endpoint Contract

### HTTP Request

**URL**: `http://<ESP32_IP>/stream`

**Method**: GET

**Headers** (required by client):
- `Host: <ESP32_IP>` or `Host: poseidon2.local`

**Example Request**:
```http
GET /stream HTTP/1.1
Host: 192.168.1.100
User-Agent: Mozilla/5.0 ...
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
Accept-Language: en-US,en;q=0.5
Connection: keep-alive
```

### HTTP Response (Success)

**Status Code**: 200 OK

**Headers**:
```http
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 18432
Connection: close
```

**Body**: Complete HTML file contents from `/stream.html` in LittleFS

**Example**:
```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Poseidon2 BoatData Dashboard</title>
    <style>
        /* Inline CSS ... */
    </style>
</head>
<body>
    <!-- Dashboard HTML ... -->
    <script>
        // Inline JavaScript ...
    </script>
</body>
</html>
```

### HTTP Response (File Not Found)

**Status Code**: 404 Not Found

**Headers**:
```http
HTTP/1.1 404 Not Found
Content-Type: text/plain
Content-Length: 37
Connection: close
```

**Body**:
```
Dashboard file not found in LittleFS
```

**Logging**:
```json
{
  "component": "HTTPFileServer",
  "event": "FILE_NOT_FOUND",
  "data": {
    "path": "/stream.html",
    "reason": "File does not exist in LittleFS"
  }
}
```

### HTTP Response (LittleFS Error)

**Status Code**: 500 Internal Server Error

**Headers**:
```http
HTTP/1.1 500 Internal Server Error
Content-Type: text/plain
Content-Length: 32
Connection: close
```

**Body**:
```
Error reading dashboard file
```

**Logging**:
```json
{
  "component": "HTTPFileServer",
  "event": "FILE_READ_ERROR",
  "data": {
    "path": "/stream.html",
    "error": "LittleFS read failed"
  }
}
```

## Implementation Requirements

### Endpoint Handler

**Location**: `src/main.cpp` in `onWiFiConnected()` function

**Implementation**:
```cpp
// Add /stream HTTP endpoint for HTML dashboard
webServer->getServer()->on("/stream", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Check if file exists
    if (!LittleFS.exists("/stream.html")) {
        logger.broadcastLog(LogLevel::ERROR, "HTTPFileServer", "FILE_NOT_FOUND",
            F("{\"path\":\"/stream.html\"}"));
        request->send(404, "text/plain", "Dashboard file not found in LittleFS");
        return;
    }

    // Serve file from LittleFS
    request->send(LittleFS, "/stream.html", "text/html");

    // Log access (optional, may be too verbose for production)
    logger.broadcastLog(LogLevel::DEBUG, "HTTPFileServer", "FILE_SERVED",
        String(F("{\"path\":\"/stream.html\",\"client\":\"")) +
        request->client()->remoteIP().toString() + F("\"}"));
});
```

### LittleFS File Location

**Path**: `/stream.html` (root of LittleFS filesystem)

**Size**: 15-20 KB (single HTML file with inline CSS and JavaScript)

**Upload Method**: PlatformIO data folder upload
```bash
# Upload files from data/ directory to LittleFS
pio run --target uploadfs
```

**File Persistence**: Survives firmware updates (LittleFS partition separate from firmware partition)

### Content-Type Header

**MUST BE**: `text/html`

**Why**: Browsers interpret file based on Content-Type header, not URL extension

**Incorrect**: `application/octet-stream`, `text/plain`
**Correct**: `text/html`

**ESPAsyncWebServer automatically sets correct Content-Type** when using `request->send(LittleFS, path, "text/html")`

## File Upload Process

### Development Workflow

**Step 1**: Create HTML file in `data/` directory
```bash
cd /home/niels/Dev/Poseidon2
mkdir -p data
nano data/stream.html  # Create HTML dashboard file
```

**Step 2**: Upload to ESP32 LittleFS
```bash
pio run --target uploadfs
```

**Step 3**: Verify upload
```bash
# Connect to ESP32 serial monitor
pio device monitor

# Should see LittleFS mount success messages
# Or check via WebSocket logs
```

**Step 4**: Test in browser
```
http://192.168.1.100/stream
```

### Production Deployment

**Option A**: Include in firmware upload
```bash
pio run --target upload   # Upload firmware
pio run --target uploadfs # Upload filesystem
```

**Option B**: OTA (Over-The-Air) filesystem update
- Future enhancement (not in current scope)
- Would allow HTML updates without physical access to ESP32

### File Update Workflow

**Without Firmware Recompile**:
1. Modify `data/stream.html` on development machine
2. Run `pio run --target uploadfs` to upload new version
3. ESP32 automatically serves new version on next HTTP request
4. **No firmware recompile or reboot required**

**Key Benefit**: Dashboard UI can be updated independently of backend logic

## Error Handling

### File Missing

**Condition**: `/stream.html` does not exist in LittleFS

**Causes**:
- Filesystem never uploaded
- File deleted accidentally
- LittleFS partition corrupted

**Handling**:
- Return HTTP 404
- Log ERROR level message
- Provide clear error message to user

**User Action**: Upload filesystem via `pio run --target uploadfs`

### LittleFS Mount Failure

**Condition**: LittleFS.begin() fails during ESP32 boot

**Causes**:
- LittleFS partition not formatted
- Flash memory corruption
- Incorrect partition scheme

**Handling**:
- Logged during boot (not in this endpoint)
- All LittleFS operations fail
- Return HTTP 500 for all `/stream` requests

**User Action**: Reformat LittleFS or reflash firmware with correct partition scheme

### File Read Error

**Condition**: LittleFS.exists() returns true, but file cannot be read

**Causes**:
- Flash memory read error
- Filesystem corruption
- Permissions issue (rare on ESP32)

**Handling**:
- Return HTTP 500
- Log ERROR level message
- Provide generic error message (don't expose internal details)

**User Action**: Reflash filesystem or check flash memory health

## Performance Requirements

### Response Time

**Requirement**: Page loads in <2 seconds on typical WiFi connection

**Breakdown**:
- HTTP request processing: <10 ms
- LittleFS file read (20 KB): <50 ms
- Network transmission (20 KB @ 5 Mbps): <40 ms
- Browser parsing/rendering: <1 second
- WebSocket connection establishment: <100 ms
- First BoatData message: <1 second (1 Hz broadcast rate)
- **Total**: ~2.2 seconds (acceptable, within margin)

**Optimization**: HTML file is small (<25 KB), loads quickly even on slow connections

### Concurrent Requests

**Requirement**: Handle 5 concurrent `/stream` requests without blocking

**ESPAsyncWebServer Behavior**:
- Asynchronous file serving (non-blocking)
- Each request processed independently
- No blocking delays for other clients

**Memory Usage**:
- Each active request: ~4 KB send buffer
- 5 concurrent requests: ~20 KB transient memory
- Acceptable impact on ESP32 RAM

## Testing Contract

### Integration Tests (test/test_webui_integration/test_html_serving.cpp)

**Test Cases**:
1. **TC-HTTP-001**: Request `/stream`, verify HTTP 200 response
2. **TC-HTTP-002**: Verify response Content-Type is `text/html`
3. **TC-HTTP-003**: Verify response body contains `<!DOCTYPE html>`
4. **TC-HTTP-004**: Verify response body contains `<script>` tag (JavaScript present)
5. **TC-HTTP-005**: Request `/stream` when file missing, verify HTTP 404
6. **TC-HTTP-006**: Verify HTTP 404 response body contains error message
7. **TC-HTTP-007**: Make 5 concurrent `/stream` requests, verify all succeed
8. **TC-HTTP-008**: Measure response time, verify <2 seconds

### Manual Browser Tests

**Test Cases**:
1. **MT-HTTP-001**: Load `http://<ESP32_IP>/stream` in Chrome, verify page displays
2. **MT-HTTP-002**: Load in Firefox, verify page displays
3. **MT-HTTP-003**: Load in Safari, verify page displays
4. **MT-HTTP-004**: Load on mobile browser (iOS Safari, Chrome Android)
5. **MT-HTTP-005**: Verify page is responsive (mobile-friendly layout)
6. **MT-HTTP-006**: Verify WebSocket connection established automatically
7. **MT-HTTP-007**: Verify BoatData displays within 2 seconds of page load

## Logging Contract

### Component Name

"HTTPFileServer"

### Event Types

| Event | Level | Frequency | Data Fields |
|-------|-------|-----------|-------------|
| `FILE_SERVED` | DEBUG | Per request | path, client IP (optional) |
| `FILE_NOT_FOUND` | ERROR | Rare (config issue) | path, reason |
| `FILE_READ_ERROR` | ERROR | Rare (hardware issue) | path, error description |

### Log Format Examples

See "HTTP Response" sections above for JSON examples.

### Production Considerations

**DEBUG Logging**: May be too verbose for production (1 log per page load)

**Recommendation**: Disable FILE_SERVED debug logging in production, or add rate limiting

## Security Considerations

### No Authentication

**Current Design**: No authentication or authorization

**Rationale**:
- Private boat network (not internet-exposed)
- WebSocket Logger also unauthenticated (consistency)
- Simplicity for MVP

**Future Enhancement**: Add basic auth or API keys if needed

### No HTTPS

**Current Design**: Plain HTTP (no TLS/SSL)

**Rationale**:
- Local network only
- ESP32 TLS overhead significant (CPU, memory)
- No sensitive data transmitted (telemetry only)

**Future Enhancement**: Add HTTPS if deploying on public networks

### No Input Validation

**Current Design**: `/stream` endpoint has no user inputs

**Security**: No injection risks (static file serving only)

## Constitutional Compliance

### Principle I: Hardware Abstraction Layer (HAL)

- ✅ LittleFS provides HAL for flash filesystem
- ✅ ESPAsyncWebServer provides HAL for HTTP protocol

### Principle II: Resource-Aware Development

- ✅ File size limited (<25 KB)
- ✅ Concurrent request memory documented (~20 KB for 5 clients)

### Principle V: Network-Based Debugging

- ✅ Errors logged to WebSocketLogger
- ✅ JSON log format

### Principle VI: Always-On Operation

- ✅ Non-blocking file serving (ESPAsyncWebServer async design)
- ✅ No delays or blocking I/O

### Principle VII: Fail-Safe Operation

- ✅ File not found handled gracefully (HTTP 404)
- ✅ Read errors handled gracefully (HTTP 500)
- ✅ System continues operation on errors

## Version History

- **v1.0.0** (2025-10-13): Initial contract definition

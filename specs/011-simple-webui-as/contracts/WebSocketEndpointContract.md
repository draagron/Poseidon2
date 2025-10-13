# Contract: WebSocket BoatData Endpoint

**Endpoint**: `/boatdata`
**Protocol**: WebSocket (RFC 6455)
**Purpose**: Stream real-time BoatData updates to web clients
**Location**: `src/main.cpp` (global WebSocket instance and event handlers)

## Endpoint Contract

### WebSocket URL

**Pattern**: `ws://<ESP32_IP>/boatdata`

**Examples**:
- `ws://192.168.1.100/boatdata`
- `ws://poseidon2.local/boatdata` (mDNS)

### Connection Lifecycle

#### 1. Connection Establishment (`WS_EVT_CONNECT`)

**Server Behavior**:
- Accept connection immediately (no authentication)
- Increment client count
- Log INFO event to WebSocketLogger
- Add client to broadcast list

**Logging**:
```json
{
  "component": "BoatDataStream",
  "event": "CLIENT_CONNECTED",
  "data": {
    "clientId": 12345,
    "totalClients": 3,
    "clientIP": "192.168.1.50"
  }
}
```

**Maximum Clients**:
- Hard limit: 10 concurrent connections
- If limit exceeded: Close new connection with 1011 status code ("Server overload")

#### 2. Data Streaming (Periodic Broadcast)

**Trigger**: ReactESP timer at 1 Hz (1000 ms interval)

**Server Behavior**:
```cpp
app.onRepeat(1000, []() {
    if (wsBoatData.count() > 0 && boatData != nullptr) {
        // Serialize BoatData to JSON
        String json = BoatDataSerializer::toJSON(boatData);

        if (json.length() > 0) {
            // Broadcast to all connected clients
            wsBoatData.textAll(json);

            // Log DEBUG event (optional, may be too verbose)
            logger.broadcastLog(LogLevel::DEBUG, "BoatDataStream", "BROADCAST",
                String(F("{\"clients\":")) + wsBoatData.count() +
                F(",\"size\":")) + json.length() + F("}"));
        }
    }
});
```

**Broadcast Characteristics**:
- Frequency: 1 Hz (exactly 1000 ms interval)
- Payload: JSON string (~1500-1800 bytes)
- Delivery: `textAll()` method (broadcasts to all clients)
- Non-blocking: ESPAsyncWebServer handles queueing automatically

**Client Receives**:
- WebSocket text frame containing JSON message
- Message conforms to data-model.md schema
- No sequence numbers or acknowledgments

#### 3. Client Disconnection (`WS_EVT_DISCONNECT`)

**Server Behavior**:
- Decrement client count
- Log INFO event to WebSocketLogger
- Remove client from broadcast list automatically

**Logging**:
```json
{
  "component": "BoatDataStream",
  "event": "CLIENT_DISCONNECTED",
  "data": {
    "clientId": 12345,
    "totalClients": 2,
    "reason": "normal_closure"
  }
}
```

#### 4. Error Handling (`WS_EVT_ERROR`)

**Server Behavior**:
- Log ERROR event with error details
- Close connection gracefully
- Continue serving other clients

**Logging**:
```json
{
  "component": "BoatDataStream",
  "event": "WEBSOCKET_ERROR",
  "data": {
    "clientId": 12345,
    "error": "connection_reset"
  }
}
```

### Client-to-Server Messages

**NOT SUPPORTED** - This is a unidirectional stream (server → client only)

**Behavior if client sends message**:
- Ignore message silently
- Do not close connection
- Continue normal broadcasting

**Rationale**: Dashboard is read-only, no control commands needed.

## Implementation Requirements

### Global WebSocket Instance

**Declaration** (in main.cpp, after includes):
```cpp
// Global WebSocket for BoatData streaming
AsyncWebSocket wsBoatData("/boatdata");
```

### Setup Function

**Function Signature**:
```cpp
void setupBoatDataWebSocket(AsyncWebServer* server);
```

**Implementation**:
```cpp
void setupBoatDataWebSocket(AsyncWebServer* server) {
    wsBoatData.onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client,
                          AwsEventType type, void* arg, uint8_t* data, size_t len) {
        switch (type) {
            case WS_EVT_CONNECT:
                logger.broadcastLog(LogLevel::INFO, "BoatDataStream", "CLIENT_CONNECTED",
                    String(F("{\"clientId\":")) + client->id() +
                    F(",\"totalClients\":")) + server->count() + F("}"));
                break;

            case WS_EVT_DISCONNECT:
                logger.broadcastLog(LogLevel::INFO, "BoatDataStream", "CLIENT_DISCONNECTED",
                    String(F("{\"clientId\":")) + client->id() +
                    F(",\"totalClients\":")) + server->count() + F("}"));
                break;

            case WS_EVT_ERROR:
                logger.broadcastLog(LogLevel::ERROR, "BoatDataStream", "WEBSOCKET_ERROR",
                    String(F("{\"clientId\":")) + client->id() + F("}"));
                break;

            case WS_EVT_DATA:
                // Ignore client messages (read-only endpoint)
                break;
        }
    });

    server->addHandler(&wsBoatData);
}
```

### Integration Point (main.cpp onWiFiConnected)

**Location**: After `logger.begin(webServer->getServer(), "/logs");`

```cpp
// Setup BoatData WebSocket endpoint
setupBoatDataWebSocket(webServer->getServer());
```

### Broadcast Timer (main.cpp setup())

**Location**: After ReactESP NMEA polling loops

```cpp
// BoatData broadcast timer (1 Hz)
app.onRepeat(1000, []() {
    if (wsBoatData.count() > 0 && boatData != nullptr) {
        String json = BoatDataSerializer::toJSON(boatData);
        if (json.length() > 0) {
            wsBoatData.textAll(json);
        }
    }
});
```

## Performance Requirements

### Broadcast Latency

**Requirement**: <100 ms from timer trigger to all clients receiving message

**Breakdown**:
- JSON serialization: <50 ms (per BoatDataSerializer contract)
- ESPAsyncWebServer queueing: <10 ms
- WiFi transmission (1.6 KB @ 20 Mbps): <10 ms
- **Total**: <70 ms (30 ms margin)

**Validation**: Measure end-to-end latency with oscilloscope or network analyzer

### Throughput

**Single Client**:
- Payload: ~1.6 KB per message
- Frequency: 1 Hz
- Bandwidth: 1.6 KB/s (~12.8 kbps)

**5 Concurrent Clients** (requirement per spec):
- Total payload: 8 KB per broadcast
- Frequency: 1 Hz
- Bandwidth: 8 KB/s (~64 kbps)
- WiFi capacity: 20 Mbps typical
- **Utilization**: 0.32% of WiFi bandwidth

**10 Concurrent Clients** (maximum enforced limit):
- Total payload: 16 KB per broadcast
- Bandwidth: 16 KB/s (~128 kbps)
- **Utilization**: 0.64% of WiFi bandwidth

### Memory Usage

**Per Client**:
- ESPAsyncWebServer client buffer: ~4 KB (library internal)
- Client metadata: ~100 bytes

**Total for 10 Clients**:
- Client buffers: 40 KB
- Metadata: 1 KB
- **Total**: ~41 KB (12.8% of ESP32 RAM)

## Error Handling

### Serialization Failure

**Condition**: `BoatDataSerializer::toJSON()` returns empty string

**Handling**:
- Skip broadcast for this cycle
- Log WARN event
- Retry on next timer trigger (1 second later)

### Client Buffer Full (Backpressure)

**Condition**: Client is slow, send buffer fills up

**Handling**: ESPAsyncWebServer handles this automatically
- Library queues messages internally
- If queue full, disconnects slow client
- Other clients unaffected

**No Action Required** by application code

### WebSocket Library Errors

**Condition**: ESPAsyncWebServer internal error

**Handling**:
- Log ERROR event
- Close affected client connection
- Continue serving other clients
- System remains operational

## Testing Contract

### Integration Tests (test/test_webui_integration/)

#### test_websocket_connection.cpp

**Test Cases**:
1. **TC-WS-001**: Connect single client, verify connection success
2. **TC-WS-002**: Connect 5 concurrent clients, verify all connected
3. **TC-WS-003**: Connect 11 clients, verify 11th is rejected
4. **TC-WS-004**: Disconnect client, verify graceful closure
5. **TC-WS-005**: Verify client count increments/decrements correctly

#### test_json_data_flow.cpp

**Test Cases**:
1. **TC-FLOW-001**: Verify client receives JSON message within 2 seconds of connection
2. **TC-FLOW-002**: Verify JSON messages arrive at ~1 Hz rate (950-1050 ms intervals)
3. **TC-FLOW-003**: Verify JSON schema matches data-model.md
4. **TC-FLOW-004**: Verify GPS data in JSON matches BoatData GPS values
5. **TC-FLOW-005**: Verify all 9 sensor groups present in each message

#### test_multi_client.cpp

**Test Cases**:
1. **TC-MULTI-001**: Connect 5 clients, verify all receive same JSON message
2. **TC-MULTI-002**: Connect 5 clients, verify messages arrive within 50 ms of each other
3. **TC-MULTI-003**: Disconnect 1 client, verify others continue receiving
4. **TC-MULTI-004**: Connect/disconnect clients rapidly, verify system stability
5. **TC-MULTI-005**: Connect 10 clients, verify broadcast completes within 100 ms

### Performance Tests (test/test_webui_units/)

#### test_throttling.cpp

**Test Cases**:
1. **TC-THROTTLE-001**: Measure broadcast intervals, verify 1000 ms ±50 ms
2. **TC-THROTTLE-002**: Verify no broadcasts if no clients connected (optimization)
3. **TC-THROTTLE-003**: Verify broadcast continues at 1 Hz even with BoatData updates at 10 Hz

## Logging Contract

### Component Name

"BoatDataStream"

### Event Types

| Event | Level | Frequency | Data Fields |
|-------|-------|-----------|-------------|
| `CLIENT_CONNECTED` | INFO | Per connection | clientId, totalClients, clientIP (optional) |
| `CLIENT_DISCONNECTED` | INFO | Per disconnection | clientId, totalClients, reason (optional) |
| `WEBSOCKET_ERROR` | ERROR | Rare | clientId, error description |
| `BROADCAST` | DEBUG | 1 Hz (may be too verbose) | clients, size |
| `MAX_CLIENTS_EXCEEDED` | WARN | When limit hit | clientId, maxClients |

### Log Format Examples

See "Connection Lifecycle" section above for JSON examples.

## Client-Side JavaScript Contract

### Connection Pattern

```javascript
const wsUrl = 'ws://' + location.host + '/boatdata';
let ws = new WebSocket(wsUrl);

ws.onopen = () => {
    console.log('Connected to BoatData stream');
    updateConnectionStatus(true);
};

ws.onmessage = (event) => {
    try {
        const data = JSON.parse(event.data);
        updateDashboard(data);
    } catch (err) {
        console.error('Invalid JSON:', err);
    }
};

ws.onerror = (error) => {
    console.error('WebSocket error:', error);
};

ws.onclose = () => {
    console.log('Disconnected from BoatData stream');
    updateConnectionStatus(false);

    // Auto-reconnect after 5 seconds
    setTimeout(() => {
        ws = new WebSocket(wsUrl);
    }, 5000);
};
```

### Expected Client Behavior

1. **Connect on page load**: Establish WebSocket connection immediately
2. **Receive messages**: Parse JSON, update DOM
3. **Handle disconnections**: Show "Disconnected" status, auto-reconnect
4. **Handle errors**: Log to console, display error message
5. **Never send messages**: Read-only endpoint

## Constitutional Compliance

### Principle I: Hardware Abstraction Layer (HAL)

- ✅ ESPAsyncWebServer provides HAL for WebSocket protocol
- ✅ No direct TCP/IP socket operations in application code

### Principle II: Resource-Aware Development

- ✅ Client limit enforced (10 max)
- ✅ Memory usage documented (~41 KB for 10 clients)
- ✅ No unbounded buffers

### Principle V: Network-Based Debugging

- ✅ All connection events logged to WebSocketLogger
- ✅ JSON log format

### Principle VI: Always-On Operation

- ✅ Non-blocking broadcast (ESPAsyncWebServer async design)
- ✅ ReactESP timer (non-blocking event loop)
- ✅ No delays or blocking I/O

### Principle VII: Fail-Safe Operation

- ✅ Serialization failure handled gracefully (skip cycle)
- ✅ Client errors isolated (no impact on other clients)
- ✅ System continues operation on WebSocket errors

## Version History

- **v1.0.0** (2025-10-13): Initial contract definition

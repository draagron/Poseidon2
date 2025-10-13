# Contract: DeviceInfoCollector

**Component**: `src/components/DeviceInfoCollector.h` / `.cpp`
**Purpose**: Poll NMEA2000 device list, extract metadata, enrich SourceRegistry
**Type**: Stateful component (periodic polling)

## Responsibilities

1. Poll `tN2kDeviceList` every 5 seconds via ReactESP timer callback
2. Detect device list updates using `ReadResetIsListUpdated()`
3. Extract device metadata from `tDevice` objects
4. Correlate devices with `MessageSource` entries by SID
5. Update `SourceRegistry` with device metadata
6. Send WebSocket notifications for device discovery/updates
7. Handle discovery timeout (60 seconds) for non-compliant devices

## Dependencies

### Input Dependencies
- `tN2kDeviceList*` - NMEA2000 library's device list instance
- `SourceRegistry*` - Source statistics registry
- `WebSocketLogger*` - Logging subsystem (optional)

### Output Dependencies
- `SourceRegistry::updateDeviceInfo()` - Updates MessageSource.deviceInfo
- `WebSocketLogger::broadcastLog()` - DEBUG level device discovery events

## Public Interface

### Constructor
```cpp
/**
 * @brief Create DeviceInfoCollector with dependencies
 *
 * @param deviceList Pointer to tN2kDeviceList instance (must outlive this object)
 * @param registry Pointer to SourceRegistry instance (must outlive this object)
 * @param logger Optional WebSocketLogger for diagnostic events
 *
 * @pre deviceList != nullptr
 * @pre registry != nullptr
 * @post Collector ready to poll, but not started (call init() to start)
 */
DeviceInfoCollector(tN2kDeviceList* deviceList,
                    SourceRegistry* registry,
                    WebSocketLogger* logger = nullptr);
```

### Initialization
```cpp
/**
 * @brief Initialize and start periodic polling
 *
 * Sets up ReactESP timer callback for 5-second polling interval.
 *
 * @param app ReactESP application instance
 *
 * @post ReactESP timer registered, polling active
 * @post First poll occurs after 5 seconds
 */
void init(ReactESP& app);
```

### Manual Polling (for testing)
```cpp
/**
 * @brief Manually trigger device list poll
 *
 * Checks for device list updates and processes discovered devices.
 * Intended for unit/integration testing without ReactESP timer.
 *
 * @return Number of sources updated with new device metadata
 *
 * @post SourceRegistry deviceInfo fields updated for discovered devices
 * @post WebSocket log messages sent for discoveries/updates
 */
uint8_t pollDeviceList();
```

### Statistics
```cpp
/**
 * @brief Get total number of discovered devices
 *
 * @return Count of MessageSource entries with deviceInfo.hasInfo == true
 */
uint8_t getDiscoveredCount() const;

/**
 * @brief Get last poll timestamp
 *
 * @return millis() timestamp of last pollDeviceList() call
 */
unsigned long getLastPollTime() const;
```

## Behavior Contracts

### BC-1: Polling Frequency
**Invariant**: `pollDeviceList()` called every 5000ms ± 50ms (ReactESP timer precision)

**Test**: Verify timer callback interval via timestamps
```cpp
TEST_CASE("DeviceInfoCollector polls every 5 seconds") {
    // Record timestamps of 10 consecutive polls
    // Assert intervals are 5000ms ± 50ms tolerance
}
```

### BC-2: Performance Budget
**Invariant**: `pollDeviceList()` completes in <10ms (FR-022)

**Test**: Measure execution time with 20 simulated devices
```cpp
TEST_CASE("pollDeviceList completes in <10ms with 20 devices") {
    unsigned long start = millis();
    collector.pollDeviceList();
    unsigned long duration = millis() - start;
    REQUIRE(duration < 10);
}
```

### BC-3: Metadata Extraction Correctness
**Invariant**: All `tDevice` fields correctly copied to `DeviceInfo` struct

**Test**: Compare extracted metadata with mock tDevice data
```cpp
TEST_CASE("pollDeviceList extracts all tDevice fields correctly") {
    // Setup: Create mock tDevice with known values
    // Act: pollDeviceList()
    // Assert: MessageSource.deviceInfo matches mock data
}
```

### BC-4: Discovery Timeout Handling
**Invariant**: Sources without device info after 60 seconds marked as "Unknown (timeout)"

**Test**: Simulate source with no device announcement for 60+ seconds
```cpp
TEST_CASE("Sources timeout after 60 seconds without discovery") {
    // Setup: SourceRegistry has NMEA2000-42 source
    // Setup: tN2kDeviceList returns nullptr for SID 42
    // Setup: 61 seconds elapse (mock millis())
    // Act: pollDeviceList()
    // Assert: WebSocket log "DEVICE_TIMEOUT" sent
    // Assert: deviceInfo.hasInfo == false
}
```

### BC-5: Manufacturer Name Lookup
**Invariant**: `manufacturerCode` correctly mapped to `manufacturer` string

**Test**: Verify lookup for known codes
```cpp
TEST_CASE("Manufacturer code 275 maps to Garmin") {
    // Setup: tDevice with manufacturerCode=275
    // Act: pollDeviceList()
    // Assert: deviceInfo.manufacturer == "Garmin"
}
```

### BC-6: WebSocket Notification on Discovery
**Invariant**: Device discovery triggers `DEVICE_DISCOVERED` WebSocket log

**Test**: Mock logger captures log messages
```cpp
TEST_CASE("pollDeviceList logs DEVICE_DISCOVERED on first discovery") {
    // Setup: Mock WebSocketLogger
    // Setup: New device in tN2kDeviceList
    // Act: pollDeviceList()
    // Assert: Logger received DEBUG log with event "DEVICE_DISCOVERED"
}
```

### BC-7: Idempotent Updates
**Invariant**: Repeated polls with unchanged device list → no duplicate notifications

**Test**: Poll twice without device list changes
```cpp
TEST_CASE("pollDeviceList does not send duplicate notifications") {
    // Setup: tN2kDeviceList.ReadResetIsListUpdated() returns false
    // Act: pollDeviceList() twice
    // Assert: No WebSocket logs sent on second poll
}
```

### BC-8: SID Correlation
**Invariant**: Device metadata matched to MessageSource by SID

**Test**: Verify correct source-device pairing
```cpp
TEST_CASE("Device metadata matched to correct MessageSource by SID") {
    // Setup: SourceRegistry has NMEA2000-42 and NMEA2000-7
    // Setup: tN2kDeviceList has devices for SID 42 and 7
    // Act: pollDeviceList()
    // Assert: SID 42 → Garmin GPS 17x
    // Assert: SID 7 → Furuno GP330B
}
```

## Error Handling

### E-1: Null Device List
**Scenario**: `tN2kDeviceList* == nullptr` (constructor precondition violation)

**Behavior**: Constructor should assert/crash in debug builds, undefined behavior in release

**Rationale**: Critical dependency, system cannot function without device list

### E-2: Null Source Registry
**Scenario**: `SourceRegistry* == nullptr` (constructor precondition violation)

**Behavior**: Same as E-1 (critical dependency)

### E-3: Device Not Found
**Scenario**: `tN2kDeviceList::FindDeviceBySource(sid)` returns `nullptr`

**Behavior**: Skip update for this source, no error logged (normal for undiscovered devices)

**Test**: Verify graceful handling
```cpp
TEST_CASE("pollDeviceList handles missing devices gracefully") {
    // Setup: SourceRegistry has NMEA2000-99
    // Setup: tN2kDeviceList has no device for SID 99
    // Act: pollDeviceList()
    // Assert: No crash, no error logs
}
```

### E-4: Manufacturer Code Unknown
**Scenario**: `ManufacturerLookup::getManufacturerName()` returns "Unknown (code)"

**Behavior**: Store "Unknown (275)" string in `deviceInfo.manufacturer`

**Test**: Verify fallback behavior
```cpp
TEST_CASE("Unknown manufacturer codes handled with fallback string") {
    // Setup: tDevice with manufacturerCode=99999
    // Act: pollDeviceList()
    // Assert: deviceInfo.manufacturer == "Unknown (99999)"
}
```

## Memory Safety

### MS-1: String Buffer Overflow Protection
**Requirement**: `strcpy()` replaced with `strncpy()` for all string copies

**Implementation**:
```cpp
// SAFE: Bounded copy with null termination
strncpy(deviceInfo.manufacturer, manufacturerName, 15);
deviceInfo.manufacturer[15] = '\0';  // Ensure null termination
```

**Test**: Verify long strings truncated
```cpp
TEST_CASE("Long model IDs truncated to 24 characters") {
    // Setup: tDevice.ModelID = "ThisIsAVeryLongModelIdentifierString"
    // Act: pollDeviceList()
    // Assert: deviceInfo.modelId == "ThisIsAVeryLongModelIde\0"
}
```

### MS-2: Stack Usage
**Requirement**: No large stack-allocated buffers in `pollDeviceList()`

**Rationale**: ReactESP timer callbacks run in loop() context with limited stack

**Implementation**: Use only small local variables (pointers, counters, timestamps)

## Performance Requirements

| Metric | Target | Test Method |
|--------|--------|-------------|
| Poll cycle time | <10ms | Measure with millis() timestamps |
| Memory footprint | <500 bytes | Static RAM analysis (no heap alloc) |
| WebSocket log overhead | <100 bytes per discovery | Log message size validation |

## Test Coverage

### Unit Tests
- Constructor initialization (dependency injection)
- `pollDeviceList()` with empty device list
- `pollDeviceList()` with 1 device
- `pollDeviceList()` with 20 devices
- Manufacturer name lookup (known/unknown codes)
- Discovery timeout detection (59s vs 61s)

### Integration Tests
- End-to-end device discovery (mock tN2kDeviceList)
- WebSocket notification delivery (mock logger)
- SourceRegistry integration (real SourceRegistry instance)
- Discovery timeout workflow (advance mock millis())
- Device metadata update (change firmware version)

### Contract Tests
- All 8 Behavior Contracts (BC-1 through BC-8)
- All 4 Error Handling scenarios (E-1 through E-4)
- All 2 Memory Safety requirements (MS-1, MS-2)

---

**Contract Version**: 1.0
**Last Updated**: 2025-10-13
**Status**: Ready for implementation

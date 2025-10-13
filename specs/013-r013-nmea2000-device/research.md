# Research: NMEA2000 Device Discovery Implementation

**Feature**: NMEA2000 Device Discovery and Identification
**Date**: 2025-10-13
**Phase**: 0 (Research)

## Purpose

This document captures research findings on the NMEA2000 library's device discovery capabilities, specifically the `tN2kDeviceList` class, to inform the implementation of automatic device metadata extraction for source enrichment in the Poseidon2 gateway.

## NMEA2000 Library API Analysis

### tN2kDeviceList Class Overview

**Purpose**: Automatic tracking and management of all devices on the NMEA2000 CAN bus.

**Documentation**: https://ttlappalainen.github.io/NMEA2000/pg_lib_ref.html
**Header**: https://github.com/ttlappalainen/NMEA2000/blob/master/src/N2kDeviceList.h

### Key Methods

#### 1. Constructor
```cpp
tN2kDeviceList(tNMEA2000 *pNMEA2000);
```
- Requires pointer to global NMEA2000 object
- Automatically registers itself with the NMEA2000 handler

#### 2. ReadResetIsListUpdated()
```cpp
bool ReadResetIsListUpdated();
```
- Returns `true` if device list has changed since last call (new devices discovered or metadata updated)
- Resets internal update flag after reading (stateful operation)
- **Usage pattern**: Call periodically (e.g., every 5 seconds) to check for device list changes

#### 3. FindDeviceBySource()
```cpp
const tDevice *FindDeviceBySource(uint8_t Source);
```
- Lookup device by NMEA2000 source address (SID: 0-252)
- Returns pointer to `tDevice` object or `nullptr` if not found
- **Key for correlation**: Use this to match device metadata with existing `MessageSource` entries

#### 4. FindDeviceByName()
```cpp
const tDevice *FindDeviceByName(uint64_t NAME);
```
- Lookup device by unique 64-bit NMEA2000 NAME identifier
- Useful for tracking devices across source address changes

#### 5. FindDeviceByIDs()
```cpp
const tDevice *FindDeviceByIDs(uint16_t ManufacturerCode, uint32_t UniqueNumber);
```
- Lookup device by manufacturer code + unique serial number
- Alternative identification method for firmware updates

#### 6. FindDeviceByProduct()
```cpp
const tDevice *FindDeviceByProduct(uint16_t ManufacturerCode, uint16_t ProductCode, uint8_t Source=0xff);
```
- Lookup device by manufacturer + product code
- Optional source parameter for disambiguation

#### 7. Count()
```cpp
uint8_t Count();
```
- Returns total number of discovered devices on bus
- Maximum: 254 devices (NMEA2000 address space: 0-252, plus broadcast 255)

#### 8. GetDeviceLastMessageTime()
```cpp
unsigned long GetDeviceLastMessageTime(uint8_t Source);
```
- Returns millis() timestamp of last message from source
- Useful for detecting inactive/disconnected devices

### tDevice Structure (Device Metadata)

**Note**: Exact structure definition not fully documented in web API docs, but based on NMEA2000 standard and library usage patterns, the following fields are available:

```cpp
struct tDevice {
    uint8_t Source;                // Source address (SID: 0-252)
    uint64_t DeviceName;           // 64-bit unique NAME
    uint16_t ManufacturerCode;     // NMEA2000 manufacturer code
    uint32_t UniqueNumber;         // Serial number / unique identifier
    uint16_t ProductCode;          // Product code
    uint8_t DeviceInstance;        // Device instance (for multi-device systems)
    uint8_t DeviceFunction;        // NMEA2000 device function
    uint8_t DeviceClass;           // NMEA2000 device class
    const char *ModelID;           // Model identifier string
    const char *SwCode;            // Software version string
    // ... additional fields
};
```

**Key Fields for User Story 1 (P1)**:
- `Source` → Correlate with sourceId "NMEA2000-{SID}"
- `ManufacturerCode` → Lookup manufacturer name via `ManufacturerLookup` utility
- `ModelID` → Display as model name
- `UniqueNumber` → Display as serial number
- `SwCode` → Display as software version
- `DeviceInstance`, `DeviceClass`, `DeviceFunction` → Additional diagnostic metadata

### Device Discovery Process (Inferred from NMEA2000 Standard)

1. **Device Announcement**: NMEA2000 devices broadcast Product Information (PGN 126996) and Configuration Information (PGN 126998) on bus startup and periodically
2. **Library Handling**: `tN2kDeviceList` automatically listens for these PGNs via `HandleMsg()` method
3. **Update Flag**: When new device discovered or metadata changes, internal flag set
4. **Polling Pattern**: Application calls `ReadResetIsListUpdated()` to check for changes without blocking

### Initialization Pattern (Inferred)

Based on NMEA2000 library patterns and Arduino conventions:

```cpp
// Global instances (main.cpp)
tNMEA2000* nmea2000 = nullptr;
tN2kDeviceList* deviceList = nullptr;

void setup() {
    // Initialize NMEA2000 library
    nmea2000 = new tNMEA2000_esp32();
    nmea2000->SetN2kCANReceiveFrameBufSize(150);
    nmea2000->SetN2kCANMsgBufSize(8);
    // ... other NMEA2000 configuration
    nmea2000->Open();

    // Initialize device list AFTER NMEA2000 is configured
    deviceList = new tN2kDeviceList(nmea2000);

    // Setup ReactESP timer for periodic polling
    app.onRepeat(5000, []() {
        if (deviceList->ReadResetIsListUpdated()) {
            // Device list changed - iterate and update SourceRegistry
            updateSourcesWithDeviceInfo();
        }
    });
}

void updateSourcesWithDeviceInfo() {
    // For each known source in SourceRegistry
    for (MessageSource* source : sourceRegistry.getAllSources()) {
        if (source->protocol == ProtocolType::NMEA2000) {
            const tDevice* device = deviceList->FindDeviceBySource(source->sid);
            if (device != nullptr) {
                // Extract metadata and update source->deviceInfo
                // Send WebSocket delta update
            }
        }
    }
}
```

### Manufacturer Code Lookup

**Problem**: `tDevice.ManufacturerCode` is a uint16_t numeric code (e.g., 275 for Garmin, 1855 for Furuno), but we need human-readable names for the UI.

**Solution Options**:
1. **NMEA2000 library built-in**: The library may include a manufacturer name lookup function (needs verification)
2. **Custom static lookup table**: Implement `ManufacturerLookup` utility with ~50 common marine manufacturers
3. **External database**: Maintain JSON file with manufacturer codes (overkill for embedded system)

**Recommendation**: Implement custom `ManufacturerLookup::getManufacturerName(uint16_t code)` utility as static lookup table.

**Reference**: NMEA2000 manufacturer codes maintained by NMEA organization: https://www.nmea.org/nmea-2000.html

### Device List Capacity

**NMEA2000 Standard Limit**: 254 devices (address space 0-252, broadcast 255)

**Library Internal Limit**: Likely 20-50 devices depending on configuration (needs verification from library implementation)

**Poseidon2 Scope**: Max 50 total sources (NMEA2000 + NMEA0183), so library capacity should be sufficient for typical marine installations (5-10 NMEA2000 devices).

**Overflow Handling**: When library's internal device list is full, older devices are evicted. Our system marks sources without corresponding device list entries as "Unknown device" after 60-second timeout.

## NMEA0183 Device Identification

**Problem**: NMEA0183 protocol has no device discovery mechanism (plain-text ASCII sentences).

**Solution**: Static lookup table mapping talker IDs (2-character prefixes) to device type descriptions.

**Common Talker IDs**:
- `GP` → GPS Receiver
- `AP` → Autopilot
- `VH` → VHF Radio
- `HC` → Heading Compass
- `SD` → Depth Sounder
- `VW` → Wind Sensor
- `II` → Integrated Instrumentation (generic)

**Implementation**: `TalkerIdLookup::getTalkerDescription(const char* talkerId)` utility with static string map.

**Reference**: NMEA0183 talker IDs defined in IEC 61162-1 standard.

## Existing Codebase Integration Points

### 1. SourceRegistry.h / SourceRegistry.cpp

**Current State**: Tracks message sources (SID-based) with frequency statistics, staleness detection.

**Required Changes**:
- Add `DeviceInfo` struct to `MessageSource` in `SourceStatistics.h`
- Add `updateDeviceInfo(const char* sourceId, const DeviceInfo& info)` method to `SourceRegistry`
- Extend `recordUpdate()` to check for undiscovered devices and track discovery timeout (60 seconds)

### 2. SourceStatsSerializer.h / SourceStatsSerializer.cpp

**Current State**: Serializes source statistics to JSON for WebSocket streaming (schema v1).

**Required Changes**:
- Increment schema version from v1 to v2
- Add `deviceInfo` object to JSON serialization for both full snapshot and delta updates
- Handle `hasInfo=false` case (null/placeholder in JSON)

### 3. SourceStatsHandler.cpp

**Current State**: Manages WebSocket connections, periodic updates (batched every 100ms), full snapshot on connect.

**Required Changes**:
- Send delta updates when device metadata discovered or updated
- Send timeout notification when 60-second discovery timeout expires for a source

### 4. main.cpp

**Current State**: Initializes NMEA2000 (`nmea2000 = new tNMEA2000_esp32()`), configures ReactESP event loops.

**Required Changes**:
- Create global `tN2kDeviceList* deviceList` pointer
- Initialize after NMEA2000 configuration: `deviceList = new tN2kDeviceList(nmea2000)`
- Create `DeviceInfoCollector` instance with dependencies (deviceList, sourceRegistry, logger)
- Setup ReactESP timer: `app.onRepeat(5000, []() { deviceCollector->pollDeviceList(); })`

### 5. data/sources.html

**Current State**: Displays sources grouped by category, shows frequency/staleness indicators.

**Required Changes**:
- Add expandable details section per source (JavaScript collapsible UI)
- Display device metadata fields (manufacturer, model, serial, software version)
- Display placeholder text: "Discovering..." or "Unknown (non-compliant)"
- Responsive layout for mobile (320px viewport)

## Performance Considerations

### Memory Footprint

**DeviceInfo struct**: ~68 bytes per source
- 1 bool (hasInfo): 1 byte
- 4 uint fields (manufacturerCode, productCode, serialNumber, deviceInstance, deviceClass, deviceFunction): 4×2-4 = ~12 bytes
- 3 fixed strings (manufacturer[16], modelId[24], softwareVersion[12]): 52 bytes
- Total: ~68 bytes

**50 sources**: 50 × 68 = 3,400 bytes (~3.4KB)

**Library overhead**: tN2kDeviceList internal storage unknown, estimated ~2KB for 20 devices

**Total estimate**: <5KB additional RAM (acceptable per constitutional principle II)

### Polling Performance

**Target**: <10ms per poll cycle (FR-022)

**Operations per cycle**:
1. `deviceList->ReadResetIsListUpdated()`: O(1) flag check (~1μs)
2. Iterate 50 sources: O(n) = 50 iterations
3. `deviceList->FindDeviceBySource(sid)`: O(1) hash lookup (estimated ~10μs per lookup)
4. Metadata extraction + string copy: ~50μs per device
5. SourceRegistry update: ~20μs per source

**Worst-case estimate**: 50 × (10μs + 50μs + 20μs) = 50 × 80μs = 4ms

**Conclusion**: Well under 10ms budget, even with conservative estimates.

### WebSocket Message Size

**Baseline (v1 schema)**: ~150 bytes per source (JSON)

**Added device metadata (v2 schema)**: ~120 bytes per source
- manufacturer: ~20 bytes
- modelId: ~30 bytes
- serialNumber: ~15 bytes
- softwareVersion: ~15 bytes
- manufacturerCode, productCode, etc.: ~40 bytes

**Total per source (v2)**: ~270 bytes

**Full snapshot (10 sources)**: ~2.7KB (acceptable for WebSocket frame, under 4KB)

**Delta update (single source)**: ~300 bytes (minimal impact)

## Risk Assessment

### Risk 1: NMEA2000 Library API Changes

**Likelihood**: Low (library is mature, stable API)

**Impact**: Medium (requires code changes if API changes)

**Mitigation**: Use library's official API documentation, avoid accessing internal/private fields, implement HAL-like abstraction for DeviceInfoCollector to isolate library dependencies.

### Risk 2: Device List Capacity Exceeded

**Likelihood**: Low (typical marine installations have 5-10 NMEA2000 devices)

**Impact**: Low (non-critical feature - statistics tracking continues even if device metadata unavailable)

**Mitigation**: Log warning when device list capacity warning detected, gracefully handle `FindDeviceBySource()` returning `nullptr`, mark sources as "Unknown device" after 60s timeout.

### Risk 3: Non-Compliant NMEA2000 Devices

**Likelihood**: Medium (some older devices may not broadcast Product Information PGNs)

**Impact**: Low (devices still function, just no metadata available)

**Mitigation**: 60-second discovery timeout transitions sources to "Unknown (non-compliant)" state, WebSocket notification sent to clients, UI displays placeholder text.

### Risk 4: Performance Impact on Message Processing

**Likelihood**: Low (polling cycle estimated at <4ms, runs every 5 seconds)

**Impact**: Low (ReactESP event loop is non-blocking, message handlers run independently)

**Mitigation**: ReactESP timer callback is non-blocking, measure actual poll cycle time via `millis()` timestamps, add WebSocket logging for performance diagnostics.

### Risk 5: Memory Fragmentation from Dynamic Allocation

**Likelihood**: Low (library uses internal static buffers, our DeviceInfo is statically allocated within MessageSource)

**Impact**: Medium (heap fragmentation can cause ESP32 crashes over long runtime)

**Mitigation**: Use static allocation for DeviceInfo struct (embedded in MessageSource), avoid `String` class in device metadata (use fixed `char[]` buffers), monitor heap usage via WebSocket logging.

## Implementation Strategy

### Phase 1: Core Device Discovery (US1 - P1)

**Components**:
1. `DeviceInfo` struct in `SourceStatistics.h`
2. `DeviceInfoCollector` component (polling, extraction, SourceRegistry updates)
3. `ManufacturerLookup` utility (static lookup table)
4. Extend `SourceRegistry` with `updateDeviceInfo()` method
5. Initialize `tN2kDeviceList` in `main.cpp`
6. ReactESP timer for 5-second polling

**Testing**:
- Contract tests: `DeviceInfoCollector` interface validation
- Integration tests: End-to-end device discovery with mock NMEA2000 devices
- Unit tests: `ManufacturerLookup` accuracy, DeviceInfo struct serialization

### Phase 2: WebSocket Integration (US2 - P2)

**Components**:
1. Extend `SourceStatsSerializer` for v2 schema (deviceInfo in JSON)
2. Increment schema version constant to v2
3. Update `SourceStatsHandler` to send delta updates on device discovery
4. Handle 60-second timeout notification

**Testing**:
- Integration tests: WebSocket full snapshot with device metadata
- Integration tests: WebSocket delta updates on device discovery/update
- Integration tests: Timeout notification message validation

### Phase 3: Web UI Display (US3 - P3)

**Components**:
1. Modify `data/sources.html` - add expandable details sections
2. JavaScript: Handle v2 schema, render device metadata
3. CSS: Responsive layout for mobile (320px viewport)

**Testing**:
- Manual browser testing: Desktop (1920px), tablet (768px), mobile (320px)
- Validate expandable UI behavior (collapse/expand)
- Verify placeholder text display ("Discovering...", "Unknown device")

### Phase 4: NMEA0183 Descriptions (US4 - P4)

**Components**:
1. `TalkerIdLookup` utility (static lookup table)
2. Integrate lookup in `SourceRegistry::recordUpdate()` for NMEA0183 sources
3. Update UI to display talker descriptions

**Testing**:
- Unit tests: `TalkerIdLookup` coverage for common talker IDs
- Integration tests: NMEA0183 source display with descriptions

## Open Questions

1. **Q: Does NMEA2000 library include built-in manufacturer name lookup?**
   **A**: Needs verification from library source code. If not, implement custom `ManufacturerLookup`.

2. **Q: What is the exact structure definition of `tDevice`?**
   **A**: Needs inspection of `N2kDeviceList.h` header file for complete field list and types.

3. **Q: Does `tN2kDeviceList` require explicit `HandleMsg()` calls, or is it automatic?**
   **A**: Likely automatic when created with `tN2kDeviceList(pNMEA2000)` constructor - library handles message routing internally.

4. **Q: What is the library's internal device list capacity?**
   **A**: Needs inspection of library implementation or testing with multiple devices to determine limit.

5. **Q: How are manufacturer codes obtained? Is there a public registry?**
   **A**: NMEA organization maintains registry (https://www.nmea.org/nmea-2000.html), but may require membership access. Alternative: crowdsource common codes from marine electronics community or use library's built-in lookup if available.

## Next Steps (Phase 1)

1. **Inspect NMEA2000 library source code** (`N2kDeviceList.h`, `N2kDeviceList.cpp`) to confirm exact `tDevice` structure definition
2. **Verify manufacturer name lookup** - check if library provides `GetManufacturerName()` or similar function
3. **Create data-model.md** - Define `DeviceInfo` struct, JSON schema v2, database/storage model (none required)
4. **Create contracts/** - Define `DeviceInfoCollector`, `ManufacturerLookup`, `TalkerIdLookup` interface contracts
5. **Create quickstart.md** - Hardware validation guide (connect 2-3 NMEA2000 devices, verify discovery via WebSocket logs)
6. **Generate tasks.md** via `/tasks` command - Dependency-ordered task breakdown for implementation

---

**Research Phase Complete**: Ready for Phase 1 (Design artifacts generation).

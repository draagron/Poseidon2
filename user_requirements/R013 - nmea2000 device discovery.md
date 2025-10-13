# R013 - NMEA2000 Device Discovery and Identification

## User Requirement

As a marine system administrator, I need the Poseidon2 gateway to automatically discover and identify NMEA2000 devices on the bus, including their manufacturer, model name, serial number, and device instance information, so that I can understand which specific hardware is providing each data source instead of just seeing anonymous SID numbers.

## Context

The current source statistics system (R012) tracks NMEA2000 sources by their SID (Source Identifier) only, displaying entries like "NMEA2000-42" without any information about what device this actually is. While this tells me *that* there's a source, it doesn't tell me *which* device it is (e.g., "Garmin GPS 17x S/N:123456789" vs "Furuno GP330B").

The NMEA2000 library provides a `tN2kDeviceList` class (demonstrated in the DeviceAnalyzer.ino example) that automatically discovers devices on the bus and extracts rich metadata including:
- Manufacturer code and name (e.g., 275 = Garmin)
- Model ID (e.g., "GPS 17x")
- Product code
- Software version
- Serial number (unique number)
- Device instance
- Device class and function
- Transmitted/received PGN lists

This feature should integrate device discovery into the existing source statistics system, enriching each NMEA2000 source entry with this metadata.

## Reference Implementation

The NMEA2000 library's DeviceAnalyzer example shows the pattern:
- https://github.com/ttlappalainen/NMEA2000/blob/master/Examples/DeviceAnalyzer/DeviceAnalyzer.ino
- https://github.com/ttlappalainen/NMEA2000/blob/master/src/N2kDeviceList.h

Key API methods:
- `tN2kDeviceList::ReadResetIsListUpdated()` - Check if devices changed
- `tN2kDeviceList::Count()` - Number of known devices
- `tN2kDeviceList::FindDeviceBySource(uint8_t Source)` - Get device by SID
- `tDevice::GetManufacturerCode()`, `GetModelID()`, `GetUniqueNumber()`, etc.

## Desired Behavior

### Device Discovery
1. System automatically discovers NMEA2000 devices as they announce themselves on the bus
2. Device information is extracted and correlated with existing source statistics entries
3. Sources display enriched information: "NMEA2000-42: Garmin GPS 17x (S/N: 123456789)" instead of just "NMEA2000-42"
4. Discovery happens continuously in the background without blocking message processing

### WebUI Display
The source statistics dashboard (`/sources`) should display device information when available:

**Before (current):**
```
GPS
  └─ PGN 129025 (Position Rapid Update)
      ├─ NMEA2000-42 [10.2 Hz, Fresh]
      └─ NMEA2000-7 [1.0 Hz, Stale]
```

**After (enhanced):**
```
GPS
  └─ PGN 129025 (Position Rapid Update)
      ├─ NMEA2000-42 [10.2 Hz, Fresh]
      │   ├─ Manufacturer: Garmin (275)
      │   ├─ Model: GPS 17x
      │   ├─ Serial: 123456789
      │   └─ Software: v2.30
      └─ NMEA2000-7 [1.0 Hz, Stale]
          └─ Device info: Discovering...
```

### WebSocket Integration
The existing `/source-stats` WebSocket endpoint should include device information in messages:

```json
{
  "event": "sourceUpdate",
  "sourceId": "NMEA2000-42",
  "category": "GPS",
  "messageType": "PGN129025",
  "changes": {
    "frequency": 10.2,
    "timeSinceLast": 98,
    "isStale": false,
    "deviceInfo": {
      "hasInfo": true,
      "manufacturerCode": 275,
      "manufacturer": "Garmin",
      "modelId": "GPS 17x",
      "productCode": 1234,
      "serialNumber": 123456789,
      "softwareVersion": "2.30",
      "deviceInstance": 0,
      "deviceClass": 25,
      "deviceFunction": 130
    }
  }
}
```

### Graceful Degradation
- If a device hasn't announced itself yet, show "Device info: Discovering..." placeholder
- If a device never announces (non-compliant), show "Unknown Device" with just the SID
- NMEA0183 sources (which have no discovery protocol) show "NMEA0183 Device" with static talker ID descriptions (e.g., "AP = Autopilot", "VH = VHF Radio")

## Integration Points

### Existing Components to Modify
- `SourceRegistry` - Add device info storage to MessageSource struct
- `SourceStatsSerializer` - Include device info in JSON serialization
- `SourceStatsHandler` - Send device info in WebSocket messages
- `data/sources.html` - Display device info in WebUI
- `nodejs-boatdata-viewer` - Relay device info through proxy (optional)

### New Components to Create
- `DeviceInfoCollector` - Polls `tN2kDeviceList` and enriches SourceRegistry
- `ManufacturerLookup` - Maps manufacturer codes to human-readable names (if not provided by library)

### Initialization Sequence
In `main.cpp`:
1. Initialize NMEA2000 (already done)
2. Create `tN2kDeviceList` instance attached to NMEA2000
3. Initialize SourceRegistry (already done)
4. Create `DeviceInfoCollector` component
5. Register ReactESP timer for periodic device list polling (every 5 seconds)

## Success Criteria

### Must Have
- System discovers NMEA2000 devices and extracts manufacturer, model, serial number, and software version
- Source statistics WebSocket messages include device information when available
- WebUI dashboard displays device information for each NMEA2000 source
- Device discovery runs continuously without impacting message processing performance
- Unknown/undiscovered devices show appropriate placeholder text

### Nice to Have
- Manufacturer code lookup table for human-readable names (if NMEA2000 library doesn't provide)
- Device instance correlation for multi-instance devices (e.g., "Port Engine" vs "Starboard Engine")
- PGN capability display (which PGNs this device can transmit/receive)
- Static NMEA0183 talker ID descriptions (e.g., "AP → Autopilot")

### Out of Scope
- Modifying device configurations via NMEA2000 (read-only discovery only)
- Storing device information in persistent storage (RAM only, rediscover on reboot)
- Supporting NMEA2000 firmware updates

## Constraints

- Memory usage must stay within ESP32 limits (target <5KB additional RAM for 20 devices)
- Device discovery polling must not block NMEA message processing
- Must follow project constitution principles (HAL abstraction, static allocation where possible)
- Must maintain compatibility with existing R012 source statistics system
- Must use NMEA2000 library's built-in `tN2kDeviceList` (no custom device list parser)

## Dependencies

- R012 (Source Statistics and WebUI) - Must be implemented first
- NMEA2000 library v4.17.2+ with `N2kDeviceList.h` support
- Existing NMEA2000 integration (R008)
- ReactESP event loop for periodic polling

## Testing Approach

- Unit tests: Mock `tN2kDeviceList` interface, verify device info extraction
- Integration tests: Connect real NMEA2000 devices (GPS, compass, engine), verify discovery
- Hardware tests: Multi-device bus, hot-plug scenarios, SID collision handling
- Manual testing: Verify WebUI displays device info correctly, test with various manufacturers

## Related Requirements

- R012: Source Statistics and WebUI (foundation for this feature)
- R008: NMEA 2000 Integration (provides NMEA2000 library integration)
- R009: WebUI for BoatData Streaming (establishes WebSocket patterns)

---

**Priority**: Medium (enhances existing system, not critical for core functionality)
**Estimated Complexity**: Medium (well-defined API, clear integration points)
**Target Release**: Next feature cycle after R012 completion

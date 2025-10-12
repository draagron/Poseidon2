# Research: NMEA 0183 Data Handlers

**Feature**: 006-nmea-0183-handlers
**Date**: 2025-10-11
**Status**: Complete

## Executive Summary

This feature implements NMEA 0183 sentence parsers for 5 message types (RSA, HDM, GGA, RMC, VTG) from 2 talker IDs (AP=autopilot, VH=VHF radio). The parsers extract marine data (GPS position, heading, rudder angle, course, speed, variation), convert units to BoatData format (degrees→radians, m/s→knots), and update the centralized BoatData repository using the existing ISensorUpdate interface. Multi-source prioritization is automatic via BoatData's SourcePrioritizer (highest frequency source wins). Custom RSA parser required (not in NMEA0183 library). Graceful error handling with silent discard for malformed/out-of-range sentences.

## Technical Decisions

### 1. NMEA 0183 Library Usage

**Decision**: Use ttlappalainen/NMEA0183 library for HDM, GGA, RMC, VTG; implement custom RSA parser

**Rationale**:
- NMEA0183 library already approved in constitution (v1.2.0, line 172)
- Library provides sentence parsing, checksum validation, field extraction for standard sentences
- RSA (Rudder Sensor Angle) is proprietary/non-standard, not in library
- Reference implementation (examples/poseidongw/src/NMEA0183Handlers.cpp:78-86) shows custom RSA parser pattern

**Alternatives Considered**:
- **Write all parsers from scratch**: Rejected - reinvents wheel, increases code size, higher bug risk
- **Use different NMEA library**: Rejected - ttlappalainen library is constitutional standard, well-tested for ESP32

**Implementation Approach**:
- Use `tNMEA0183` class for sentence-level parsing (checksum, field extraction)
- Use library parsers: `NMEA0183ParseRMC()`, `NMEA0183ParseGGA()`, `NMEA0183ParseVTG()`, `NMEA0183ParseHDM()`
- Implement custom `NMEA0183ParseRSA()` following library patterns (return bool, extract fields via `Field()` method)
- Filter by talker ID in handler functions: `strcmp(msg.Sender(), "AP") == 0` or `strcmp(msg.Sender(), "VH") == 0`

**Reference**:
- Library: https://github.com/ttlappalainen/NMEA0183
- Example RSA parser: examples/poseidongw/src/NMEA0183Handlers.cpp:78-86
- Library API: tNMEA0183Msg class provides `Sender()`, `MessageCode()`, `Field(index)` methods

---

### 2. BoatData Integration Pattern

**Decision**: Use ISensorUpdate interface with source identifiers "NMEA0183-AP" and "NMEA0183-VH"

**Rationale**:
- BoatData feature (R005) already implemented with ISensorUpdate interface (src/components/BoatData.h:88-95)
- Interface provides validation, multi-source prioritization, diagnostic tracking
- Source IDs distinguish autopilot vs VHF data for prioritization (even within NMEA 0183 sources)
- Automatic frequency-based priority: NMEA 2000 (10Hz) naturally wins over NMEA 0183 (1Hz)

**Alternatives Considered**:
- **Direct BoatData field writes**: Rejected - bypasses validation, no source tracking, violates HAL abstraction
- **Single "NMEA0183" source ID**: Rejected - loses ability to prioritize between AP and VH sources

**Implementation Approach**:
```cpp
// In HandleRSA (autopilot)
bool accepted = boatData->updateRudder(angleDegrees * DEG_TO_RAD, "NMEA0183-AP");

// In HandleGGA (VHF)
bool accepted = boatData->updateGPS(lat, lon, cog, sog, "NMEA0183-VH");
```

**Return Value Handling**:
- `true`: Data accepted, BoatData updated
- `false`: Data rejected (outlier, out-of-range, or lower priority source active)
- Per FR-024/FR-025/FR-026: Silent discard on rejection (no logging)

**Reference**:
- Interface definition: src/hal/interfaces/ISensorUpdate.h
- Implementation: src/components/BoatData.cpp (updateGPS, updateCompass, updateWind, updateSpeed, updateRudder methods)

---

### 3. Unit Conversion Strategy

**Decision**: Convert all units at handler level before calling ISensorUpdate methods

**Rationale**:
- BoatData stores all angles in radians, speeds in knots (src/types/BoatDataTypes.h:8-12)
- NMEA 0183 sentences provide data in degrees, m/s (per NMEA 0183 spec)
- Handler functions are single responsibility: parse sentence → convert units → update BoatData
- Conversion constants already defined in Arduino: `DEG_TO_RAD`, `RAD_TO_DEG`

**Alternatives Considered**:
- **Convert in BoatData**: Rejected - BoatData shouldn't know about NMEA 0183 units, violates abstraction
- **Store both units**: Rejected - doubles memory footprint, adds complexity

**Conversion Table**:

| Field | NMEA 0183 Unit | BoatData Unit | Conversion |
|-------|----------------|---------------|------------|
| Latitude | Degrees (DDMM.MMMM) | Decimal degrees | Parse DDMM → DD.dddd |
| Longitude | Degrees (DDDMM.MMMM) | Decimal degrees | Parse DDDMM → DDD.dddd |
| Heading (HDM) | Degrees | Radians | `deg * DEG_TO_RAD` |
| COG (RMC/VTG) | Degrees | Radians | `deg * DEG_TO_RAD` |
| SOG (RMC/VTG) | Knots (library) | Knots | **No conversion needed** |
| Variation | Degrees | Radians | `deg * DEG_TO_RAD` |
| Rudder angle (RSA) | Degrees | Radians | `deg * DEG_TO_RAD` |

**Correction to FR-009**: Research shows NMEA0183 library already returns SOG in knots (not m/s). No conversion needed.

**Implementation Approach**:
```cpp
#define DEG_TO_RAD (M_PI / 180.0)

void HandleHDM(const tNMEA0183Msg &msg) {
    double headingDegrees;
    if (NMEA0183ParseHDM(msg, headingDegrees)) {
        double headingRadians = headingDegrees * DEG_TO_RAD;
        boatData->updateCompass(0.0, headingRadians, 0.0, "NMEA0183-AP");
    }
}
```

**Reference**:
- NMEA0183 library source: Returns angles in degrees, speeds in knots
- Arduino constants: DEG_TO_RAD, RAD_TO_DEG defined in Arduino.h

---

### 4. Serial Port Abstraction (HAL)

**Decision**: Create ISerialPort interface for Serial2 access with ESP32SerialAdapter and MockSerialPort

**Rationale**:
- Constitution Principle I requires HAL for all hardware (line 68-72)
- Serial2 (NMEA 0183) is hardware dependency
- Mock-first testing requires mockable serial input
- Reference implementation uses direct Serial2 access (needs HAL upgrade)

**Alternatives Considered**:
- **Direct Serial2 access**: Rejected - violates HAL principle, untestable on native platform
- **Use Stream abstraction**: Rejected - Stream is Arduino-specific, still can't mock on native

**Interface Design**:
```cpp
// ISerialPort.h
class ISerialPort {
public:
    virtual int available() = 0;
    virtual int read() = 0;
    virtual void begin(unsigned long baud) = 0;
    virtual ~ISerialPort() = default;
};

// ESP32SerialAdapter.h (hal/implementations/)
class ESP32SerialPort : public ISerialPort {
private:
    HardwareSerial* serial;
public:
    ESP32SerialPort(HardwareSerial* s) : serial(s) {}
    int available() override { return serial->available(); }
    int read() override { return serial->read(); }
    void begin(unsigned long baud) override { serial->begin(baud); }
};

// MockSerialPort.h (mocks/)
class MockSerialPort : public ISerialPort {
private:
    const char* mockData;
    size_t position;
public:
    void setMockData(const char* data);
    int available() override;
    int read() override;
    void begin(unsigned long baud) override {}
};
```

**Reference**:
- Constitution HAL principle: .specify/memory/constitution.md:68-72
- Existing HAL examples: src/hal/interfaces/IDisplayAdapter.h, src/hal/interfaces/IWiFiAdapter.h

---

### 5. ReactESP Integration Pattern

**Decision**: Use ReactESP `app.onRepeat()` for periodic sentence polling (10ms interval)

**Rationale**:
- Reference implementation uses 10ms polling (examples/poseidongw/src/main.cpp)
- Non-blocking operation required (Constitution Principle VI: Always-On)
- 10ms interval supports up to 100 sentences/second (far exceeds 1Hz NMEA 0183 typical rate)
- Matches existing pattern for NMEA 2000 parsing

**Alternatives Considered**:
- **Interrupt-driven Serial**: Rejected - adds complexity, 10ms polling sufficient for 4800 baud
- **Dedicated RTOS task**: Rejected - ReactESP event loop simpler, lower memory overhead

**Implementation Approach**:
```cpp
// In main.cpp setup()
tNMEA0183* nmea0183 = new tNMEA0183();
ISerialPort* serial0183 = new ESP32SerialPort(&Serial2);
NMEA0183Handler* handler = new NMEA0183Handler(nmea0183, serial0183, boatData);

serial0183->begin(4800);
nmea0183->SetMessageStream(serial0183); // Adapter wraps ISerialPort as Stream

// ReactESP periodic task
app.onRepeat(10, []() {
    nmea0183->ParseMessages(); // Reads from stream, calls handler
});
```

**Performance Validation**:
- 4800 baud = 600 bytes/second max
- NMEA sentence max length: 82 bytes
- Max sentence rate: ~7 sentences/second
- 10ms polling: checks 100 times/second → latency <10ms per sentence
- Processing budget: 50ms per sentence (FR-027)

**Reference**:
- ReactESP library: https://github.com/mairas/ReactESP
- Reference implementation: examples/poseidongw/src/main.cpp (periodic message parsing)

---

### 6. Error Handling Strategy

**Decision**: Silent discard with no logging for malformed/invalid sentences (per FR-024, FR-025)

**Rationale**:
- Clarification session resolved error handling: "Silently discard sentence (no logging, no counter)"
- NMEA 0183 environments can be noisy (electrical interference, multi-device bus)
- Logging every bad checksum creates log spam
- Valid sentences are logged at DEBUG level for confirmation

**Alternatives Considered**:
- **Log all errors**: Rejected - user explicitly rejected this approach in clarifications
- **Error counters**: Rejected - adds memory footprint, user specified "no counter"

**Implementation Approach**:
```cpp
void HandleGGA(const tNMEA0183Msg &msg) {
    // Talker ID filter
    if (strcmp(msg.Sender(), "VH") != 0) {
        return; // Silent discard - wrong talker ID
    }

    double lat, lon;
    if (NMEA0183ParseGGA(msg, lat, lon, ...)) {
        // Validate range
        if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) {
            return; // Silent discard - out of range (FR-026)
        }

        // Convert and update
        bool accepted = boatData->updateGPS(lat, lon, ...);
        // Note: Don't log if rejected - ISensorUpdate handles priority internally
    }
    // Malformed sentence returns false from parser - silent discard
}
```

**Validation Ranges** (FR-026):
- Latitude: [-90°, 90°]
- Longitude: [-180°, 180°]
- Rudder angle: [-90°, 90°]
- Variation: [-30°, 30°]
- Speed: [0, 100 knots]
- Heading: [0°, 360°]

**Reference**:
- Clarification Q1: spec.md:45 (silent discard for checksum failures)
- Clarification Q2: spec.md:46 (reject sentence for out-of-range)
- FR-024, FR-025, FR-026: spec.md:168-171

---

### 7. Variation Calculation from VTG

**Decision**: Calculate variation as `trueCOG - magneticCOG`, validate result in [-30°, 30°]

**Rationale**:
- VTG sentence provides both true and magnetic COG (Track Made Good)
- Magnetic variation = true heading - magnetic heading (nautical standard formula)
- Typical variation ranges: -30° (30°W) to +30° (30°E)
- Values outside this range indicate sentence error or extreme location (rare)

**Alternatives Considered**:
- **Accept any variation**: Rejected - >30° likely indicates sentence corruption
- **Stricter range (±20°)**: Rejected - legitimate variation can reach ±25° in some polar regions

**Implementation Approach**:
```cpp
void HandleVTG(const tNMEA0183Msg &msg) {
    double trueCOG, magCOG, sog;
    if (NMEA0183ParseVTG(msg, trueCOG, magCOG, sog)) {
        // Calculate variation
        double variation = trueCOG - magCOG;

        // Normalize to [-180, 180]
        while (variation > 180.0) variation -= 360.0;
        while (variation < -180.0) variation += 360.0;

        // Validate range
        if (fabs(variation) > 30.0) {
            return; // Silent discard - invalid variation
        }

        double variationRadians = variation * DEG_TO_RAD;
        double cogRadians = trueCOG * DEG_TO_RAD;
        double sogKnots = sog;

        boatData->updateGPS(0.0, 0.0, cogRadians, sogKnots, "NMEA0183-VH");
        boatData->updateCompass(0.0, 0.0, variationRadians, "NMEA0183-VH");
    }
}
```

**Reference**:
- FR-011: spec.md:146 (calculate variation from VTG)
- FR-026: spec.md:170 (variation range: -30° to +30°)
- NMEA VTG format: True COG (field 1), Magnetic COG (field 3), SOG (field 5/7)

---

### 8. Testing Strategy

**Decision**: 4-tier grouped test structure (contracts, integration, units, hardware)

**Rationale**:
- PlatformIO grouped test organization (test_[feature]_[type]/)
- Constitution Principle III: Mock-first, hardware tests minimal
- Native platform (x86/ARM) for 95% of tests, ESP32 only for Serial2 timing
- Matches existing test patterns (test_boatdata_*, test_wifi_*)

**Test Organization**:
```
test/
├── test_nmea0183_contracts/        # HAL interface validation
│   ├── test_main.cpp
│   ├── test_iserialport.cpp        # ISerialPort contract
│   └── test_parser_interface.cpp   # Parser function signatures
│
├── test_nmea0183_integration/      # End-to-end scenarios (mocked Serial)
│   ├── test_main.cpp
│   ├── test_rsa_to_boatdata.cpp    # RSA → rudder angle
│   ├── test_hdm_to_boatdata.cpp    # HDM → magnetic heading
│   ├── test_gga_to_boatdata.cpp    # GGA → GPS position
│   ├── test_rmc_to_boatdata.cpp    # RMC → GPS + variation
│   ├── test_vtg_to_boatdata.cpp    # VTG → COG/SOG + variation
│   ├── test_talker_id_filter.cpp   # Ignore non-AP/VH
│   ├── test_multi_source.cpp       # Priority: N2K wins over 0183
│   └── test_invalid_sentences.cpp  # Malformed/out-of-range rejection
│
├── test_nmea0183_units/            # Formula/utility validation
│   ├── test_main.cpp
│   ├── test_unit_conversions.cpp   # Degrees↔radians, DDMM→decimal
│   ├── test_variation_calc.cpp     # VTG variation formula
│   ├── test_checksum_validation.cpp # Sentence checksum
│   └── test_field_parsing.cpp      # Extract fields from sentence
│
└── test_nmea0183_hardware/         # ESP32 only
    └── test_main.cpp
        ├── Serial2 4800 baud timing
        ├── 50ms processing budget validation
        └── Buffer overflow handling
```

**Test Execution**:
```bash
# All NMEA 0183 tests (native)
pio test -e native -f test_nmea0183_*

# Specific test group
pio test -e native -f test_nmea0183_integration

# Hardware tests (ESP32)
pio test -e esp32dev_test -f test_nmea0183_hardware
```

**Reference**:
- Test organization: CLAUDE.md:121-140 (test structure pattern)
- PlatformIO grouped tests: https://docs.platformio.org/en/latest/advanced/unit-testing/structure/hierarchy.html#grouped-tests

---

## Open Questions / Risks

### Risk 1: Custom RSA Parser Correctness

**Risk**: RSA sentence format may vary by autopilot manufacturer (no standard)

**Mitigation**:
- Reference implementation parser validated on real hardware (examples/poseidongw/src/NMEA0183Handlers.cpp:78-86)
- RSA format: `$APRSA,starboard_angle,A,port_angle,A*checksum`
- Validate with AP autopilot hardware during integration testing
- Document parser assumptions in code comments

**Flagged for**: Human review (Principle III checklist item)

### Risk 2: NMEA 0183 Library Stream Compatibility

**Risk**: NMEA0183 library expects Arduino Stream, ISerialPort is custom interface

**Mitigation**:
- Create StreamAdapter wrapper: ISerialPort → Stream
- Adapter implements Stream::available() and Stream::read() by delegating to ISerialPort
- Already tested pattern in WiFiManager (wraps IFileSystem for LittleFS)

**Implementation**:
```cpp
class SerialPortStreamAdapter : public Stream {
private:
    ISerialPort* port;
public:
    SerialPortStreamAdapter(ISerialPort* p) : port(p) {}
    int available() override { return port->available(); }
    int read() override { return port->read(); }
    size_t write(uint8_t c) override { return 0; } // Not needed for NMEA RX
    int peek() override { return -1; } // Not needed
};
```

---

## Dependency Analysis

### External Libraries
- **NMEA0183** (ttlappalainen/NMEA0183): Already in platformio.ini, approved in constitution
- **ReactESP**: Already in use for event loops
- **Arduino Core ESP32**: Platform requirement

### Internal Dependencies
- **BoatData** (R005): ISensorUpdate interface, source prioritization, validation
- **WebSocketLogger** (existing): Network debugging (Principle V)
- **Serial2** (hardware): UART2 on ESP32 (RX=GPIO25, TX=GPIO27)

### Initialization Order (main.cpp)
1. WiFi connection
2. BoatData initialization
3. Serial2.begin(4800) via ESP32SerialPort
4. NMEA0183Handler initialization
5. ReactESP event loop registration
6. NMEA2000 initialization (separate feature)

**Critical**: Serial2 must init AFTER WiFi but BEFORE ReactESP loops (per constitution sequence)

---

## Memory Impact Estimate

**Static Allocation**:
- Sentence buffer: 82 bytes (NMEA max length)
- Handler state: ~50 bytes (message handlers, flags)
- ISerialPort adapter: 8 bytes (pointer wrapper)
- **Total RAM**: ~140 bytes (0.04% of 320KB ESP32 RAM)

**Flash Impact**:
- Parser functions: ~8KB (5 handlers + custom RSA)
- Unit conversion utilities: ~2KB
- HAL interfaces: ~1KB
- Test code (not in production): ~15KB
- **Total Flash**: ~15KB production (~0.8% of 1.9MB partition)

**Actual Measurement** (Phase 3.5 - T041):
- RAM usage: 44,948 bytes total (13.7% of 327,680 bytes)
- Flash usage: 957,769 bytes total (48.7% of 1,966,080 bytes)
- **Baseline comparison**: Estimated impact ~140 bytes RAM, ~15KB flash
- **Constitutional Compliance**: ✓ Well under resource limits (Principle II)
- **No heap allocations**: All memory statically allocated per constitutional requirements

---

## Performance Budget

**Per-Sentence Processing**:
- Serial read: <1ms (interrupt-driven)
- Checksum validation: <1ms
- Field parsing: <5ms
- Unit conversion: <1ms
- BoatData update: <5ms (includes validation)
- **Total**: <15ms per sentence

**Budget**: 50ms per sentence (FR-027)
**Margin**: 35ms (70% headroom)

**Worst Case**:
- 7 sentences/second at 4800 baud
- 15ms × 7 = 105ms/second
- Leaves 895ms/second for other tasks (89% CPU idle)

**Constitutional Compliance**: ✓ Non-blocking operation (Principle VI)

---

## Next Steps

1. **Phase 1**: Design data model (contracts, HAL interfaces)
2. **Phase 2**: Generate tasks.md (TDD order: tests first)
3. **Phase 3**: Implement following task order
4. **Phase 4**: Hardware validation on ESP32 with Serial2
5. **QA Review**: Custom RSA parser requires human review (flagged)

---

**Research Complete**: All NEEDS CLARIFICATION resolved, ready for Phase 1 design.

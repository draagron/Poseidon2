# Contract: ManufacturerLookup

**Component**: `src/utils/ManufacturerLookup.h` / `.cpp`
**Purpose**: Static lookup table mapping NMEA2000 manufacturer codes to human-readable names
**Type**: Stateless utility (pure functions, PROGMEM data)

## Responsibilities

1. Map NMEA2000 manufacturer codes (uint16_t) to manufacturer names (const char*)
2. Provide "Unknown (code)" fallback for unrecognized codes
3. Store lookup table in PROGMEM (flash memory) to conserve RAM

## Public Interface

```cpp
/**
 * @brief Get manufacturer name from NMEA2000 code
 *
 * @param code NMEA2000 manufacturer code (e.g., 275, 1855)
 * @return Manufacturer name (e.g., "Garmin", "Furuno") or "Unknown (275)" for unrecognized codes
 *
 * @note Returns static string (no allocation), valid for program lifetime
 * @note Thread-safe (read-only data)
 */
const char* getManufacturerName(uint16_t code);
```

## Behavior Contracts

### BC-1: Known Manufacturer Codes
**Requirement**: Lookup table includes ≥40 common marine manufacturers

**Test Data** (minimum required entries):
| Code | Manufacturer | Reference |
|------|--------------|-----------|
| 137 | Airmar | Marine electronics |
| 135 | B&G | Navigation systems |
| 275 | Garmin | GPS, chartplotters |
| 1855 | Furuno | Marine electronics |
| 1857 | Navico (Simrad/Lowrance/B&G) | Navigation |
| 378 | Raymarine | Marine electronics |
| 307 | Yanmar | Engines |
| 163 | Maretron | Instrumentation |
| 529 | Victron Energy | Power systems |
| 304 | Mercury Marine | Engines |

**Test**:
```cpp
TEST_CASE("getManufacturerName returns correct names for known codes") {
    REQUIRE(strcmp(getManufacturerName(275), "Garmin") == 0);
    REQUIRE(strcmp(getManufacturerName(1855), "Furuno") == 0);
    // ... test all 40+ entries
}
```

### BC-2: Unknown Manufacturer Codes
**Requirement**: Unrecognized codes return "Unknown (code)" format

**Test**:
```cpp
TEST_CASE("getManufacturerName returns Unknown fallback for unrecognized codes") {
    REQUIRE(strcmp(getManufacturerName(99999), "Unknown (99999)") == 0);
    REQUIRE(strcmp(getManufacturerName(0), "Unknown (0)") == 0);
}
```

### BC-3: Performance
**Requirement**: Lookup completes in <50μs (linear search acceptable for ~50 entries)

**Test**:
```cpp
TEST_CASE("getManufacturerName completes in <50 microseconds") {
    unsigned long start = micros();
    getManufacturerName(275);
    unsigned long duration = micros() - start;
    REQUIRE(duration < 50);
}
```

### BC-4: Memory Usage
**Requirement**: Lookup table stored in PROGMEM (flash), not RAM

**Implementation**:
```cpp
const ManufacturerEntry PROGMEM manufacturerTable[] = {
    {275, "Garmin"},
    {1855, "Furuno"},
    // ...
};
```

**Test**: Verify via platform-specific memory report (not unit testable on native)

## Error Handling

**No Error Conditions**: Function is total (defined for all uint16_t inputs), always returns valid string.

## Data Source

**Reference**: NMEA2000 manufacturer codes maintained by NMEA organization:
- Official: https://www.nmea.org/nmea-2000.html (requires membership)
- Community: https://github.com/canboat/canboat/blob/master/analyzer/pgns.json (open-source reference)

**Maintenance**: Update table when new marine electronics manufacturers emerge (infrequent, ~yearly).

---

**Contract Version**: 1.0
**Last Updated**: 2025-10-13

# Implementation Plan: NMEA2000 Device Discovery and Identification

**Branch**: `013-r013-nmea2000-device` | **Date**: 2025-10-13 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/home/niels/Dev/Poseidon2/specs/013-r013-nmea2000-device/spec.md`

**Note**: This template is filled in by the `/plan` command. See `.claude/commands/plan.md` for the execution workflow.

## Summary

Implement automatic discovery and metadata extraction for NMEA2000 devices on the CAN bus using the NMEA2000 library's `tN2kDeviceList` class. The system will poll for device announcements every 5 seconds, extract manufacturer, model, serial number, and software version information, and enrich existing `MessageSource` entries in the `SourceRegistry`. Device metadata will be exposed via WebSocket API (schema v2) and displayed in the web dashboard. NMEA0183 sources will use static talker ID lookups for device type descriptions.

## Technical Context

**Language/Version**: C++ (C++11 minimum, C++14 preferred) with Arduino framework
**Primary Dependencies**:
- NMEA2000 library (https://github.com/ttlappalainen/NMEA2000) - Device discovery via `tN2kDeviceList`
- NMEA2000_esp32 (https://github.com/ttlappalainen/NMEA2000_esp32) - ESP32 CAN bus implementation
- NMEA0183 library (https://github.com/ttlappalainen/NMEA0183) - Serial sentence parsing
- ReactESP (https://github.com/mairas/ReactESP) - Non-blocking event loops for periodic polling
- ESPAsyncWebServer (https://github.com/ESP32Async/ESPAsyncWebServer) - WebSocket streaming

**Storage**: LittleFS for web UI assets (stream.html, sources.html); No persistent device metadata storage (rediscovered on reboot)
**Testing**: PlatformIO native environment for unit/integration/contract tests; ESP32 environment for hardware tests
**Target Platform**: ESP32 family (ESP32, ESP32-S2, ESP32-C3, ESP32-S3) running on SH-ESP32 hardware (24/7 always-on marine gateway)

**Project Type**: Embedded single-application (ESP32 firmware) with web frontend served from LittleFS

**Performance Goals**:
- Device discovery poll cycle: <10ms per 5-second interval (20 devices on bus)
- WebSocket message latency: <100ms from device discovery to client notification
- Zero impact on NMEA message processing throughput (10 Hz GPS, 1 Hz compass, etc.)

**Constraints**:
- Memory: <5KB additional RAM for device metadata (50 sources × ~68 bytes per DeviceInfo struct)
- Network: Always-on WiFi requirement (no sleep modes)
- Discovery timeout: 60 seconds for non-compliant devices (mark as "Unknown")
- Non-blocking: All operations must use ReactESP timers (no blocking delays)
- Static allocation: Prefer fixed-size buffers over dynamic memory to avoid heap fragmentation

**Scale/Scope**:
- Max 50 total NMEA sources tracked (NMEA2000 + NMEA0183)
- Max 20 NMEA2000 devices on bus (typical marine installation has 5-10)
- 9 BoatData categories, 19 message types (PGNs + sentence IDs)
- 4 user stories (P1-P4 priority), 22 functional requirements
- Estimated implementation: ~1500 LOC (DeviceInfoCollector, lookups, UI updates, tests)

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Constitutional Alignment

✅ **I. Hardware Abstraction Layer (HAL)**: COMPLIANT
- Device discovery logic uses NMEA2000 library's abstract `tN2kDeviceList` interface (no direct CAN register access)
- No new HAL interfaces required (device discovery operates through existing NMEA2000 library abstraction)
- `DeviceInfoCollector` component interfaces with library API, not raw hardware

✅ **II. Resource-Aware Development**: COMPLIANT
- Static memory allocation: `DeviceInfo` struct embedded in `MessageSource` (~68 bytes × 50 sources = 3.4KB)
- Fixed-size string buffers: manufacturer (16 bytes), modelId (24 bytes), softwareVersion (12 bytes)
- No dynamic heap allocations for device metadata storage
- Memory footprint documented: <5KB additional RAM total

✅ **III. QA-First Review Process**: COMPLIANT
- All code changes will undergo QA subagent review before merge
- Critical path: Device discovery polling must be validated for timing (<10ms) and memory safety (buffer bounds)

✅ **IV. Modular Component Design**: COMPLIANT
- `DeviceInfoCollector`: Single responsibility (poll device list, extract metadata, update SourceRegistry)
- `ManufacturerLookup`: Static utility for manufacturer code→name mapping
- `TalkerIdLookup`: Static utility for NMEA0183 talker ID→description mapping
- Dependency injection: `DeviceInfoCollector` receives `tN2kDeviceList*`, `SourceRegistry*`, `WebSocketLogger*` as constructor parameters

✅ **V. Network-Based Debugging**: COMPLIANT
- WebSocket logging at DEBUG level for all device discovery events (device discovered, metadata updated, timeouts)
- JSON-formatted log messages with structured device metadata
- No reliance on Serial output (Serial2 reserved for NMEA0183 device communication)

✅ **VI. Always-On Operation**: COMPLIANT
- ReactESP timer-based polling (every 5 seconds) - non-blocking
- No sleep modes or power management changes
- Designed for 24/7 continuous device discovery operation

✅ **VII. Fail-Safe Operation**: COMPLIANT
- Graceful degradation: Non-compliant devices marked "Unknown" after 60s timeout (system continues operating)
- Display initialization failure (R004 OLED feature) already handles graceful degradation; device discovery inherits this robustness
- No critical system dependencies on device metadata (statistics tracking continues regardless of discovery status)

✅ **VIII. Workflow Selection**: COMPLIANT
- Using Feature Development workflow (`/specify` → `/clarify` → `/plan` → `/tasks` → `/implement`)
- Full specification with TDD approach (contract tests for HAL-like interfaces, integration tests for end-to-end scenarios)

### Gate Status: ✅ PASSED

All constitutional principles satisfied. No complexity violations requiring justification. Proceeding to Phase 0 research.

## Project Structure

### Documentation (this feature)

```
specs/013-r013-nmea2000-device/
├── plan.md              # This file (/plan command output)
├── research.md          # Phase 0 output - NMEA2000 library API analysis
├── data-model.md        # Phase 1 output - DeviceInfo structures and schemas
├── quickstart.md        # Phase 1 output - Hardware validation guide
├── contracts/           # Phase 1 output - Component interface contracts
│   ├── DeviceInfoCollectorContract.md
│   ├── ManufacturerLookupContract.md
│   └── TalkerIdLookupContract.md
└── tasks.md             # Phase 2 output (/tasks command - NOT created by /plan)
```

### Source Code (repository root)

**Structure**: Embedded single-application (ESP32 firmware) with C++ components and web assets

```
src/
├── components/          # Feature components (modular, single-responsibility)
│   ├── DeviceInfoCollector.h         # NEW: NMEA2000 device discovery polling
│   ├── DeviceInfoCollector.cpp
│   ├── SourceRegistry.h               # MODIFIED: Add DeviceInfo to MessageSource
│   ├── SourceRegistry.cpp
│   ├── SourceStatsSerializer.h        # MODIFIED: Include deviceInfo in JSON (v2 schema)
│   ├── SourceStatsSerializer.cpp
│   └── SourceStatsHandler.cpp         # MODIFIED: Handle discovery timeout events
│
├── utils/               # Utility functions and helpers
│   ├── ManufacturerLookup.h           # NEW: Static manufacturer code→name mapping
│   ├── ManufacturerLookup.cpp
│   ├── TalkerIdLookup.h               # NEW: Static NMEA0183 talker ID→description
│   └── TalkerIdLookup.cpp
│
├── types/               # Data structures
│   └── SourceStatistics.h             # MODIFIED: Add DeviceInfo struct to MessageSource
│
└── main.cpp             # MODIFIED: Initialize tN2kDeviceList, DeviceInfoCollector, ReactESP timer

data/                    # LittleFS web assets
└── sources.html         # MODIFIED: Add device metadata display (expandable sections)

test/                    # PlatformIO test groups
├── helpers/             # Shared test utilities (not a test itself)
├── test_device_discovery_contracts/      # NEW: HAL-like interface validation
├── test_device_discovery_integration/    # NEW: End-to-end device discovery scenarios
├── test_device_discovery_units/          # NEW: Utility functions, lookups, formatters
└── test_source_stats_integration/        # MODIFIED: Update for v2 schema with deviceInfo
```

**Structure Decision**: Single embedded application with modular C++ components. New components follow existing pattern (components/ for business logic, utils/ for stateless helpers, types/ for shared data structures). Web UI modifications contained in data/ LittleFS partition. Tests follow PlatformIO grouped test pattern (`test_<feature>_<type>/`) with native environment for non-hardware tests.

## Complexity Tracking

*Fill ONLY if Constitution Check has violations that must be justified*

No complexity violations detected. All design decisions align with constitutional principles.

## Progress Tracking

- [x] **Technical Context** - Filled with C++/Arduino/ESP32 platform details
- [x] **Constitution Check** - All 8 principles validated, PASSED
- [x] **Project Structure** - Documented with actual repository paths
- [x] **Phase 0: Research** - NMEA2000 library API analysis completed (research.md)
- [x] **Phase 1: Design** - Data model, contracts, quickstart guide completed
  - [x] data-model.md - DeviceInfo struct, WebSocket schema v2, lookup tables
  - [x] contracts/DeviceInfoCollectorContract.md - Component interface and behavior contracts
  - [x] contracts/ManufacturerLookupContract.md - Static lookup utility contract
  - [x] contracts/TalkerIdLookupContract.md - NMEA0183 talker ID lookup contract
  - [x] quickstart.md - Hardware validation guide (7 test scenarios)
- [x] **Phase 2: Tasks** - Dependency-ordered task breakdown completed (tasks.md)
  - 29 tasks total across 7 phases
  - 4 user stories (P1-P4 priority)
  - TDD approach (tests before implementation)
  - Parallel execution opportunities identified

## Next Steps

1. ✅ ~~Run `/tasks` command~~ - **COMPLETE**
2. **Run `/implement` command** to execute tasks.md with TDD approach (29 tasks)
3. **QA Review** - All code changes reviewed by QA subagent before merge
4. **Hardware Validation** - Follow quickstart.md test suite with real NMEA2000 devices

## Artifacts Generated

**Planning Phase** (`/plan` command):
- ✅ `specs/013-r013-nmea2000-device/plan.md` (this file)
- ✅ `specs/013-r013-nmea2000-device/research.md` (NMEA2000 library API analysis)
- ✅ `specs/013-r013-nmea2000-device/data-model.md` (DeviceInfo structures, schemas)
- ✅ `specs/013-r013-nmea2000-device/contracts/` (3 component contracts)
- ✅ `specs/013-r013-nmea2000-device/quickstart.md` (Hardware validation guide)

**Tasks Phase** (`/tasks` command):
- ✅ `specs/013-r013-nmea2000-device/tasks.md` (29 tasks across 7 phases, TDD approach)

**Implementation Phase** (`/implement` command - NOT YET RUN):
- ⏳ Source code changes per tasks.md
- ⏳ Test implementation (contracts, integration, units)
- ⏳ QA subagent review

---

**Planning Phase Status**: ✅ COMPLETE
**Tasks Phase Status**: ✅ COMPLETE
**Ready for**: `/implement` command to execute tasks.md (29 tasks, TDD approach)

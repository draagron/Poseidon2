# Feature Specification: NMEA2000 Device Discovery and Identification

**Feature Branch**: `013-r013-nmea2000-device`
**Created**: 2025-10-13
**Status**: Draft
**Input**: User description: "R013 - nmea2000 device discovery"

## Clarifications

### Session 2025-10-13

- Q: Should the system trigger an immediate device list query when a new source is first detected, or wait for the next scheduled 5-second poll? → A: Wait for scheduled 5-second poll (simpler, consistent timing)
- Q: What maximum string lengths should be enforced for device metadata fields to ensure predictable memory usage? → A: manufacturer: 16 bytes, modelId: 24 bytes, softwareVersion: 12 bytes (minimal, tighter constraint)
- Q: Should the system send a WebSocket notification when a source transitions from "Discovering..." to "Unknown (non-compliant)" after the 60-second timeout? → A: Yes - send delta with hasInfo=false and explicit "timeout" reason field

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Automatic Device Discovery with Metadata Enrichment (Priority: P1)

As a marine system administrator, I need the system to automatically discover NMEA2000 devices as they announce themselves on the bus and extract their manufacturer, model name, serial number, and software version, so that I can identify which specific hardware is providing each data source instead of just seeing anonymous SID numbers like "NMEA2000-42".

**Why this priority**: This is the foundational capability that transforms anonymous SID identifiers into actionable device information. Without this, administrators cannot distinguish between multiple GPS units or identify which specific hardware is malfunctioning. This delivers immediate diagnostic value by answering "what device is source 42?" - the most critical question when troubleshooting marine electronics.

**Independent Test**: Can be fully tested by connecting 2-3 NMEA2000 devices (e.g., Garmin GPS, Furuno compass) to the bus, monitoring device discovery via WebSocket logging, and verifying that each device's manufacturer, model, serial number, and software version are correctly extracted and correlated with their SID. Delivers immediate value by enabling device identification without requiring any UI changes.

**Acceptance Scenarios**:

1. **Given** two NMEA2000 GPS units (Garmin GPS 17x on SID 42 and Furuno GP330B on SID 7) are connected to the bus and actively transmitting PGN 129025, **When** the devices announce themselves via NMEA2000 device list messages, **Then** the system extracts manufacturer codes (275 for Garmin, 1855 for Furuno), model IDs ("GPS 17x", "GP330B"), serial numbers, and software versions, and correlates this metadata with the existing source statistics entries "NMEA2000-42" and "NMEA2000-7"

2. **Given** a new NMEA2000 device (e.g., engine monitor on SID 15) is connected to the bus during runtime, **When** it announces itself for the first time, **Then** the system automatically discovers the device, extracts its metadata (manufacturer: Yanmar, model: "6LPA-STP2", serial: 87654321), and enriches the corresponding source entry without requiring a system restart

3. **Given** an NMEA2000 device that has been discovered and enriched with metadata, **When** the device is unplugged and then reconnected with the same SID after 30 seconds, **Then** the system re-discovers the device and updates the metadata, preserving the source's message statistics history (frequency, timestamp buffer)

4. **Given** an NMEA2000 device that never announces itself (non-compliant or silent device), **When** 60 seconds have elapsed since the first message from that SID, **Then** the source entry remains enriched with just the SID ("NMEA2000-42") and displays "Device info: Unknown (non-compliant)" as a placeholder, while still tracking message statistics normally

---

### User Story 2 - WebSocket Integration with Device Metadata (Priority: P2)

As a developer or marine system integrator, I need the existing `/source-stats` WebSocket endpoint to include device metadata in both full snapshot and delta update messages, so that I can build custom monitoring applications that display rich device information alongside source statistics without requiring separate API calls.

**Why this priority**: This builds directly on P1's device discovery foundation and integrates it into the existing data streaming infrastructure. It enables programmatic access to device metadata, which is essential for custom dashboards, third-party integrations, and automated monitoring systems. However, it depends on P1 being complete and can be developed independently once device discovery is working.

**Independent Test**: Can be tested by running a WebSocket client (e.g., Python `ws_logger.py` or browser console) that connects to `ws://<ESP32_IP>/source-stats`, capturing the initial full snapshot message, triggering source updates by having devices transmit messages, and verifying that delta messages include device metadata fields. Delivers value for programmatic access to device information.

**Acceptance Scenarios**:

1. **Given** three NMEA2000 devices are discovered with full metadata (GPS, compass, engine), **When** a WebSocket client connects to `/source-stats`, **Then** the initial full snapshot message includes device metadata for all sources in the format: `{"event": "fullSnapshot", "version": 2, "timestamp": 123456, "sources": [{"sourceId": "NMEA2000-42", "deviceInfo": {"hasInfo": true, "manufacturerCode": 275, "manufacturer": "Garmin", "modelId": "GPS 17x", ...}}]}`

2. **Given** a WebSocket client is already connected and receiving updates, **When** a source's frequency changes due to new message arrivals, **Then** the delta update message includes both the updated statistics and the device metadata: `{"event": "sourceUpdate", "sourceId": "NMEA2000-42", "changes": {"frequency": 10.2, "timeSinceLast": 98, "isStale": false, "deviceInfo": {...}}}`

3. **Given** a new NMEA2000 device is hot-plugged and discovered during an active WebSocket session, **When** the device is first discovered and metadata is extracted, **Then** connected clients receive a delta update containing the complete device metadata along with initial statistics, without requiring clients to reconnect or request a new snapshot

4. **Given** the WebSocket schema version has been incremented from v1 (no device info) to v2 (with device info), **When** clients connect, **Then** the full snapshot message includes `"version": 2` in the JSON payload, allowing clients to detect and handle the schema change appropriately

---

### User Story 3 - WebUI Dashboard Display with Device Details (Priority: P3)

As a marine system administrator viewing the source statistics dashboard in a web browser, I need each NMEA2000 source entry to display its manufacturer, model name, serial number, and software version in an expandable details section, so that I can quickly identify hardware without connecting external tools or consulting installation documentation.

**Why this priority**: This provides the user-facing visualization layer for device metadata. While valuable for usability, it's a presentation enhancement that can be developed after the underlying data collection (P1) and API integration (P2) are complete. Administrators can still access device information via WebSocket logging or custom clients while this UI is being built.

**Independent Test**: Can be tested by opening the `/sources` web page in a browser, verifying that device metadata appears in expandable sections for each NMEA2000 source, and confirming that placeholder text ("Discovering..." or "Unknown device") displays appropriately for undiscovered sources. Delivers standalone value for web-based monitoring.

**Acceptance Scenarios**:

1. **Given** multiple NMEA2000 devices are discovered and enriched with metadata, **When** I navigate to `http://<ESP32_IP>:3030/sources` and view the GPS category, **Then** I see each source entry (e.g., "NMEA2000-42 [10.2 Hz, Fresh]") with an expandable details section that displays: "Manufacturer: Garmin (275)", "Model: GPS 17x", "Serial: 123456789", "Software: v2.30"

2. **Given** a source has not yet been discovered (no metadata available), **When** viewing the source in the dashboard, **Then** the expandable details section displays "Device info: Discovering..." as a placeholder, indicating that the system is actively polling for device announcements

3. **Given** a source has been marked as non-compliant after 60 seconds without device announcement, **When** viewing the source in the dashboard, **Then** the expandable details section displays "Device info: Unknown (non-compliant device)", distinguishing it from sources that are still in the discovery phase

4. **Given** I am viewing the sources dashboard on a mobile device (320px viewport width), **When** I expand a device details section, **Then** the metadata fields stack vertically in a responsive layout, with manufacturer, model, serial, and software version each on separate lines for readability

---

### User Story 4 - NMEA0183 Static Talker ID Descriptions (Priority: P4)

As a marine system administrator, I need NMEA0183 sources to display human-readable device type descriptions based on their talker IDs (e.g., "AP" → "Autopilot", "VH" → "VHF Radio"), so that I can identify NMEA0183 devices with the same ease as NMEA2000 devices, despite the lack of a discovery protocol.

**Why this priority**: This is a usability enhancement for NMEA0183 sources that provides consistency with NMEA2000 device identification. However, it's lower priority because NMEA0183 talker IDs are already somewhat self-documenting (e.g., "AP" is recognizable as autopilot to experienced users), and this feature provides informational value rather than diagnostic capability. It can be implemented as a simple lookup table after higher-priority features are complete.

**Independent Test**: Can be tested by connecting NMEA0183 devices with various talker IDs (AP, VH, GP, etc.), viewing the sources dashboard, and verifying that each NMEA0183 source displays the appropriate device type description. Delivers standalone value for NMEA0183 source identification.

**Acceptance Scenarios**:

1. **Given** an autopilot (talker ID "AP") is transmitting RSA sentences and a VHF radio (talker ID "VH") is transmitting GGA sentences, **When** I view the sources dashboard, **Then** the NMEA0183 sources display: "NMEA0183-AP [1.0 Hz, Fresh] - Autopilot" and "NMEA0183-VH [1.0 Hz, Fresh] - VHF Radio"

2. **Given** an NMEA0183 device uses an unrecognized talker ID (e.g., "XY") not in the static lookup table, **When** viewing the source in the dashboard, **Then** the source displays "NMEA0183-XY [1.0 Hz, Fresh] - Unknown NMEA0183 Device", gracefully handling unrecognized talker IDs

3. **Given** the static NMEA0183 talker ID lookup table is defined in a configuration file or header, **When** a developer needs to add support for a new talker ID, **Then** they can add the entry (e.g., `"HC" → "Heading Compass"`) without modifying core source registry logic

---

### Edge Cases

- **What happens when an NMEA2000 device changes its SID dynamically during runtime?** The system treats this as a new source and begins tracking it independently with fresh device discovery. The old SID entry transitions to stale status and is eventually garbage collected after 5 minutes. The device metadata is re-extracted for the new SID via the device list API.

- **What happens when two NMEA2000 devices report the same manufacturer code and model ID but different serial numbers?** The system correlates device metadata using the SID (source address) as the primary key, ensuring each device is tracked separately even if they're identical models. Serial numbers differentiate the devices in the UI.

- **How does the system handle devices that announce themselves multiple times with updated metadata (e.g., firmware update)?** The system updates the stored device metadata when `tN2kDeviceList::ReadResetIsListUpdated()` returns true, replacing old values with new ones. A WebSocket delta message is sent to notify clients of the metadata change.

- **What happens when the NMEA2000 library's device list reaches its internal capacity?** The NMEA2000 library's `tN2kDeviceList` has its own capacity limits (typically 20-50 devices depending on configuration). When this limit is reached, older devices are evicted by the library. Our system reflects this by marking sources without corresponding device list entries as "Unknown device" after the 60-second discovery timeout.

- **How does the system handle NMEA2000 devices that partially populate device information (e.g., manufacturer code present but no model ID)?** The system stores all available fields and marks missing fields as null/empty. The UI displays "N/A" or "Unknown" for missing fields while still showing available metadata (e.g., "Manufacturer: Garmin, Model: N/A").

- **What happens when device discovery polling (every 5 seconds) detects a device list update during high message traffic?** Device discovery runs in a ReactESP timer callback that's non-blocking. The `ReadResetIsListUpdated()` check and metadata extraction complete in <10ms, ensuring minimal impact on message processing. If a burst of messages occurs during discovery, they're buffered by the NMEA2000 library's internal queues.

- **How does the system handle the initial boot scenario where devices may not have announced themselves yet?** All sources initially display "Device info: Discovering..." until the first device list update. Devices typically announce themselves within 10-30 seconds of bus startup. The 60-second timeout ensures non-compliant devices are eventually marked as "Unknown" rather than "Discovering" indefinitely.

- **What WebSocket message is sent when a source times out without device discovery?** When the 60-second discovery timeout occurs, the system sends a WebSocket delta update message: `{"event": "sourceUpdate", "sourceId": "NMEA2000-42", "changes": {"deviceInfo": {"hasInfo": false, "reason": "discovery_timeout"}}}`. This notifies connected clients that the source is now marked as "Unknown (non-compliant)" and device discovery has stopped for this source.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST integrate the NMEA2000 library's `tN2kDeviceList` class to automatically discover NMEA2000 devices on the bus and extract device metadata

- **FR-002**: System MUST create a `tN2kDeviceList` instance attached to the global `NMEA2000` object during initialization in `main.cpp`, before registering message handlers

- **FR-003**: System MUST poll the device list every 5 seconds using a ReactESP timer to check for device list updates via `ReadResetIsListUpdated()` without blocking message processing; device discovery occurs only on scheduled polls, not immediately upon new source detection

- **FR-004**: System MUST extract the following metadata from each discovered device: manufacturer code (uint16_t), model ID (string), product code (uint16_t), software version (string), serial number (uint32_t, unique number), device instance (uint8_t), device class (uint8_t), device function (uint8_t)

- **FR-005**: System MUST correlate discovered device metadata with existing source statistics entries using the SID (source address) as the correlation key

- **FR-006**: System MUST extend the `MessageSource` struct in `SourceStatistics.h` with a `DeviceInfo` nested struct containing fields for all device metadata plus a `hasInfo` boolean flag; string fields MUST use fixed-size buffers: manufacturer (16 bytes), modelId (24 bytes), softwareVersion (12 bytes)

- **FR-007**: System MUST mark device info as unavailable (`hasInfo = false`) for sources that have not been discovered within 60 seconds of their first message, displaying "Unknown device (non-compliant)" in logs and UI; system MUST send WebSocket delta update with `hasInfo = false` and `reason: "discovery_timeout"` field when timeout occurs

- **FR-008**: System MUST update device metadata when the device list changes (detected via `ReadResetIsListUpdated()`), replacing old metadata values with new ones and sending WebSocket delta updates

- **FR-009**: System MUST send WebSocket delta updates when device metadata is first discovered or updated, including the complete `deviceInfo` object in the `changes` field of the `sourceUpdate` event

- **FR-010**: System MUST include device metadata in the initial full snapshot WebSocket message sent to newly connected clients, embedded within each source's data structure

- **FR-011**: System MUST increment the WebSocket schema version from v1 to v2 to indicate the addition of device metadata fields, allowing clients to detect and handle the schema change

- **FR-012**: System MUST display device metadata in the `/sources` web dashboard UI, showing manufacturer, model, serial number, and software version in an expandable details section for each NMEA2000 source

- **FR-013**: System MUST display appropriate status placeholder text for undiscovered sources using `DeviceInfo::getStatusString()` method: "Discovering..." (within 60 seconds after first message) or "Unknown (timeout)" (after 60-second discovery timeout)

- **FR-014**: System MUST use `DeviceInfo::getStatusString()` to determine display status, distinguishing between sources in the discovery phase and those that have exceeded the timeout without being found in the device list

- **FR-015**: System MUST implement a static lookup table for NMEA0183 talker IDs mapping to human-readable device type descriptions (e.g., "AP" → "Autopilot", "VH" → "VHF Radio", "GP" → "GPS Receiver")

- **FR-016**: System MUST display NMEA0183 device type descriptions in the sources dashboard based on the talker ID lookup table, with fallback to "Unknown NMEA0183 Device" for unrecognized talker IDs

- **FR-017**: System MUST handle partial device metadata gracefully, storing available fields and displaying "N/A" or "Unknown" for missing fields in the UI

- **FR-018**: System MUST use static memory allocation for device metadata storage where possible, with maximum limits: 50 total sources × 72 bytes per DeviceInfo struct (1 bool + 11 numeric fields + 3 fixed strings: 16+24+12 bytes + alignment padding) = 3.6KB maximum additional RAM

- **FR-019**: System MUST log device discovery events to the WebSocket logger at DEBUG level, including: device discovered (SID, manufacturer, model), device metadata updated (SID, changed fields), device list capacity warnings

- **FR-020**: System MUST NOT store device metadata in persistent storage (LittleFS); all device information is rediscovered on each system reboot

- **FR-021**: System MUST maintain compatibility with the existing SourceRegistry API, ensuring that device metadata enrichment does not break existing source statistics functionality (frequency tracking, staleness detection, garbage collection)

- **FR-022**: System MUST complete device metadata extraction and SourceRegistry updates within 10ms per device list poll cycle to avoid impacting message processing performance

### Key Entities

- **DeviceInfo**: Represents NMEA2000 device metadata extracted from the device list. Key attributes: `hasInfo` (bool, true if device was discovered), `manufacturerCode` (uint16_t, NMEA2000 manufacturer code), `manufacturer` (char[16], human-readable manufacturer name), `modelId` (char[24], device model identifier), `productCode` (uint16_t, product code), `serialNumber` (uint32_t, unique device serial), `softwareVersion` (char[12], firmware version), `deviceInstance` (uint8_t, instance number for multi-device setups), `deviceClass` (uint8_t, NMEA2000 device class), `deviceFunction` (uint8_t, NMEA2000 device function). Total size: 72 bytes (with alignment padding). Embedded within MessageSource struct.

- **DeviceInfoCollector**: New component responsible for polling the NMEA2000 device list and enriching SourceRegistry with device metadata. Key responsibilities: Poll `tN2kDeviceList::ReadResetIsListUpdated()` every 5 seconds, iterate discovered devices via `FindDeviceBySource()`, extract metadata from `tDevice` objects, update MessageSource entries in SourceRegistry, send WebSocket notifications for metadata changes. Interfaces with: `tN2kDeviceList` (NMEA2000 library), `SourceRegistry` (existing), `WebSocketLogger` (existing).

- **ManufacturerLookup**: (Optional, if NMEA2000 library doesn't provide manufacturer name lookup) Static utility for mapping manufacturer codes to human-readable names. Key function: `const char* getManufacturerName(uint16_t code)` returns manufacturer name or "Unknown Manufacturer (code)" fallback. Implementation: Static array or switch statement with ~50 common manufacturers (Garmin, Furuno, Raymarine, Simrad, etc.).

- **TalkerIdLookup**: Static utility for mapping NMEA0183 talker IDs to device type descriptions. Key function: `const char* getTalkerDescription(const char* talkerId)` returns description or "Unknown NMEA0183 Device" fallback. Implementation: Static string map or switch statement covering common talker IDs (AP, VH, GP, HC, etc.).

- **MessageSource** (Extended): Existing entity representing an individual NMEA message source, now enhanced with embedded DeviceInfo struct. New field: `deviceInfo` (DeviceInfo struct, populated for NMEA2000 sources, hasInfo=false for NMEA0183 or undiscovered sources).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: System successfully discovers and extracts complete metadata (manufacturer, model, serial, software version) for 100% of compliant NMEA2000 devices within 30 seconds of device announcement, verified by connecting 3+ devices from different manufacturers (Garmin, Furuno, Raymarine)

- **SC-002**: Device discovery polling completes in under 10ms per 5-second cycle when 20 devices are on the bus, measured via `millis()` timestamps before/after `DeviceInfoCollector::pollDeviceList()`, ensuring no impact on message processing latency

- **SC-003**: WebSocket clients receive device metadata in the initial full snapshot message within 100ms of connection, with JSON payload size increase of less than 2KB for a typical 10-device configuration, verified by WebSocket client timestamping

- **SC-004**: Device metadata appears correctly in the sources dashboard UI for all discovered NMEA2000 sources, with manufacturer, model, serial, and software version displayed in expandable sections, verified by manual browser testing with 3+ device types

- **SC-005**: Sources that have not been discovered within 60 seconds display "Device info: Unknown (non-compliant)" placeholder text in both WebSocket messages and UI, distinguishing them from sources in the discovery phase ("Discovering...")

- **SC-006**: NMEA0183 sources display appropriate device type descriptions (e.g., "Autopilot", "VHF Radio") based on talker ID lookup table, verified by connecting devices with AP, VH, and GP talker IDs

- **SC-007**: System memory footprint increases by less than 5KB when tracking 20 sources with full device metadata (50 sources × 72 bytes = 3.6KB + library overhead ≈ 4.5KB total), measured via heap monitoring before/after device discovery initialization

- **SC-008**: Device metadata updates correctly when devices announce firmware version changes, with WebSocket delta messages sent to connected clients within 500ms of device list update detection, verified by simulating device re-announcement with modified metadata

- **SC-009**: Zero message processing delays or dropped messages occur during device discovery polling over a 24-hour continuous operation test with 10+ devices transmitting at various frequencies (1Hz to 10Hz)

- **SC-010**: 90% of marine system administrators successfully identify a specific device (manufacturer and model) in under 30 seconds using the sources dashboard during user acceptance testing, compared to baseline of "unable to identify" with SID-only display

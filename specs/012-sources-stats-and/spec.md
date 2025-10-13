# Feature Specification: Source Statistics Tracking and WebUI

**Feature Branch**: `012-sources-stats-and`
**Created**: 2025-10-13
**Status**: Draft
**Input**: User description: "Sources Stats and WebUI as specified in user_requirements/R012 - sources stats and webui.md"

## Clarifications

### Session 2025-10-13

- Q: Should clients receive an explicit removal notification when garbage collection removes stale sources, or do removed sources simply disappear from the next full statistics update? → A: Send explicit WebSocket removal event for each garbage-collected source (e.g., `{"event": "sourceRemoved", "sourceId": "NMEA2000-42"}`)
- Q: When a source's frequency or staleness changes, should the WebSocket send full snapshots of all sources, or incremental updates for only the changed sources? → A: Hybrid: Send full snapshot on initial connection, then incremental deltas for subsequent updates
- Q: What should happen when the 50-source limit is reached and a new source is discovered? → A: Evict oldest inactive: Remove the source with the longest time-since-last-update (even if not yet garbage-collected)
- Q: When should the system send WebSocket delta messages to connected clients? → A: Batched periodic: Accumulate changes and send batched deltas every 500ms (balanced latency and efficiency)
- Q: For consistent implementation, should the circular buffer store exactly how many timestamps? → A: 10

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Real-time Source Discovery and Statistics (Priority: P1)

As a marine system administrator, I need to see which NMEA 2000 and NMEA 0183 sources are actively providing each type of message (PGN/sentence), along with their update frequencies and staleness indicators, so that I can diagnose sensor connectivity issues and understand my system topology.

**Why this priority**: This is the foundational capability that enables all source prioritization decisions. Without visibility into which sources provide which data at what frequency, it's impossible to make informed decisions about source selection or diagnose connectivity problems. This delivers immediate diagnostic value independent of any prioritization logic.

**Independent Test**: Can be fully tested by connecting multiple NMEA 2000 devices (e.g., 2 GPS units, 1 compass) and multiple NMEA 0183 talkers (e.g., autopilot + VHF), then accessing a WebSocket endpoint to verify that all sources are discovered, their update frequencies are calculated correctly, and staleness indicators update in real-time. Delivers diagnostic value immediately without requiring any prioritization features.

**Acceptance Scenarios**:

1. **Given** two NMEA 2000 GPS units broadcasting PGN 129025 at different frequencies (10Hz and 1Hz), **When** I connect to the source statistics WebSocket endpoint, **Then** I see both sources listed under the GPS category with their respective SIDs, measured frequencies (approximately 10Hz and 1Hz), and time-since-last-update values refreshing continuously
2. **Given** an autopilot (talker ID "AP") transmitting RSA sentences at 1Hz and a VHF radio (talker ID "VH") transmitting GGA sentences at 1Hz, **When** I access the source statistics data, **Then** I see both NMEA 0183 sources listed under their respective BoatData categories with talker IDs and measured update frequencies
3. **Given** a previously active NMEA 2000 source stops transmitting (e.g., unplugged), **When** more than 5 seconds elapse without an update, **Then** the isStale flag for that source transitions to true and the time-since-last-update continues incrementing
4. **Given** a new NMEA 2000 device is connected during runtime, **When** it begins transmitting supported PGNs, **Then** the new source is automatically discovered and added to the statistics without requiring a system restart

---

### User Story 2 - WebUI Dashboard for Source Monitoring (Priority: P2)

As a marine system administrator, I need a web-based dashboard that displays the source statistics in a human-readable format organized by BoatData categories, so that I can easily monitor my system's sensor health without requiring technical knowledge of PGNs or sentence types.

**Why this priority**: While the WebSocket endpoint (P1) provides the raw data, a user-friendly dashboard is essential for practical usability. This builds directly on P1's data structure and makes the system accessible to non-technical users. It can be developed and tested independently once P1 is complete.

**Independent Test**: Can be tested by opening the web dashboard in a browser, verifying that all discovered sources from P1 are displayed in a table or card layout organized by category (GPS, Compass, DST, Engine, Wind, etc.), and confirming that the display updates in real-time as new messages arrive. Delivers standalone value for system monitoring.

**Acceptance Scenarios**:

1. **Given** multiple active NMEA sources are transmitting, **When** I navigate to the source statistics web page at `http://<ESP32_IP>:3030/sources`, **Then** I see a dashboard organized by BoatData categories (GPS, Compass, DST, Engine, Wind, Rudder) with each source listed under its appropriate category
2. **Given** a source is actively transmitting, **When** viewing the dashboard, **Then** each source entry displays: source identifier (NMEA2000-<SID> or NMEA0183-<TalkerID>), message type (PGN number or sentence type), update frequency in Hz (formatted to 1 decimal place), time since last update in milliseconds, and staleness indicator (green = fresh, red = stale)
3. **Given** the web page is open, **When** a source transitions from fresh to stale (5+ seconds without update), **Then** the visual staleness indicator changes from green to red without requiring a page refresh
4. **Given** I am monitoring the dashboard on a mobile device, **When** I access the page, **Then** the layout adapts responsively to display source statistics in a readable format on smaller screens

---

### User Story 3 - Node.js Proxy Integration for Multi-Client Support (Priority: P3)

As a developer or power user, I need the Node.js boatdata viewer proxy to support both the existing boat data stream and the new source statistics stream, so that I can simultaneously monitor sensor data and source health from the same application with support for multiple concurrent clients.

**Why this priority**: This extends the existing Node.js infrastructure to include source statistics, providing consistency in the tooling ecosystem. However, it's a convenience enhancement rather than a core requirement, as the ESP32 can serve the dashboard directly to single clients. This is useful for development and multi-user scenarios but not essential for basic functionality.

**Independent Test**: Can be tested by running the Node.js proxy, configuring it to connect to the ESP32 WebSocket endpoint for source statistics, and accessing both `/stream` (existing boat data) and `/sources` (new source stats) endpoints through the proxy. Verify that multiple browser clients can connect simultaneously without overloading the ESP32. Delivers value for development workflows and multi-user environments.

**Acceptance Scenarios**:

1. **Given** the Node.js proxy is running and connected to the ESP32, **When** I navigate to `http://localhost:3000/sources`, **Then** I see the same source statistics dashboard that would be served directly from the ESP32, with data relayed through the WebSocket proxy
2. **Given** multiple clients are connected to the Node.js proxy, **When** source statistics update on the ESP32, **Then** all connected clients receive the updates without causing ESP32 resource exhaustion
3. **Given** the Node.js proxy is running, **When** I navigate between `/stream` (boat data) and `/sources` (source stats) pages, **Then** both pages function correctly with real-time WebSocket updates on each page
4. **Given** the ESP32 WebSocket connection to the proxy is interrupted, **When** the connection is restored, **Then** the proxy automatically reconnects and resumes streaming source statistics data

---

### Edge Cases

- **What happens when a NMEA 2000 source changes its SID dynamically?** The system should treat this as a new source and begin tracking it independently. The old SID will transition to stale status.
- **What happens when update frequencies fluctuate significantly (e.g., 8-12 Hz for a "10Hz" source)?** The frequency calculation should use a rolling average over the last 10 updates to smooth out transient variations while still detecting genuine frequency changes.
- **How does the system handle sources that only transmit sporadically by design (e.g., engine parameters only when engine is running)?** Sources should be marked as stale after 5 seconds of inactivity, but remain in the statistics table with their last-known frequency. When they resume transmitting, they should be automatically reactivated without creating duplicate entries.
- **What happens when the ESP32 receives more PGN/sentence types than the supported list?** The system should only track statistics for the explicitly supported PGNs and sentences defined in NMEA2000Handlers.h and NMEA0183Handler.h. Unsupported message types are silently ignored (no statistics tracked).
- **How does the system handle NMEA 0183 talker IDs that conflict with NMEA 2000 SIDs (e.g., both use numeric identifiers)?** Source identifiers must include the protocol type prefix (NMEA2000-<SID> vs NMEA0183-<TalkerID>) to ensure uniqueness and clear differentiation in the UI.
- **What happens when the statistics data structure grows large with many stale sources?** The system should implement a garbage collection strategy that removes sources that have been stale for more than 5 minutes, with a maximum cap on total tracked sources (e.g., 50 sources total across all categories). When sources are removed, the system sends explicit WebSocket removal events to all connected clients (format: `{"event": "sourceRemoved", "sourceId": "<sourceId>"}`). When the 50-source limit is reached and a new source is discovered, the system evicts the source with the longest time-since-last-update (even if not yet stale for 5 minutes) to make room for the new active source.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST track statistics for all NMEA 2000 PGNs listed in `src/components/NMEA2000Handlers.h` (13 PGNs: 129025, 129026, 129029, 127258, 127250, 127251, 127252, 127257, 128267, 128259, 130316, 127488, 127489, 130306)
- **FR-002**: System MUST track statistics for all NMEA 0183 sentences listed in `src/components/NMEA0183Handler.h` (5 sentences: RSA, HDM, GGA, RMC, VTG)
- **FR-003**: System MUST identify each NMEA 2000 source uniquely by its SID (Source Identifier) field from the PGN message header
- **FR-004**: System MUST identify each NMEA 0183 source uniquely by its talker ID (first two characters of the sentence, e.g., "AP", "VH")
- **FR-005**: System MUST calculate update frequency in Hz for each source using a rolling average over the last 10 messages (circular buffer of 10 timestamps)
- **FR-006**: System MUST track time-since-last-update in milliseconds for each source based on `millis()` timestamp
- **FR-007**: System MUST mark a source as stale (isStale = true) when no update has been received for more than 5000 milliseconds
- **FR-008**: System MUST organize source statistics according to BoatData categories (GPS, Compass, Wind, DST, Rudder, Engine, Saildrive, Battery, ShorePower)
- **FR-009**: System MUST expose source statistics via a WebSocket endpoint at `ws://<ESP32_IP>/source-stats` with JSON payload format using hybrid update strategy: full snapshot on initial connection, then incremental deltas for subsequent changes
- **FR-010**: System MUST serve a web page at `http://<ESP32_IP>:3030/sources` from LittleFS that displays source statistics in a human-readable format
- **FR-011**: Web page MUST connect to the WebSocket endpoint and update statistics display in real-time without page refresh
- **FR-012**: System MUST update source statistics incrementally as messages are received (no periodic batch processing required)
- **FR-013**: System MUST support discovery of new sources at runtime without requiring system restart or reinitialization
- **FR-014**: System MUST persist source statistics in RAM only (no flash storage required for statistics data)
- **FR-015**: Source statistics data structure MUST follow the format: `BoatData Category → Message Type [PGN | Sentence] → Source [NMEA2000-<SID> | NMEA0183-<TalkerID>] → {frequency (Hz), timeSinceLast (ms), isStale (bool)}`
- **FR-016**: Node.js proxy (`nodejs-boatdata-viewer`) MUST be extended with a `/sources` endpoint that relays source statistics from the ESP32 WebSocket
- **FR-017**: Node.js proxy MUST support simultaneous connections for both `/stream` (boat data) and `/sources` (source stats) endpoints
- **FR-018**: System MUST implement garbage collection to remove sources that have been stale for more than 300000 milliseconds (5 minutes)
- **FR-019**: System MUST enforce a maximum limit of 50 total tracked sources across all categories to prevent memory exhaustion; when limit is reached and a new source is discovered, system MUST evict the source with the longest time-since-last-update and send a removal event to connected clients
- **FR-020**: WebSocket message format MUST include schema version identifier for future compatibility
- **FR-021**: System MUST send explicit WebSocket removal events when garbage collection removes sources (format: `{"event": "sourceRemoved", "sourceId": "<sourceId>", "timestamp": <millis>}`) to notify connected clients
- **FR-022**: WebSocket MUST send initial full snapshot message on client connection (format: `{"event": "fullSnapshot", "version": <schemaVersion>, "timestamp": <millis>, "sources": {...}}`) containing all currently tracked sources
- **FR-023**: WebSocket MUST send incremental delta messages for source changes after initial snapshot (format: `{"event": "sourceUpdate", "sourceId": "<sourceId>", "timestamp": <millis>, "changes": {"frequency": <Hz>, "timeSinceLast": <ms>, "isStale": <bool>}}`) containing only modified fields
- **FR-024**: System MUST batch WebSocket delta updates and transmit them every 500ms to balance latency and efficiency; multiple source changes within the same 500ms window MUST be accumulated into a single batched message

### Key Entities *(include if feature involves data)*

- **SourceStatistics**: Represents the complete hierarchy of source statistics organized by category, message type, and source identifier. Contains nested structures for each category (GPS, Compass, etc.), each with arrays of message types, each containing arrays of sources. Each source includes frequency (Hz), timeSinceLast (ms), and isStale (bool) fields.

- **MessageSource**: Represents an individual source of NMEA messages. Key attributes: sourceId (string, format "NMEA2000-<SID>" or "NMEA0183-<TalkerID>"), messageType (PGN number or sentence type string), protocol (NMEA2000 or NMEA0183), category (enum: GPS, Compass, Wind, etc.), frequency (double, Hz), timeSinceLast (unsigned long, ms), isStale (bool), lastUpdateTime (unsigned long, millis() timestamp), updateCount (unsigned long, for frequency calculation).

- **CategoryStats**: Groups all message sources belonging to a specific BoatData category. Key attributes: categoryName (enum: GPS, Compass, Wind, DST, Rudder, Engine, Saildrive, Battery, ShorePower), messages (array of MessageType entries), sourceCount (int, total active sources in this category).

- **MessageType**: Groups all sources providing a specific PGN or sentence type. Key attributes: identifier (string, e.g., "PGN129025" or "RSA"), protocol (NMEA2000 or NMEA0183), sources (array of MessageSource entries), primarySourceIndex (int, reserved for future prioritization feature).

- **FrequencyCalculator**: Utility component responsible for calculating rolling average update frequencies. Maintains a circular buffer of exactly 10 update timestamps per source and calculates Hz based on average interval between timestamps.

- **SourceRegistry**: Manages the collection of all tracked sources, handles new source discovery, garbage collection of stale sources, and enforcement of the 50-source limit. Provides lookup functions by category, message type, and source ID.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: System successfully discovers and tracks at least 5 concurrent NMEA 2000 sources and 2 concurrent NMEA 0183 sources without missing any messages or corrupting statistics data
- **SC-002**: Update frequency measurements are accurate within ±10% of the actual transmission rate for sources transmitting at stable rates (e.g., 10Hz source measured as 9.0-11.0 Hz)
- **SC-003**: Staleness detection triggers reliably within 5.5 seconds (5.0s threshold + 0.5s tolerance) of the last message from any source, verified by disconnecting sources during testing
- **SC-004**: WebSocket endpoint delivers batched source statistics updates within 500ms ±50ms window (batching interval), measured via client-side timestamping of update events
- **SC-005**: Web dashboard page loads and displays initial source statistics within 2 seconds of page request on the ESP32's web server
- **SC-006**: Real-time dashboard updates reflect source statistics changes within 200ms of WebSocket message arrival (visual update latency measured via browser performance tools)
- **SC-007**: System memory footprint for source statistics remains under 10KB for typical configurations (up to 20 active sources) to ensure compatibility with ESP32 RAM constraints
- **SC-008**: Garbage collection successfully removes stale sources after 5 minutes and reclaims memory, verified by monitoring heap usage before and after collection cycles
- **SC-009**: Node.js proxy supports at least 10 concurrent client connections to the `/sources` endpoint without dropped WebSocket messages or significant latency increase (latency under 150ms)
- **SC-010**: Source statistics data structure serializes to JSON format with payload size under 5KB for typical configurations, ensuring efficient WebSocket transmission
- **SC-011**: 90% of marine system administrators successfully identify connectivity issues (e.g., stale GPS source) within 1 minute of accessing the source statistics dashboard during user testing
- **SC-012**: Zero source discovery failures occur during 24-hour continuous operation with sources intermittently connecting and disconnecting (simulated by toggling devices on/off)

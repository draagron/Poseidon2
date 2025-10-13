# Feature Specification: Simple WebUI for BoatData Streaming

**Feature Branch**: `011-simple-webui-as`
**Created**: 2025-10-13
**Status**: Draft
**Input**: User description: "simple webui as per user_requirements/R009 - webui.md"

## Clarifications

### Session 2025-10-13

- Q: FR-004 specifies WebSocket broadcasts should be throttled to "1-2 Hz". Which exact broadcast rate should be implemented? → A: Exactly 1 Hz (1000ms interval)
- Q: Should the spec include an explicit security posture statement documenting "no authentication" and "no HTTPS" design decisions? → A: Yes - Add security posture to spec

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Real-time BoatData Streaming via WebSocket (Priority: P1)

A sailor wants to view real-time boat sensor data on their smartphone/tablet browser while onboard, without needing to install any apps or software.

**Why this priority**: This is the core functionality that enables remote monitoring of all boat systems. Without this, there's no way for users to access the data collected by the Poseidon2 gateway. It's the foundation for all other features.

**Independent Test**: Can be fully tested by connecting to the ESP32's WebSocket endpoint (e.g., `ws://192.168.1.100/boatdata`) from a WebSocket test client and verifying that JSON-formatted BoatData messages are received at regular intervals.

**Acceptance Scenarios**:

1. **Given** the ESP32 is powered on and connected to WiFi, **When** a client connects to the WebSocket endpoint `/boatdata`, **Then** the connection is established successfully and the client receives an initial BoatData snapshot
2. **Given** a client is connected to the `/boatdata` WebSocket, **When** sensor data updates occur (e.g., GPS position changes), **Then** the client receives updated BoatData messages within 1 second
3. **Given** multiple clients are connected to the WebSocket, **When** BoatData updates, **Then** all connected clients receive the same data simultaneously
4. **Given** a client is connected to the WebSocket, **When** no sensor data is available (e.g., GPS signal lost), **Then** the client receives BoatData with `available: false` flags for affected sensors
5. **Given** the ESP32 is receiving high-frequency NMEA2000 data (10 Hz), **When** streaming to WebSocket clients, **Then** data is throttled to exactly 1 Hz (1000ms interval) to prevent network congestion

---

### User Story 2 - Static HTML Dashboard Hosting (Priority: P2)

A sailor wants to access a simple web dashboard by navigating to a URL on their browser (e.g., `http://192.168.1.100/stream`) to view live boat data without needing to set up separate hosting infrastructure.

**Why this priority**: This provides the user interface layer that makes the WebSocket data accessible to non-technical users. While the WebSocket endpoint (P1) is functional on its own, this HTML page makes the system usable without requiring custom client development.

**Independent Test**: Can be fully tested by navigating to `http://192.168.1.100/stream` in a web browser and verifying that an HTML page loads from the ESP32's LittleFS storage, displaying a dashboard layout with placeholders for boat data.

**Acceptance Scenarios**:

1. **Given** the ESP32 web server is running, **When** a user navigates to `http://<ESP32_IP>/stream` in a browser, **Then** a static HTML page is served from LittleFS storage and renders correctly
2. **Given** the HTML page is loaded, **When** the page initializes, **Then** JavaScript automatically connects to the `/boatdata` WebSocket endpoint on the same host
3. **Given** the HTML file is stored in LittleFS, **When** the file is updated via OTA or serial upload, **Then** the new version is served on subsequent page loads without requiring firmware recompile
4. **Given** LittleFS storage is unavailable or the HTML file is missing, **When** a user navigates to `/stream`, **Then** the server returns a 404 error or a fallback error page

---

### User Story 3 - Organized Data Display with BoatData Structure Grouping (Priority: P3)

A sailor wants to see boat sensor data organized into logical categories (GPS, Compass, Wind, DST, Engine, etc.) on the web dashboard, with data fields labeled using familiar names from the BoatData structure.

**Why this priority**: This enhances usability by presenting data in a structured, scannable format. While less critical than getting the data to the browser (P1/P2), it significantly improves the user experience by making large amounts of sensor data comprehensible at a glance.

**Independent Test**: Can be fully tested by loading the `/stream` page and verifying that data is displayed in separate sections (GPS, Compass, Wind, DST, Engine, Battery, etc.) with labels matching the BoatData structure field names (e.g., `latitude`, `longitude`, `trueHeading`, `apparentWindSpeed`).

**Acceptance Scenarios**:

1. **Given** the dashboard HTML page is loaded, **When** BoatData is received via WebSocket, **Then** GPS data is displayed in a "GPS" section with fields: `latitude`, `longitude`, `cog`, `sog`, `variation`
2. **Given** BoatData contains compass data, **When** displayed on the dashboard, **Then** data is grouped in a "Compass" section with fields: `trueHeading`, `magneticHeading`, `rateOfTurn`, `heelAngle`, `pitchAngle`, `heave`
3. **Given** BoatData contains wind data, **When** displayed, **Then** data appears in a "Wind" section with fields: `apparentWindAngle`, `apparentWindSpeed`
4. **Given** BoatData contains DST data, **When** displayed, **Then** data appears in a "DST" section with fields: `depth`, `measuredBoatSpeed`, `seaTemperature`
5. **Given** BoatData contains engine data, **When** displayed, **Then** data appears in an "Engine" section with fields: `engineRev`, `oilTemperature`, `alternatorVoltage`
6. **Given** BoatData contains battery data, **When** displayed, **Then** data appears in a "Battery" section with separate subsections for Battery A and Battery B, showing voltage, amperage, state of charge, and charger status
7. **Given** a sensor's `available` flag is `false`, **When** displayed on the dashboard, **Then** the corresponding data section shows "N/A" or grayed-out values
8. **Given** BoatData updates arrive via WebSocket, **When** values change, **Then** the displayed values update in real-time without requiring page refresh

---

### Edge Cases

- What happens when the WebSocket connection drops unexpectedly? → Dashboard should detect disconnection, display a "Disconnected" status, and automatically attempt to reconnect
- How does the system handle network congestion when broadcasting to multiple clients? → WebSocket broadcast should be non-blocking; if a client's send buffer is full, that client is disconnected to prevent blocking other clients
- What happens when LittleFS storage becomes corrupted or the HTML file is deleted? → The `/stream` endpoint should return a 404 error or serve a minimal inline HTML error page
- How does the dashboard handle missing or incomplete BoatData? → Each data section should gracefully handle missing fields by displaying "N/A" or empty values
- What happens when sensor data arrives faster than the WebSocket can send (e.g., 10 Hz NMEA2000 updates)? → System should throttle updates to a sustainable rate (1-2 Hz) by sampling the latest available data
- How does the system handle very old or stale data? → Dashboard should display a timestamp or "Last Updated" indicator and gray out stale data (e.g., >5 seconds old)
- What happens when the ESP32 runs out of memory due to too many WebSocket clients? → System should limit the maximum number of concurrent WebSocket connections (e.g., 5 clients) and reject new connections with a clear error message

## Requirements *(mandatory)*

### Functional Requirements

#### WebSocket BoatData Streaming
- **FR-001**: System MUST provide a WebSocket endpoint at `/boatdata` for streaming real-time boat sensor data
- **FR-002**: System MUST serialize the current BoatData structure to JSON format for WebSocket transmission
- **FR-003**: System MUST broadcast BoatData updates to all connected WebSocket clients when sensor data changes
- **FR-004**: System MUST throttle BoatData broadcasts to exactly 1 Hz (1000ms interval) to prevent network congestion
- **FR-005**: System MUST handle WebSocket connections asynchronously without blocking the main event loop or sensor data processing
- **FR-006**: System MUST support a minimum of 5 concurrent WebSocket client connections without performance degradation
- **FR-007**: System MUST automatically disconnect clients if their send buffer becomes full (non-blocking send)
- **FR-008**: System MUST include all BoatData structure groups in the JSON payload: GPS, Compass, Wind, DST, Rudder, Engine, Saildrive, Battery, ShorePower

#### Static HTML Hosting
- **FR-009**: System MUST serve a static HTML file from LittleFS storage at the HTTP endpoint `/stream`
- **FR-010**: System MUST store the HTML dashboard file in LittleFS with the path `/stream.html` (or similar configurable path)
- **FR-011**: System MUST set the correct Content-Type header (`text/html`) when serving the HTML file
- **FR-012**: System MUST return HTTP 404 error if the HTML file is not found in LittleFS storage
- **FR-013**: System MUST support updating the HTML file via LittleFS upload without requiring firmware recompilation

#### HTML Dashboard Content
- **FR-014**: HTML page MUST include JavaScript code that automatically connects to the `/boatdata` WebSocket endpoint on page load
- **FR-015**: HTML page MUST display BoatData grouped into logical sections: GPS, Compass, Wind, DST, Rudder, Engine, Saildrive, Battery, ShorePower
- **FR-016**: HTML page MUST label each data field using names consistent with the BoatData structure (e.g., `latitude`, `longitude`, `trueHeading`)
- **FR-017**: HTML page MUST update displayed values in real-time when new WebSocket messages are received, without requiring page refresh
- **FR-018**: HTML page MUST display "N/A" or grayed-out values for sensors where the `available` flag is `false`
- **FR-019**: HTML page MUST display a connection status indicator showing whether the WebSocket is connected or disconnected
- **FR-020**: HTML page MUST automatically attempt to reconnect to the WebSocket if the connection is lost
- **FR-021**: HTML page MUST display timestamps or "Last Updated" indicators to show data freshness

#### Data Format and Units
- **FR-022**: WebSocket JSON payload MUST include all fields from the BoatData structure, including `available` and `lastUpdate` metadata
- **FR-023**: WebSocket JSON payload MUST use consistent units matching the BoatData structure documentation (angles in radians, speeds in knots/m/s, etc.)
- **FR-024**: HTML dashboard MUST convert units to user-friendly formats for display (e.g., radians → degrees, radians/s → degrees/s, meters/s → knots if appropriate)
- **FR-025**: HTML dashboard MUST display numeric values with appropriate precision (e.g., GPS coordinates to 6 decimal places, heading to 1 decimal place)

### Security & Privacy

**Security Posture**: This feature is designed for **private boat networks only** and intentionally omits security controls appropriate for public/internet-exposed deployments.

**Explicit Design Decisions**:
- **No Authentication**: WebSocket and HTTP endpoints do not require login credentials or API keys
  - **Rationale**: Private boat WiFi network (not internet-exposed), consistency with existing WebSocketLogger implementation, simplicity for MVP
  - **Risk Acceptance**: Anyone on the boat network can view telemetry data (accepted for private network use case)

- **No HTTPS/TLS**: All communication uses plain HTTP and WebSocket (ws://, not wss://)
  - **Rationale**: Local network only, ESP32 TLS overhead significant (CPU and memory), no sensitive credentials transmitted
  - **Risk Acceptance**: Network traffic is unencrypted and visible to anyone on the boat WiFi network

- **No Authorization**: No role-based access control or permissions system
  - **Rationale**: Single-user boat system, read-only telemetry data only, no control commands

- **No Input Validation Required**: Dashboard is read-only (no user input, no forms, no POST requests)
  - **Security**: No injection risks (XSS, SQL injection, command injection) as system does not accept user input

**Threat Model Assumptions**:
- Boat WiFi network is trusted (crew and authorized guests only)
- Physical access to boat implies authorized access to telemetry data
- No internet exposure (ESP32 not port-forwarded or exposed via cloud services)
- Telemetry data is not sensitive (boat sensor readings, not personal/financial data)

**Future Enhancements** (out of scope for this feature):
- Add HTTP Basic Authentication if boat network becomes less trusted
- Add HTTPS support if internet exposure or remote monitoring is required
- Add CORS restrictions if integrating with third-party web services

### Key Entities

- **WebSocket Endpoint (`/boatdata`)**: A WebSocket server endpoint that accepts client connections and streams JSON-serialized BoatData messages at exactly 1 Hz (1000ms interval). Handles connection lifecycle (connect, disconnect, error) and broadcasts data to multiple clients simultaneously.

- **HTTP Endpoint (`/stream`)**: An HTTP GET endpoint that serves a static HTML file from LittleFS storage. Returns `text/html` content type and handles 404 errors if the file is missing.

- **BoatData JSON Message**: A JSON-formatted representation of the complete BoatData structure, including all sensor groups (GPS, Compass, Wind, DST, Rudder, Engine, Saildrive, Battery, ShorePower) with their respective fields, availability flags, and timestamps.

- **HTML Dashboard File**: A static HTML file stored in LittleFS (`/stream.html`) containing HTML, CSS, and embedded JavaScript. The JavaScript establishes a WebSocket connection, parses incoming JSON messages, and dynamically updates the DOM to display real-time sensor data.

- **Data Grouping Structure**: The organizational scheme for displaying BoatData on the HTML dashboard, with separate visual sections for each sensor category (GPS, Compass, Wind, DST, etc.), matching the BoatData C++ structure layout.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: WebSocket endpoint successfully handles 5 concurrent client connections without dropped messages or performance degradation
- **SC-002**: BoatData updates are broadcast to all connected clients within 1 second of sensor data changes
- **SC-003**: HTML dashboard page loads in under 2 seconds on a typical WiFi connection
- **SC-004**: WebSocket connection remains stable for at least 8 hours of continuous operation without disconnections or memory leaks
- **SC-005**: Dashboard displays all BoatData groups (GPS, Compass, Wind, DST, Engine, Battery) with correctly labeled fields matching the BoatData structure
- **SC-006**: Dashboard automatically reconnects to the WebSocket within 5 seconds after a connection drop
- **SC-007**: System memory usage remains stable (no memory leaks) when streaming to WebSocket clients for extended periods (>24 hours)
- **SC-008**: WebSocket broadcast rate does not exceed 2 Hz even when receiving high-frequency NMEA2000 data (10 Hz)
- **SC-009**: LittleFS HTML file can be updated without firmware recompile, and the new version is served immediately on next page load
- **SC-010**: Dashboard correctly displays "N/A" or grayed-out values for sensors with `available: false` within 1 second of status change

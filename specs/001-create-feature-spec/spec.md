---
**⚠️ LEGACY IMPLEMENTATION NOTICE**

This specification describes the initial WiFi management implementation which used UDP broadcast logging (port 4444). The system has since been migrated to WebSocket logging for improved reliability.

**Current Implementation**: WebSocket logging via `ws://<device-ip>/logs`
**Historical Implementation** (described below): UDP broadcast logging on port 4444

This document is preserved for historical reference and architectural decision context. For current logging setup, see [README.md](../../README.md) and [CLAUDE.md](../../CLAUDE.md).

---

# Feature Specification: WiFi Network Management Foundation

**Feature Branch**: `001-create-feature-spec`
**Created**: 2025-10-06
**Status**: Draft
**Input**: User description: "create feature spec based on the foundation requirement defined in user_requirements/R001_foundation.md"

## Execution Flow (main)
```
1. Parse user description from Input
   → If empty: ERROR "No feature description provided"
2. Extract key concepts from description
   → Identify: actors, actions, data, constraints
3. For each unclear aspect:
   → Mark with [NEEDS CLARIFICATION: specific question]
4. Fill User Scenarios & Testing section
   → If no clear user flow: ERROR "Cannot determine user scenarios"
5. Generate Functional Requirements
   → Each requirement must be testable
   → Mark ambiguous requirements
6. Identify Key Entities (if data involved)
7. Run Review Checklist
   → If any [NEEDS CLARIFICATION]: WARN "Spec has uncertainties"
   → If implementation details found: ERROR "Remove tech details"
8. Return: SUCCESS (spec ready for planning)
```

---

## ⚡ Quick Guidelines
- ✅ Focus on WHAT users need and WHY
- ❌ Avoid HOW to implement (no tech stack, APIs, code structure)
- 👥 Written for business stakeholders, not developers

### Section Requirements
- **Mandatory sections**: Must be completed for every feature
- **Optional sections**: Include only when relevant to the feature
- When a section doesn't apply, remove it entirely (don't leave as "N/A")

---

## Clarifications

### Session 2025-10-06
- Q: What is the connection timeout per WiFi network attempt? → A: 30 seconds - Balanced approach for typical conditions
- Q: When a connected WiFi network drops (connection lost after successful connection), what should the gateway do? → A: Retry current network only (stay loyal to last successful)
- Q: What is the maximum number of WiFi networks supported in the configuration? → A: 3 networks - Minimal (home/marina/alternate)
- Q: Should other gateway services wait for WiFi connection, or run in parallel during WiFi connection attempts? → A: All services run independently (no WiFi dependency)
- Q: What format should the WiFi configuration file use? → A: Plain text - Simple line-based format (SSID,password per line)

---

## User Scenarios & Testing

### Primary User Story
As a Poseidon Gateway device, I need to automatically connect to a known WiFi network at startup so that I can communicate with boat instruments and SignalK servers. The gateway should attempt to connect to networks from a predefined list in priority order, and retry indefinitely until successful connection is established.

### Acceptance Scenarios

1. **Given** the gateway has a list of 3 configured WiFi networks, **When** the device boots and the first network is available, **Then** the gateway connects to the first network and proceeds with normal operation.

2. **Given** the gateway has a list of 3 configured WiFi networks, **When** the device boots and the first network is unavailable but the second is available, **Then** the gateway skips the first network and connects to the second network.

3. **Given** the gateway has a list of configured WiFi networks, **When** the device boots and none of the networks are available, **Then** the gateway reboots and retries the connection sequence.

4. **Given** a WiFi network configuration file exists, **When** a user uploads the file to the device, **Then** the configuration is stored persistently and survives device reboots.

5. **Given** the gateway is connected to a WiFi network, **When** the connection is lost, **Then** the gateway retries connecting to the current network with 30-second timeout attempts until successful.

### Edge Cases

- What happens when the WiFi credentials file is missing on first boot?
- What happens when the WiFi credentials file is corrupted or has invalid format (not comma-separated, more than 3 lines)?
- What happens when a line in the configuration file contains invalid characters or exceeds maximum length?
- How does the system handle networks that require additional authentication (captive portals, enterprise WiFi)?
- Should there be a delay between reboot cycles to avoid rapid reboot loops?
- What feedback (visual/network) indicates connection status during the connection process?

## Requirements

### Functional Requirements

- **FR-001**: System MUST read WiFi network credentials from a persistent storage location that survives reboots
- **FR-002**: System MUST support multiple WiFi network configurations (SSID and password pairs)
- **FR-003**: System MUST attempt to connect to WiFi networks in the order they appear in the configuration
- **FR-004**: System MUST skip to the next network if a connection attempt fails within 30 seconds
- **FR-005**: System MUST reboot and retry the entire connection sequence if all configured networks fail to connect
- **FR-006**: System MUST provide a mechanism for users to upload/edit the WiFi configuration file
- **FR-007**: System MUST validate WiFi configuration file format (plain text, comma-separated SSID,password per line, maximum 3 lines) before storing it persistently
- **FR-008**: System MUST indicate WiFi connection status (connected/connecting/failed) to other gateway services
- **FR-009**: System MUST persist WiFi configuration data in a non-volatile storage that survives power cycles
- **FR-010**: System MUST support a maximum of 3 WiFi networks in the configuration
- **FR-011**: WiFi connection attempts MUST NOT block other gateway services from starting and operating independently
- **FR-012**: System MUST retry connection to the current network (using same 30-second timeout) when an established connection is lost, without switching to other configured networks

### Key Entities

- **WiFi Network Configuration**: Collection of up to 3 network credentials (SSID, password) with priority ordering. Stored persistently and editable by users. Contains validation rules for SSID format and password length.

- **Connection State**: Tracks current connection attempt, including which network is being tried, connection status, retry count, and time since last attempt. Used to determine when to move to next network or reboot.

- **Configuration File**: User-editable plain text file containing WiFi network credentials. Format: one network per line with SSID and password separated by comma (e.g., "HomeNetwork,password123"). Maximum 3 lines supported.

---

## Review & Acceptance Checklist

### Content Quality
- [x] No implementation details (languages, frameworks, APIs)
- [x] Focused on user value and business needs
- [x] Written for non-technical stakeholders
- [x] All mandatory sections completed

### Requirement Completeness
- [x] No [NEEDS CLARIFICATION] markers remain in requirements
- [x] Requirements are testable and unambiguous
- [x] Success criteria are measurable (30s timeout, 3 networks max, plain text format)
- [x] Scope is clearly bounded
- [x] Dependencies and assumptions identified (services run independently)

---

## Execution Status

- [x] User description parsed
- [x] Key concepts extracted
- [x] Ambiguities marked
- [x] User scenarios defined
- [x] Requirements generated
- [x] Entities identified
- [x] Review checklist passed (critical ambiguities resolved via clarification session)

---

## Notes for Planning Phase

**Clarifications Deferred to Planning Phase** (low impact or implementation-level):
1. Reboot delay to prevent rapid cycling (default: 5 seconds recommended)
2. Connection status feedback mechanism (LED, OLED display, UDP broadcast?)
3. Handling of missing/corrupted configuration on first boot (factory defaults?)

**Reference Implementation**: `examples/poseidongw/src/main.cpp` lines 1-8 show hardcoded WiFi credentials - this feature will replace that approach with configurable, persistent storage.

**Documentation Requirements** (from user requirements):
- README with setup instructions
- Project structure documentation

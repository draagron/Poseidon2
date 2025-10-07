# Quickstart: Documentation Update Verification

**Feature**: UDP to WebSocket logging documentation cleanup
**Branch**: 004-removal-of-udp
**Purpose**: Verify all documentation updates are complete and accurate

## Overview

This quickstart provides manual verification steps to confirm that all UDP logging references have been replaced with accurate WebSocket logging documentation. Each scenario corresponds to a functional requirement from spec.md.

---

## Prerequisites

- Branch `004-removal-of-udp` checked out
- All documentation update tasks completed (from tasks.md)
- Text editor or IDE for file examination

---

## Verification Scenarios

### Scenario 1: Developer Reads README for Logging Setup
**Requirement**: FR-002 - README.md MUST contain accurate WebSocket logging setup instructions

**Steps**:
1. Open `README.md` in text editor
2. Search for the word "logging" (Ctrl+F or Cmd+F)
3. Verify each match refers to **WebSocket** logging, not UDP
4. Search for "UDP" (case-insensitive)
5. Verify no UDP references remain (except possible "legacy" context)
6. Locate the "Logging" or "Debugging" section
7. Verify it contains:
   - WebSocket endpoint: `ws://<device-ip>/logs`
   - Python client usage: `python3 ws_logger.py <ESP32_IP>`
   - Log level filtering option: `--filter LEVEL`
   - Auto-reconnect option: `--reconnect`

**Expected Results**:
- ✅ No obsolete UDP logging references (port 4444, nc -ul, UDP broadcast)
- ✅ WebSocket logging clearly documented with endpoint and client usage
- ✅ Troubleshooting section references WebSocket logs, not UDP logs

**Failure Criteria**:
- ❌ Any reference to "UDP port 4444" without "legacy" context
- ❌ Instructions to use `nc -ul 4444` or `socat UDP-LISTEN:4444`
- ❌ Missing WebSocket endpoint documentation

---

### Scenario 2: Developer Troubleshoots Connection Issues
**Requirement**: FR-004 - Troubleshooting guides MUST instruct users to monitor WebSocket logs

**Steps**:
1. Open `README.md` in text editor
2. Navigate to "Troubleshooting" section (typically near end of file)
3. Read through all troubleshooting scenarios
4. Verify logging instructions reference WebSocket logging
5. Open `CLAUDE.md` in text editor
6. Navigate to "Troubleshooting Common Issues" section
7. Verify all debug instructions use WebSocket logging

**Expected Results**:
- ✅ README troubleshooting uses `python3 ws_logger.py <ip>` for log monitoring
- ✅ CLAUDE troubleshooting uses `python3 ws_logger.py <ip>` for log monitoring
- ✅ No instructions to use `nc -ul 4444` or similar UDP listeners
- ✅ Firewall troubleshooting mentions WebSocket/HTTP port (80), not UDP port 4444

**Failure Criteria**:
- ❌ Any troubleshooting step instructs "monitor UDP logs"
- ❌ Firewall section mentions "UDP port 4444"
- ❌ Missing WebSocket client setup instructions

---

### Scenario 3: Developer Reviews Project Architecture
**Requirement**: FR-003 - CLAUDE.md MUST reference WebSocketLogger utility

**Steps**:
1. Open `CLAUDE.md` in text editor
2. Navigate to "Project Structure" section (typically lines 260-285)
3. Locate the `src/utils/` directory description
4. Verify `WebSocketLogger` is listed (not `UDPLogger`)
5. Navigate to "Key Implementation Patterns" section
6. Verify logging examples use `WebSocketLogger` class
7. Search for "UDPLogger" (case-insensitive)
8. Verify no references exist (except possible historical context)

**Expected Results**:
- ✅ Project structure lists `WebSocketLogger.cpp` and `WebSocketLogger.h`
- ✅ No reference to `UDPLogger` in utils/ directory
- ✅ Code examples demonstrate WebSocketLogger usage
- ✅ Logging patterns section describes WebSocket integration

**Failure Criteria**:
- ❌ Project structure lists `UDPLogger.cpp` or `UDPLogger.h`
- ❌ Code examples use UDP broadcast methods
- ❌ Architecture diagrams show UDP logging flow

---

### Scenario 4: Developer Implements Logging in New Feature
**Requirement**: FR-007 - Constitutional requirements MUST reflect WebSocket logging

**Steps**:
1. Open `.specify/memory/constitution.md` in text editor
2. Navigate to "Principle V: Network-Based Debugging" (typically lines 92-100)
3. Read the principle description and bullet points
4. Verify it prescribes **WebSocket** logging, not UDP
5. Check for specific mentions of:
   - WebSocket endpoint (`ws://<device-ip>/logs`)
   - TCP-based protocol for reliability
   - No packet loss guarantee (vs. UDP)
6. Verify constitution version is v1.2.0 or higher (header metadata)
7. Check SYNC IMPACT REPORT comment at top of file
8. Verify Principle V amendment is documented

**Expected Results**:
- ✅ Principle V title: "Network-Based Debugging" (unchanged)
- ✅ Principle V body prescribes WebSocket logging (not UDP)
- ✅ Bullet points mention WebSocket endpoint and TCP reliability
- ✅ Constitution version bumped to v1.2.0
- ✅ SYNC IMPACT REPORT documents Principle V amendment

**Failure Criteria**:
- ❌ Principle V still prescribes "UDP broadcast logging"
- ❌ No mention of WebSocket or TCP reliability
- ❌ Constitution version remains v1.1.0 (not bumped)
- ❌ SYNC IMPACT REPORT missing or doesn't mention Principle V

---

### Scenario 5: Developer Reads Historical Spec Files
**Requirement**: FR-006 - Historical spec files MUST be clearly marked as "legacy"

**Steps**:
1. Open `specs/001-create-feature-spec/plan.md`
2. Check the **first 20 lines** of the file (before ## Summary)
3. Verify presence of "LEGACY IMPLEMENTATION" disclaimer
4. Verify disclaimer explains UDP → WebSocket migration
5. Verify disclaimer links to current documentation (README.md, CLAUDE.md)
6. Repeat for other 001-series spec files:
   - `specs/001-create-feature-spec/quickstart.md`
   - `specs/001-create-feature-spec/research.md`
   - `specs/001-create-feature-spec/spec.md`
   - `specs/001-create-feature-spec/tasks.md`
7. Check `specs/001-create-feature-spec/contracts/wifi-config-api.md`

**Expected Results**:
- ✅ All 001-series spec files have prominent disclaimer header
- ✅ Disclaimer clearly states "LEGACY IMPLEMENTATION"
- ✅ Disclaimer explains current implementation uses WebSocket
- ✅ Disclaimer links to README.md and/or CLAUDE.md for current docs
- ✅ Disclaimer is **before** the first ## heading (maximum visibility)

**Failure Criteria**:
- ❌ Any 001-series spec file missing disclaimer
- ❌ Disclaimer hidden at bottom of file or in middle of content
- ❌ Disclaimer doesn't mention WebSocket as current implementation
- ❌ Disclaimer doesn't link to current documentation

---

### Scenario 6: Configuration File Review
**Requirement**: FR-005 - All configuration examples MUST demonstrate WebSocket logging patterns

**Steps**:
1. Open `src/config.h` in text editor
2. Locate the `UDP_DEBUG_PORT` constant (typically line 18)
3. Verify one of the following:
   - **Option A**: Constant is removed entirely
   - **Option B**: Constant has comment indicating "legacy, unused" or "for reference only"
   - **Option C**: Constant is renamed to indicate WebSocket logging
4. Search for any other UDP-related constants
5. Verify no new code references UDP_DEBUG_PORT

**Expected Results**:
- ✅ UDP_DEBUG_PORT constant either removed or marked as legacy/unused
- ✅ If retained, comment clearly indicates it's not used in current implementation
- ✅ No other UDP-related constants present
- ✅ Configuration comments reference WebSocket logging where applicable

**Failure Criteria**:
- ❌ UDP_DEBUG_PORT constant present without clarifying comment
- ❌ Comments suggest UDP_DEBUG_PORT is actively used
- ❌ Other UDP-related constants added without context

---

### Scenario 7: Cross-Documentation Consistency
**Requirement**: FR-008 - System MUST maintain consistency across all documentation files

**Steps**:
1. Create a checklist for key logging references:
   - [ ] README.md mentions WebSocket logging
   - [ ] CLAUDE.md mentions WebSocket logging
   - [ ] constitution.md Principle V prescribes WebSocket logging
   - [ ] src/helpers/README.md documents ws_logger.py
   - [ ] All three agree on endpoint: `ws://<device-ip>/logs`
   - [ ] All three agree on client: `python3 ws_logger.py <ip>`
2. Search each file for endpoint documentation
3. Verify endpoint format is consistent: `ws://<device-ip>/logs`
4. Verify client invocation is consistent: `python3 ws_logger.py <ESP32_IP>`
5. Verify log levels are consistent: DEBUG, INFO, WARN, ERROR, FATAL

**Expected Results**:
- ✅ All documentation files agree on WebSocket endpoint format
- ✅ All documentation files agree on Python client usage
- ✅ Log level enumerations consistent across documentation
- ✅ No contradictory information about logging mechanism

**Failure Criteria**:
- ❌ Different endpoint formats in different files (e.g., `/logs` vs `/websocket/logs`)
- ❌ Different client invocation patterns
- ❌ Contradictory statements about logging reliability or mechanism

---

## Success Criteria

✅ **All 7 scenarios pass** - Documentation is accurate and consistent

## Failure Recovery

If any scenario fails:

1. **Identify the specific file and section** that failed
2. **Refer to tasks.md** for the task that was supposed to update that section
3. **Re-execute the failed task**
4. **Re-run the verification scenario**
5. **If failure persists**, consult plan.md Phase 1 design for correct update approach

---

## Post-Verification Actions

Once all scenarios pass:

1. **Commit the changes**:
   ```bash
   git add README.md CLAUDE.md .specify/memory/constitution.md src/config.h specs/001-*
   git commit -m "docs: update logging documentation to reflect WebSocket implementation

   - Update README.md and CLAUDE.md to document WebSocket logging
   - Amend constitution.md Principle V to prescribe WebSocket logging (v1.2.0)
   - Add LEGACY IMPLEMENTATION disclaimers to specs/001-* files
   - Update/clarify UDP_DEBUG_PORT constant in src/config.h

   Closes #[issue-number]"
   ```

2. **Create pull request** (optional, if using feature branches)
3. **Request QA review** for documentation accuracy

---

## Troubleshooting

### Problem: Can't find WebSocket logging documentation in README

**Solution**: Check if README.md was updated correctly. Should have section titled "WebSocket Logging" or "Network Debugging" that describes ws_logger.py usage.

### Problem: Constitution still shows UDP logging

**Solution**: Verify you're on branch `004-removal-of-udp`. Run `git status` to confirm. If on wrong branch, checkout correct branch and re-run verification.

### Problem: Legacy disclaimers not visible in spec files

**Solution**: Check you're reading the **top** of the file (first 20 lines). Disclaimer should be prominently placed before first ## heading.

---

## Related Documentation

- [Feature Specification](./spec.md) - Requirements and acceptance criteria
- [Implementation Plan](./plan.md) - Technical approach and design decisions
- [Research Notes](./research.md) - Current implementation analysis and decision rationale
- [Task List](./tasks.md) - Ordered implementation tasks (created by /tasks command)

---

**Last Updated**: 2025-10-07
**Status**: Ready for verification after task execution

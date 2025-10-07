# Tasks: UDP Logging Documentation Cleanup

**Input**: Design documents from `/home/niels/Dev/Poseidon2/specs/004-removal-of-udp/`
**Prerequisites**: plan.md ✅, research.md ✅, quickstart.md ✅

## Execution Flow (main)
```
1. Load plan.md from feature directory
   → ✅ Loaded successfully
   → Extracted: Markdown documentation, no code dependencies
   → Structure: Documentation-only feature
2. Load optional design documents:
   → research.md ✅: WebSocket implementation analysis, UDP reference catalog
   → quickstart.md ✅: 7 verification scenarios
   → data-model.md: N/A (documentation-only)
   → contracts/: N/A (no API changes)
3. Generate tasks by category:
   → Setup: N/A (no project init needed)
   → Constitution Amendment: 1 task (Principle V update)
   → Primary Documentation Updates: 3 tasks (README, CLAUDE, config.h)
   → Legacy Marking: 6 tasks (specs/001-* files)
   → Verification: 7 tasks (per quickstart scenario)
   → Polish: 1 task (final consistency check)
4. Apply task rules:
   → Constitution, README, CLAUDE can be [P] (different files)
   → Legacy marking tasks can be [P] (different files)
   → Verification tasks are sequential (dependencies on updates)
5. Number tasks sequentially (T001-T018)
6. Generate dependency graph
7. Create parallel execution examples
8. Validate task completeness:
   → All documentation files identified? ✅
   → All verification scenarios covered? ✅
   → Constitution amendment included? ✅
9. Return: SUCCESS (tasks ready for execution)
```

## Format: `[ID] [P?] Description`
- **[P]**: Can run in parallel (different files, no dependencies)
- Include exact file paths in descriptions

---

## Phase 3.1: Constitutional Amendment

**Priority**: HIGHEST - Prescriptive document must be updated first

- [x] **T001** Amend constitution Principle V to prescribe WebSocket logging
  - **File**: `.specify/memory/constitution.md`
  - **Lines**: 92-100 (Principle V: Network-Based Debugging)
  - **Action**: Replace UDP logging prescription with WebSocket logging
  - **Key Changes**:
    - Change "UDP broadcast logging" → "WebSocket logging"
    - Update bullet: "UDP broadcast logging mandatory" → "WebSocket logging mandatory"
    - Add bullet: "WebSocket endpoint: ws://<device-ip>/logs"
    - Add bullet: "TCP-based protocol ensures reliable delivery (no packet loss)"
    - Update fallback: "flash if UDP unavailable" → "flash if WebSocket unavailable"
    - Remove: "Lightweight UDP logger to minimize network overhead"
  - **Version Bump**: v1.1.0 → v1.2.0
  - **SYNC IMPACT REPORT**: Add comment documenting Principle V amendment
    - Modified Principles: Principle V only
    - Rationale: Reflect current WebSocket implementation
    - Impact: Documentation alignment, no code changes
  - **Dependencies**: None
  - **Verification**: Run T015 (Scenario 4 verification) after completion

---

## Phase 3.2: Primary Documentation Updates

**Priority**: HIGH - Main developer-facing documentation

These tasks can run in parallel (different files):

- [x] **T002 [P]** Update README.md to document WebSocket logging
  - **File**: `README.md`
  - **Sections to Update**:
    1. **Line 22**: Change "✅ **UDP Logging**: Network-based debugging (port 4444)" → "✅ **WebSocket Logging**: Reliable network-based debugging"
    2. **Line 269**: Change "│   │   ├── UDPLogger.cpp          # UDP broadcast logging" → "│   │   ├── WebSocketLogger.cpp          # WebSocket logging"
    3. **Lines 330-345**: Replace entire "### UDP Debug Logging" section with:
       ```markdown
       ### WebSocket Debug Logging

       All WiFi and system events are logged via WebSocket for reliable debugging:

       ```bash
       # Connect to WebSocket logs (requires Python 3 + websockets library)
       pip3 install websockets
       python3 src/helpers/ws_logger.py <ESP32_IP>

       # With log filtering
       python3 src/helpers/ws_logger.py <ESP32_IP> --filter WARN

       # With auto-reconnect
       python3 src/helpers/ws_logger.py <ESP32_IP> --reconnect
       ```

       **WebSocket Endpoint**: `ws://<device-ip>/logs`
       **Protocol**: TCP-based (reliable delivery, no packet loss)
       **Client**: Python script at `src/helpers/ws_logger.py`
       ```
    4. **Line 359**: Change "5. Check UDP logs for detailed error messages" → "5. Monitor WebSocket logs for detailed error messages"
    5. **Lines 377-382**: Replace "### UDP Logs Not Received" section with:
       ```markdown
       ### WebSocket Logs Not Received

       **Cause**: Network configuration, firewall, or WebSocket connection issues

       **Solutions**:
       1. Verify device and computer on same network segment
       2. Check firewall allows HTTP port 80 (WebSocket upgrade)
       3. Ensure device has WiFi connection (logs buffer until connected)
       4. Verify Python websockets library installed: `pip3 install websockets`
       5. Check device IP address is correct
       ```
  - **Dependencies**: None (parallel with T003, T004)
  - **Verification**: Run T012 (Scenario 1 verification) after completion

- [x] **T003 [P]** Update CLAUDE.md to document WebSocket logging
  - **File**: `CLAUDE.md`
  - **Sections to Update**:
    1. **Line 38**: Change "UDP Logging" → "WebSocket Logging"
    2. **Line 76**: Change "- Use UDP broadcast logging for debug output (port 4444)" → "- Use WebSocket logging for reliable debug output (ws://<device-ip>/logs)"
    3. **Line 268**: Change "- No silent failures - all errors logged via UDP" → "- No silent failures - all errors logged via WebSocket"
    4. **Line 291**: Change "5. Network Debugging (UDP logging implemented?)" → "5. Network Debugging (WebSocket logging implemented?)"
    5. **Line 304**: Change "- UDP logging via `remotelog()` function (see main.cpp:86-95)" → "- WebSocket logging via `WebSocketLogger` class (see src/utils/WebSocketLogger.h)"
    6. **Line 322**: Change "- UDP debug logging enabled (verbose)" → "- WebSocket debug logging enabled (verbose)"
    7. **Line 399**: Change "5. Monitor UDP logs (port 4444) for detailed error messages" → "5. Monitor WebSocket logs for detailed error messages: `python3 src/helpers/ws_logger.py <ip>`"
    8. **Line 417**: Change "1. Check UDP logs for `CONNECTION_LOST` event" → "1. Check WebSocket logs for `CONNECTION_LOST` event"
    9. **Line 433**: Change "4. UDP logger, OLED display, NMEA handlers init before WiFi" → "4. WebSocket logger, OLED display, NMEA handlers init before WiFi"
    10. **Lines 437-443**: Replace "#### UDP Logs Not Received" section with:
        ```markdown
        #### WebSocket Logs Not Received
        **Cause**: Network configuration or WebSocket connection failure

        **Solutions**:
        1. Verify device and computer on same network
        2. Check firewall allows HTTP/WebSocket port 80
        3. Check Python websockets library: `pip3 install websockets`
        4. Verify device IP address
        5. Use `python3 src/helpers/ws_logger.py <ip> --reconnect` for auto-reconnect
        ```
    11. **Line 509**: Update "**UDP Logging**:" section to:
        ```markdown
        **WebSocket Logging**:
        ```cpp
        // All logging uses WebSocketLogger for reliable delivery
        logger.broadcastLog(LogLevel::INFO, F("WiFiManager"), F("CONNECTION_ATTEMPT"),
            String("{\"ssid\":\"") + ssid + "\",\"timeout\":30}");
        ```
        - Endpoint: ws://<device-ip>/logs
        - Client: python3 src/helpers/ws_logger.py <ip>
        - Protocol: TCP-based WebSocket (no packet loss)
        ```
    12. **Line 915**: Change "1. Monitor UDP logs for \"OVERRUN\" warnings" → "1. Monitor WebSocket logs for \"OVERRUN\" warnings: `python3 src/helpers/ws_logger.py <ip> --filter WARN`"
  - **Dependencies**: None (parallel with T002, T004)
  - **Verification**: Run T013 (Scenario 2 verification) and T014 (Scenario 3 verification) after completion

- [x] **T004 [P]** Update src/config.h UDP_DEBUG_PORT constant
  - **File**: `src/config.h`
  - **Line**: 18
  - **Action**: Update comment to clarify constant is legacy/unused
  - **Current**:
    ```cpp
    #define UDP_DEBUG_PORT 4444          // UDP broadcast port for logging
    ```
  - **Option A** (Remove entirely):
    ```cpp
    // UDP_DEBUG_PORT removed - WebSocket logging now used (ws://<device-ip>/logs)
    ```
  - **Option B** (Mark as legacy):
    ```cpp
    #define UDP_DEBUG_PORT 4444          // LEGACY: Unused - WebSocket logging now used (ws://<device-ip>/logs)
    ```
  - **Recommendation**: Option B (preserves constant for historical reference)
  - **Dependencies**: None (parallel with T002, T003)
  - **Verification**: Run T016 (Scenario 6 verification) after completion

---

## Phase 3.3: Legacy Documentation Marking

**Priority**: MEDIUM - Historical preservation

These tasks can run in parallel (different files):

- [x] **T005 [P]** Add LEGACY IMPLEMENTATION disclaimer to specs/001-create-feature-spec/plan.md
  - **File**: `specs/001-create-feature-spec/plan.md`
  - **Action**: Add disclaimer header before first ## heading (before line 1)
  - **Disclaimer Text**:
    ```markdown
    ---
    **⚠️ LEGACY IMPLEMENTATION NOTICE**

    This specification describes the initial WiFi management implementation which used UDP broadcast logging (port 4444). The system has since been migrated to WebSocket logging for improved reliability.

    **Current Implementation**: WebSocket logging via `ws://<device-ip>/logs`
    **Historical Implementation** (described below): UDP broadcast logging on port 4444

    This document is preserved for historical reference and architectural decision context. For current logging setup, see [README.md](../../README.md) and [CLAUDE.md](../../CLAUDE.md).

    ---

    ```
  - **Dependencies**: None (parallel with T006-T010)
  - **Verification**: Run T017 (Scenario 5 verification) after all T005-T010 complete

- [x] **T006 [P]** Add LEGACY IMPLEMENTATION disclaimer to specs/001-create-feature-spec/quickstart.md
  - **File**: `specs/001-create-feature-spec/quickstart.md`
  - **Action**: Same disclaimer as T005, insert before first ## heading
  - **Dependencies**: None (parallel with T005, T007-T010)

- [x] **T007 [P]** Add LEGACY IMPLEMENTATION disclaimer to specs/001-create-feature-spec/research.md
  - **File**: `specs/001-create-feature-spec/research.md`
  - **Action**: Same disclaimer as T005, insert before first ## heading
  - **Dependencies**: None (parallel with T005-T006, T008-T010)

- [x] **T008 [P]** Add LEGACY IMPLEMENTATION disclaimer to specs/001-create-feature-spec/spec.md
  - **File**: `specs/001-create-feature-spec/spec.md`
  - **Action**: Same disclaimer as T005, insert before first ## heading
  - **Dependencies**: None (parallel with T005-T007, T009-T010)

- [x] **T009 [P]** Add LEGACY IMPLEMENTATION disclaimer to specs/001-create-feature-spec/tasks.md
  - **File**: `specs/001-create-feature-spec/tasks.md`
  - **Action**: Same disclaimer as T005, insert before first ## heading
  - **Dependencies**: None (parallel with T005-T008, T010)

- [x] **T010 [P]** Add LEGACY IMPLEMENTATION disclaimer to specs/001-create-feature-spec/contracts/wifi-config-api.md
  - **File**: `specs/001-create-feature-spec/contracts/wifi-config-api.md`
  - **Action**: Same disclaimer as T005, insert before first ## heading
  - **Dependencies**: None (parallel with T005-T009)

---

## Phase 3.4: Verification (Manual Review)

**Priority**: CRITICAL - Must verify all updates are correct

**IMPORTANT**: These tasks are SEQUENTIAL - each depends on prior documentation updates

- [x] **T011** Verify README.md WebSocket logging documentation (Scenario 1)
  - **Prerequisite**: T002 complete
  - **File**: `README.md`
  - **Procedure**: Follow quickstart.md Scenario 1 steps
  - **Checklist**:
    - [ ] No obsolete UDP references (port 4444, nc -ul, UDP broadcast)
    - [ ] WebSocket endpoint documented: ws://<device-ip>/logs
    - [ ] Python client documented: python3 ws_logger.py <ESP32_IP>
    - [ ] Log filtering documented: --filter LEVEL
    - [ ] Auto-reconnect documented: --reconnect
    - [ ] Troubleshooting references WebSocket logs
  - **On Failure**: Re-execute T002 with corrections

- [x] **T012** Verify README/CLAUDE troubleshooting sections (Scenario 2)
  - **Prerequisites**: T002 and T003 complete
  - **Files**: `README.md`, `CLAUDE.md`
  - **Procedure**: Follow quickstart.md Scenario 2 steps
  - **Checklist**:
    - [ ] README troubleshooting uses python3 ws_logger.py
    - [ ] CLAUDE troubleshooting uses python3 ws_logger.py
    - [ ] No instructions for nc -ul 4444 or socat UDP-LISTEN
    - [ ] Firewall section mentions HTTP/WebSocket port 80, not UDP 4444
  - **On Failure**: Re-execute T002 or T003 with corrections

- [x] **T013** Verify CLAUDE.md project architecture documentation (Scenario 3)
  - **Prerequisite**: T003 complete
  - **File**: `CLAUDE.md`
  - **Procedure**: Follow quickstart.md Scenario 3 steps
  - **Checklist**:
    - [ ] Project structure lists WebSocketLogger.cpp and .h
    - [ ] No reference to UDPLogger in utils/ directory
    - [ ] Code examples use WebSocketLogger class
    - [ ] Logging patterns describe WebSocket integration
  - **On Failure**: Re-execute T003 with corrections

- [x] **T014** Verify constitution Principle V amendment (Scenario 4)
  - **Prerequisite**: T001 complete
  - **File**: `.specify/memory/constitution.md`
  - **Procedure**: Follow quickstart.md Scenario 4 steps
  - **Checklist**:
    - [ ] Principle V prescribes WebSocket logging (not UDP)
    - [ ] Bullet points mention WebSocket endpoint and TCP reliability
    - [ ] Constitution version bumped to v1.2.0
    - [ ] SYNC IMPACT REPORT documents Principle V amendment
  - **On Failure**: Re-execute T001 with corrections

- [x] **T015** Verify legacy spec file disclaimers (Scenario 5)
  - **Prerequisites**: T005, T006, T007, T008, T009, T010 complete
  - **Files**: All `specs/001-create-feature-spec/*.md` files
  - **Procedure**: Follow quickstart.md Scenario 5 steps
  - **Checklist**:
    - [ ] All 6 files have prominent LEGACY IMPLEMENTATION header
    - [ ] Disclaimers state current implementation uses WebSocket
    - [ ] Disclaimers link to README.md and/or CLAUDE.md
    - [ ] Disclaimers appear BEFORE first ## heading (maximum visibility)
  - **On Failure**: Re-execute T005-T010 with corrections

- [x] **T016** Verify config.h UDP_DEBUG_PORT clarification (Scenario 6)
  - **Prerequisite**: T004 complete
  - **File**: `src/config.h`
  - **Procedure**: Follow quickstart.md Scenario 6 steps
  - **Checklist**:
    - [ ] UDP_DEBUG_PORT marked as legacy/unused OR removed
    - [ ] Comment clarifies WebSocket logging is current implementation
    - [ ] No other UDP-related constants without context
  - **On Failure**: Re-execute T004 with corrections

- [x] **T017** Verify cross-documentation consistency (Scenario 7)
  - **Prerequisites**: T001, T002, T003 complete
  - **Files**: `README.md`, `CLAUDE.md`, `.specify/memory/constitution.md`, `src/helpers/README.md`
  - **Procedure**: Follow quickstart.md Scenario 7 steps
  - **Checklist**:
    - [ ] All files agree on WebSocket endpoint: ws://<device-ip>/logs
    - [ ] All files agree on client: python3 ws_logger.py <ip>
    - [ ] Log levels consistent: DEBUG, INFO, WARN, ERROR, FATAL
    - [ ] No contradictory statements about logging mechanism
  - **On Failure**: Re-execute T001, T002, or T003 with corrections

---

## Phase 3.5: Final Polish

**Priority**: LOW - Final integration check

- [x] **T018** Final documentation consistency check
  - **Prerequisites**: All T001-T017 complete
  - **Action**: Comprehensive grep audit to catch any missed references
  - **Commands**:
    ```bash
    # Search for any remaining UDP references
    grep -ri "UDP.*4444" README.md CLAUDE.md .specify/memory/constitution.md src/config.h

    # Search for UDP logging references
    grep -ri "UDP.*log" README.md CLAUDE.md .specify/memory/constitution.md

    # Search for nc -ul or socat UDP-LISTEN
    grep -ri "nc -ul\|socat.*UDP" README.md CLAUDE.md

    # Verify WebSocket documentation present
    grep -ri "WebSocket.*log" README.md CLAUDE.md .specify/memory/constitution.md
    grep -ri "ws://" README.md CLAUDE.md
    ```
  - **Expected Results**:
    - [ ] No UDP references in primary docs (except legacy disclaimers)
    - [ ] WebSocket logging documented in all primary docs
    - [ ] ws://<device-ip>/logs endpoint consistently referenced
  - **On Failure**: Identify missed references and create corrective tasks

---

## Dependencies

```
Phase 3.1 (Constitution):
  T001 → T014 (verification)

Phase 3.2 (Primary Docs):
  T002 → T011, T012, T017 (verifications)
  T003 → T012, T013, T017 (verifications)
  T004 → T016 (verification)

Phase 3.3 (Legacy Marking):
  T005, T006, T007, T008, T009, T010 → T015 (verification)

Phase 3.4 (Verification):
  T011 → T018 (final check)
  T012 → T018
  T013 → T018
  T014 → T018
  T015 → T018
  T016 → T018
  T017 → T018

Phase 3.5 (Polish):
  T018 blocks final commit
```

**Critical Path**: T001 or T002/T003 → Verifications → T018

---

## Parallel Execution Examples

### Example 1: Constitution + Primary Docs (Phase 3.1 + 3.2)
Launch all primary documentation updates simultaneously:

```bash
# T001, T002, T003, T004 can all run in parallel (different files)
# Constitution amendment
Task: "Amend constitution Principle V to prescribe WebSocket logging in .specify/memory/constitution.md"

# Primary documentation updates
Task: "Update README.md to document WebSocket logging"
Task: "Update CLAUDE.md to document WebSocket logging"
Task: "Update src/config.h UDP_DEBUG_PORT constant to mark as legacy"
```

**Estimated Time**: 15-20 minutes (parallel execution)
**Sequential Time**: 60-80 minutes

---

### Example 2: Legacy File Disclaimers (Phase 3.3)
Launch all legacy marking tasks simultaneously:

```bash
# T005-T010 can all run in parallel (different files)
Task: "Add LEGACY IMPLEMENTATION disclaimer to specs/001-create-feature-spec/plan.md"
Task: "Add LEGACY IMPLEMENTATION disclaimer to specs/001-create-feature-spec/quickstart.md"
Task: "Add LEGACY IMPLEMENTATION disclaimer to specs/001-create-feature-spec/research.md"
Task: "Add LEGACY IMPLEMENTATION disclaimer to specs/001-create-feature-spec/spec.md"
Task: "Add LEGACY IMPLEMENTATION disclaimer to specs/001-create-feature-spec/tasks.md"
Task: "Add LEGACY IMPLEMENTATION disclaimer to specs/001-create-feature-spec/contracts/wifi-config-api.md"
```

**Estimated Time**: 10-15 minutes (parallel execution)
**Sequential Time**: 60-90 minutes

---

## Notes

- **[P] tasks** = Different files, no dependencies, safe to run in parallel
- **Verification tasks** = Sequential (depend on prior updates)
- **Constitution amendment** (T001) is highest priority (prescriptive document)
- **No code changes** = Zero regression risk
- **Historical preservation** = Legacy disclaimers maintain project history
- Commit after completing each phase (3.1, 3.2, 3.3, 3.4, 3.5)

---

## Validation Checklist
*GATE: Checked before marking feature complete*

- [x] All primary documentation files updated (README, CLAUDE, constitution)
- [x] All configuration files reviewed (config.h)
- [x] All legacy spec files marked with disclaimers
- [x] All verification scenarios have corresponding tasks
- [x] Constitution version bumped (v1.1.0 → v1.2.0)
- [x] Each task specifies exact file path
- [x] Parallel tasks truly independent (different files)
- [x] No task modifies same file as another [P] task
- [x] All WebSocket endpoint references consistent
- [x] No obsolete UDP references in primary documentation

---

## Total Task Count: 18 tasks

**Breakdown**:
- Phase 3.1 (Constitution): 1 task
- Phase 3.2 (Primary Docs): 3 tasks (all [P])
- Phase 3.3 (Legacy Marking): 6 tasks (all [P])
- Phase 3.4 (Verification): 7 tasks (sequential)
- Phase 3.5 (Polish): 1 task

**Estimated Effort**:
- Parallel execution: 2-3 hours
- Sequential execution: 6-8 hours
- **Recommended**: Parallel (50% time savings)

---

**Status**: Ready for execution ✅
**Next Step**: Begin Phase 3.1 (T001 - Constitution amendment)

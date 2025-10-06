# Gemini Code-along Agent Context

This document provides context for the Gemini code-along agent to understand and assist with the development of the Poseidon2 project.

## Project Overview

This repository contains the source code for PoseidonGW, an ESP32-based gateway for marine electronics. The project is built using the PlatformIO IDE and the Arduino framework.

The main application is located in the `examples/poseidongw` directory. It utilizes several libraries to handle NMEA 0183 and NMEA 2000 data, and provides a web interface for user interaction via an ESPAsyncWebServer.

The project is structured as follows:

*   `src/`: Main source code for the ESP32 gateway.
*   `include/`: Shared header files.
*   `lib/`: Project-specific libraries.
*   `test/`: Unit tests.
*   `scripts/`: Utility scripts, such as a UDP logger for debugging.
*   `user_requirements/`: Project requirements.
*   `specs/`: Project specifications.

## Building and Running

All commands should be executed from the `examples/poseidongw` directory.

*   **Build:** `platformio run`
*   **Upload:** `platformio run --target upload`
*   **Monitor:** `platformio device monitor`
*   **Test:** `platformio test --environment esp32dev`

## Development Conventions

*   **Coding Style:** The project follows the Arduino coding style:
    *   2-space indentation.
    *   Opening braces on their own line for functions.
    *   Inline braces for short control blocks.
    *   `SCREAMING_SNAKE_CASE` for constants and macros.
    *   `PascalCase` for types.
    *   `lowerCamelCase` for functions.
*   **Testing:** Unit tests are written using the Unity framework and are located in the `test/` directory. Tests should be run before submitting pull requests.
*   **Commits and Pull Requests:**
    *   Commit messages should follow the Conventional Commits specification (e.g., `feat:`, `fix:`, `docs:`).
    *   Pull requests must link to the relevant user requirement and include a summary of the tests that were run.
*   **Configuration:** Do not commit Wi-Fi credentials or other secrets. Use placeholders and explain how to override them in the pull request notes.

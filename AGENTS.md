# Repository Guidelines

## Project Structure & Module Organization
Use `examples/poseidongw/` for the ESP32 gateway firmware; place C++ sources in `src/`, shared headers in `include/`, and keep hardware-specific code scoped by module. Utility helpers belong in `examples/poseidongw/scripts/` (Python 3). Prototype math experiments live under `examples/Calculations/` with a short annotation describing the supported experiment. Link significant changes back to the matching entry in `user_requirements/` to preserve traceability.

## Build, Test, and Development Commands
- `cd examples/poseidongw && platformio run` compiles the default `esp32dev` target defined in `platformio.ini`.
- `cd examples/poseidongw && platformio run --target upload` flashes a connected board; override `upload_port` locally if needed.
- `cd examples/poseidongw && platformio device monitor` opens a 115200 baud serial console for runtime logs.
- `python examples/poseidongw/scripts/udp_logger.py` mirrors the UDP debug feed when validating network output.

## Coding Style & Naming Conventions
Follow the Arduino style already in `src/main.cpp`: 2-space indents, function braces on their own line, inline braces for brief control blocks. Favor `constexpr` for configuration values. Use `SCREAMING_SNAKE_CASE` for constants/macros, `PascalCase` for types, and `lowerCamelCase` for functions. Collect pin maps and configuration constants near the top of each translation unit.

## Testing Guidelines
Place Unity tests under `examples/poseidongw/test/` with filenames such as `test_serial.cpp`. Expose seam-friendly abstractions so hardware interactions can be faked. Before submitting changes, run `cd examples/poseidongw && platformio test --environment esp32dev` and record the summary in your PR.

## Commit & Pull Request Guidelines
Adopt Conventional Commit prefixes (`feat:`, `fix:`, `docs:`) with subjects under 72 characters. Describe hardware assumptions, wiring changes, and protocol impacts in the body. PRs must link to the relevant `user_requirements` item, note build/test commands executed, and include serial or UDP snippets if user-visible output changes. Call out any secrets or environment variables contributors must supply to reproduce results.

## Security & Configuration Tips
Never commit real Wi-Fi credentials—use placeholders such as `wifi_ssid` and `wifi_pwd`. Document new defaults or wiring updates in `examples/poseidongw/README.md` so integrators can mirror the environment safely.

# Repository Guidelines

## Project Structure & Module Organization
- `examples/poseidongw/` hosts the ESP32 gateway; keep firmware in `src/`, shared headers in `include/`, and hardware-specific code isolated per module.
- Utility scripts such as `scripts/udp_logger.py` belong in `examples/poseidongw/scripts/`; keep them Python 3 friendly and document CLI usage inline.
- Use `examples/Calculations/` for small math prototypes and annotate each file with the experiment it supports.
- Link features back to `user_requirements/` entries to maintain traceability in docs and PRs.

## Build, Test & Development Commands
- `cd examples/poseidongw && platformio run` compiles the default `esp32dev` target defined in `platformio.ini`.
- `cd examples/poseidongw && platformio run --target upload` flashes a connected board; set `upload_port` via a local `.ini` override.
- `cd examples/poseidongw && platformio device monitor` opens a 115200 baud console for runtime logs.
- `python examples/poseidongw/scripts/udp_logger.py` mirrors the UDP debug feed when validating network output.

## Coding Style & Naming Conventions
- Match the existing Arduino style: 2-space indents, opening braces on their own line for functions, inline braces for short control blocks.
- Use `SCREAMING_SNAKE_CASE` for constants/macros, `PascalCase` for types, and `lowerCamelCase` for functions, following `main.cpp`.
- Collect pin maps and configuration constants near the top of each translation unit and prefer `constexpr` over new `#define`s.

## Testing Guidelines
- Add Unity tests under `examples/poseidongw/test/` with filenames like `test_serial.cpp` that mirror the module name.
- Run `cd examples/poseidongw && platformio test --environment esp32dev` before opening a PR; capture the summary in your submission.
- Hardware-facing code should expose seam-friendly wrappers so tests can inject fakes and stay device-independent.

## Commit & Pull Request Guidelines
- Follow Conventional Commit prefixes (`feat:`, `fix:`, `docs:`) and keep subject lines under 72 characters, as in the current `git log`.
- Summarize hardware assumptions, wiring tweaks, and protocol impacts in commit bodies or PR descriptions.
- PRs must link to the relevant `user_requirements` item, note test commands run, and attach serial or UDP snippets when user-visible output changes.
- Call out any secrets or environment variables contributors must supply locally to reproduce results.

## Documentation & Configuration Tips
- Do not commit real Wi-Fi credentials; leave placeholders (`wifi_ssid`, `wifi_pwd`) and explain overrides in PR notes.
- Record new configuration defaults or wiring updates in `examples/poseidongw/README.md` to help downstream integrators.

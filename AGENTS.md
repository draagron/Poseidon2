# Repository Guidelines

## Project Structure & Module Organization
- Firmware lives in `src/`; keep the Arduino entry in `main.cpp` and group modules under `components/`, `hal/`, `helpers/`, `types/`, and `utils/`.
- Place mocks in `src/mocks/` and persist LittleFS payloads or calibration data in `data/`.
- Reserve `examples/poseidongw/` for board prototypes and annotate math experiments inside `examples/Calculations/`.
- Link significant changes to the matching `user_requirements/` file (e.g., `user_requirements/R012 - source prioritization.md`).

## Build, Test, and Development Commands
- `platformio run` builds the default `esp32dev` environment.
- `platformio run --target upload` flashes a connected ESP32; set `upload_port` locally when needed.
- `platformio device monitor` opens the 115200 baud serial console.
- `python examples/poseidongw/scripts/udp_logger.py` mirrors the UDP debug feed.

## Coding Style & Naming Conventions
- Follow `src/main.cpp`: 2-space indents, braces on their own line, inline braces allowed for one-line conditionals.
- Prefer `constexpr` for configuration, `SCREAMING_SNAKE_CASE` for constants/macros, `PascalCase` for types, and `lowerCamelCase` for functions.
- Collect pin maps and config values near the top of each translation unit; use purposeful comments and stick to ASCII unless the file already differs.

## Testing Guidelines
- Unity tests live in `test/` grouped by feature (`test_wifi_units`, `test_nmea2000_integration`, etc.); align filenames with their directory.
- Expose seam-friendly adapters around hardware so `native` and `esp32dev_test` targets can substitute fakes.
- Run `platformio test -e native` and `platformio test -e esp32dev_test` (or `--environment esp32dev`) before submission and note the summaries in your PR.

## Commit & Pull Request Guidelines
- Use Conventional Commits with subjects under 72 characters and capture hardware, wiring, or protocol impacts in the body.
- PRs must link the relevant `user_requirements` item, list executed build/test commands, and share serial or UDP snippets for user-facing changes.
- Call out secrets or environment variables others must provide and document new defaults.

## Specification Workflow
- Use `/speckit.specify` with a feature summary and follow `.codex/prompts/speckit.specify.md`; run `.specify/scripts/bash/create-new-feature.sh --json "$ARGUMENTS"` only once per feature.
- Author specs with `.specify/templates/spec-template.md`, populating the generated `SPEC_FILE` with actors, flows, requirements, and success criteria.
- Create `FEATURE_DIR/checklists/requirements.md`, validate each item, and resolve issues before moving to `/speckit.clarify` or `/speckit.plan`.

## Security & Configuration Tips
- Never commit real Wi-Fi credentials—use placeholders such as `wifi_ssid` and `wifi_pwd`.
- Document new defaults, wiring updates, or required peripherals in `README.md` (or module-specific docs) so integrators can mirror the setup.

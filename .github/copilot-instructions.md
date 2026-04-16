# Copilot Instructions

## Scratch folder

Use a scratch folder **outside the repository** for all temporary scripts and experiments. Never create temporary files inside the repository.

## Project Structure
- The VS workspace root is inside `build/` (e.g. `build/msvc/`); the repository root is `../../` relative to it.
- Never use absolute paths or place any files inside `build/` — it is generated and git-ignored.
- `external/` is git-ignored and populated by `bootstrap.sh` / `bootstrap.bat`.
- All build outputs go to `bin/` (WASM → `bin/wasm/`, Android → `bin/arm64-v8a/`).

## Targets
- `IFStile` — main GUI application (Windows/macOS/Linux/Android/WASM).
- `IFScore` — static library with core logic; source in `SrcCore/`.
- `IFScore_ST` — single-threaded variant of `IFScore` for WASM `STANDALONE_WASM` (no `-pthread`).
- `IFSfinder` — CLI search tool; source in `SrcFinder/`.
- `IFSlib` — WASM-only standalone `.wasm` library; source in `SrcLib/`.
- `Tests` — GTest suite; source in `SrcTests/`. Has its own `main()` — link `GTest::gtest`, not `GTest::gtest_main`.

## Build
- C++20, CMake 4.2, configurations: Debug and Release only.

## Code Style
- Code comments must be written in English.

## Dependencies
Available in `external/`: SDL3, ImGui, QuickJS, Eigen, {fmt}, GoogleTest.
`Tiny/` contains: miniz, md4c, imgui_md (TinyLib).Platform-specific source files follow the pattern `platform_<os>.cpp` in `Src/`.

## CI
GitHub Actions runs a Release build + ctest on Ubuntu on every push/PR to `main`.

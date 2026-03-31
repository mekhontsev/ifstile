# Copilot Instructions

## Project Structure
- This is a CMake project. The VS workspace root is the CMake binary directory (`build/msvc/`), not the repository root.
- The repository root is **two levels above** the VS workspace root (`../../` relative to `build/msvc/`).
- Never place project files, configs, or instructions inside `build/` — it is a generated directory listed in `.gitignore`.
- When creating any file for the project, always use the repository root as the base.

## Code Style
- Code comments must be written in English.
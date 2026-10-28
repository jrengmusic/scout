# scout

## Identity

- **Name:** scout
- **Type:** Headless command-line tool, JUCE console project
- **Language:** C and C++17
- **Framework:** JUCE, with the `jam` user module
- **Version:** 0.1.0
- **Description:** Systematic Context Observer for Unit Telemetry
- **Root path:** /Users/jreng/Documents/Poems/dev/scout

## Current state

- **Last sprint:** None. `carol/SPRINT-LOG.md` holds no sprint entry.
- **Active ODE:** None recorded.
- **Active debt:** None. No `DEBT.md` exists at the project root.

## Layout

- `Source/` — application source: `Main.cpp`, `Help.cpp`/`Help.h`, `Benchmark.cpp`/`Benchmark.h`, `ProfileReport.cpp`/`ProfileReport.h`, `Statistics.cpp`/`Statistics.h`.
- `Source/generated/` — generated sources.
- `cast/` — generation inputs and notes: `spell.md`, `signing.md`, `identifiers.md`, `source.cast`, `installer/`.
- `Builds/` — build output: `Debug/`, `Release/`.

## Key documents

| Document | Purpose |
|---|---|
| `CMakeLists.txt` | Build manifest. Generated. Do not edit by hand. |
| `project-info.md` | Source table for project metadata and CMake values. |
| `cast/signing.md` | Input for the signing chain of the build. |
| `RELEASE.md` | Release notes for scout v0.1.0. |
| `RFC-live-measurement.md` | RFC on scout as the runner and statistics lane. |
| `PLAN-live-measurement.md` | Plan that builds the RFC, with its amendments. |
| `PLAN-mermaid-writer.md` | Plan of the Mermaid writer and the report charts, with Amendment D. |
| `carol/SPRINT-LOG.md` | Sprint log, written by primary agents only. |
| `LICENSE` | License text. |

## Build

- Configure with CMake. `CMakeLists.txt` requires CMake 3.25 or later.
- The JUCE path is `../../JUCE` relative to the project. The user module path is `../jam`.
- The Release build runs the sign, package, notarize, and staple chain. Set `CAST_SIGN` to `OFF` to skip it.
- Edit `project-info.md`, run `cast`, then configure. Do not edit `CMakeLists.txt` directly.

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

- **Last sprint:** Sprint 7: MCP Server Mode (`scout --mcp`) (`carol/SPRINT-LOG.md:117`).
- **Active ODE:** None recorded.
- **Active debt:** None. No `DEBT.md` exists at the project root.

## Layout

- `Source/` — application source: `Main.cpp`, `Processor.cpp`/`Processor.h`, `Validator.cpp`/`Validator.h`, `Toolchain.cpp`/`Toolchain.h`, `Benchmark.cpp`/`Benchmark.h`, `Sampler.cpp`/`Sampler.h`, `Symbolizer.cpp`/`Symbolizer.h`, `ProfileReport.cpp`/`ProfileReport.h`, `Statistics.cpp`/`Statistics.h`, `ToolServer.cpp`/`ToolServer.h`, `tools.json`, `HELP.md`.
- `Source/generated/` — generated sources.
- `cast/` — generation inputs and notes: `spell.md`, `signing.md`, `identifiers.md`, `banner.md`, `files.md`, `text.md`, `cmake.cast`, `installer.cast`, `source.cast`, `installer/`.
- `Builds/` — build output: `Debug/`, `Release/`.
- `test/` — MCP test suite: `mcp.lua`, `fixture/`, `baseline/`.
- `benchmark/` — scout manifests and their reports: `mcp.md`, `mcp.sh`, `mcp-report.md`, `mcp-samples.md`.
- `docs/` — generated doxygen output: `xml/`, `tagfile.xml`.

## Key documents

| Document | Purpose |
|---|---|
| `CMakeLists.txt` | Build manifest. Generated. Do not edit by hand. |
| `project-info.md` | Source table for project metadata and CMake values. |
| `cast/signing.md` | Input for the signing chain of the build. |
| `RELEASE.md` | Release notes for scout v0.1.0. |
| `PLAN-sampling-gaps.md` | Plan for the sampling gaps: symbols, attribution, and the incoming process (2026-10-31). |
| `carol/SPRINT-LOG.md` | Sprint log, written by primary agents only. |
| `LICENSE` | License text. |

## API docs

- Doxygen XML: `docs/xml/`. The index is `docs/tagfile.xml`.

## Build

- Configure with CMake. `CMakeLists.txt` requires CMake 3.25 or later.
- The JUCE path is `../../JUCE` relative to the project. The user module path is `../jam`.
- The Release build runs the sign, package, notarize, and staple chain. Set `CAST_SIGN` to `OFF` to skip it.
- Edit `project-info.md`, run `cast`, then configure. Do not edit `CMakeLists.txt` directly.

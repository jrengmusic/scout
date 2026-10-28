# RFC — profiling: four lanes, scout as the one command, skills as thin callers
Date: 2026-10-28
Status: open
Author: MACHINIST, at ARCHITECT's request. This RFC replaces three earlier RFCs: `~/.carol/RFC-profile-skill.md`, `scout/RFC-cli-help.md`, `scout/RFC-scout-toolchain.md`. Their content is merged here, and the contradictions between them are resolved in §R10.
Targets: `scout/Source/Main.cpp`, `scout/Source/HELP.md`, `scout/project-info.md` (if a module is added), `eve/benchmark/live.md`, `eve/benchmark/replay.md`, `~/.carol/skills/profile/SKILL.md`, `~/.carol/skills/trace/SKILL.md`, `~/.carol/skills/trace/sample.lua`, `~/.config/nvim` (read side only).
Related: `scout/RFC-live-measurement.md` (committed, built; R1–R12 are built, §R2 lists their state). `~/.carol/HANDOFF-profiling.md` (work order after scout is updated).

This RFC states facts, requirements and options. It records no decision that ARCHITECT did not state. Every new name, field and message text is ARCHITECT's (NAMES.md Rule -1).

## Problem Statement

ARCHITECT's words that govern this RFC, verbatim, in order:

1. *"let's start profiling eve with scout. what genuienely we could optimise to get stable, most deterministic, highest critical performance, within the limit of AUDIO ARCHITECTURE discipline with lock free, fully event driven, concurrent, at parity of endless that beats all terminal definitively?"*
2. *"what do we need to fix scout to measure deterministically whatever you need? as SKILLS.md? scout engine contract changes? both? i'll fix it first."*
3. *"so each time we want to profile we need to rebuild scout with whatever project we're profiling? i thought it was standalone headless cli only need to be "attached""*
4. *"profiling with scout is granted with build permission"*
5. *"what gives? how do we make the profile works without rebuilding scout each time? rebuilding project that being profiled is logically make sense, but why do we need to rebuild scout each time?"*
6. *"can we just integrate with whatdbg? is scout need to be redesigned with attach process like whatdbg? or we need all to profile? each with its own lane?"*
7. *"is whatdbg need update too? need to be able generate report as markdown/mermaid like scout?"*
8. *"does changes for driver.lua included on your RFC for carol?"*, then *"go"*.
9. *"i dont need to build profile with nvim. with profiling protocol, agent granted permission to build. profiling is always agents building"*
10. *"profile related data should be at scout manifest, not cast as toolchain"*
11. *"cast stays as toolchain. contract does not change. you need to run cmake for whatever profile need to build, hand it off to scout."*
12. *"instead of hardcoding whatever command to build profile into skill, why not we just make scout able to launch "toolchain" to profile like cast? so skill always deterministically call the exact scout command whatever flags it needs? so this is IDENTICAL to cast, while the the command is not running cast, but scout with its manifest"*
13. *"scout always profile, why do we need extra redundant flag"*
14. *"jam_unit_test should just moved to scout, so JAM and KANJUT cleaner no?"* (scout/RFC-live-measurement §0 item 15)

Target number, from the deleted `jam/PLAN-eve-max-speed.md:24` (quoted in RFC-live-measurement §0): *"END (old endless) is sub 1 min. that's the least acceptable number"*.

State of the work, in one paragraph. scout is built as a runner: it runs a manifest's `command` once per epoch, reads the `samples` file the command writes, and writes the report (RFC-live-measurement, built). The EVE manifests, the replay build and the live build exist. The profile and trace skills are rewritten against the built scout. What remains: (a) four defects and gaps in the scout command line; (b) scout running the manifest's toolchain, so that the skill calls one fixed command; (c) the Where-lane raw-file output of `sample.lua`; (d) a rule for how clangd sees profile-only code; (e) a few decisions on the profile build flags and the epoch size.

## Research Summary

### R1. The four lanes

| Lane | Tool | What is rebuilt | Question it answers | What it may not claim |
|---|---|---|---|---|
| Where | whatdbg `sample`, attach by pid (trace skill) | the target, with debug symbols | Which functions are hot on each thread? | Throughput or wall time. The driver pauses the process. |
| How long | zones compiled into the target's profile build | the target only | How long does each stage take, per call? | Kernel time of a child process. |
| Statistics and gate | scout | nothing | Median, spread, baseline, p-value. | Anything that no lane measured. |
| Verdict | scout `user` and `system` rows, or `time seq` | nothing | The number against endless. | Where the time goes. |

Rules: one measurement at a time on the machine (ghostty `src/benchmark/AGENTS.md:29-30`); a performance verdict needs a Release build, and the report states the build type; a finding names its lane.

The measured gap that started the work (RFC-live-measurement §1): `seq 1 100000000` took 59.609 s in endless (`eve/comparison.md:58`), 1:01.20 in EVE Sprint 176 (`:62`), and 1:04.78 in the latest Release (ARCHITECT, 2026-10-28). The loss is in the system time of `seq` (46.69 s against 39.38 s), which is kernel time of a child process.

### R2. State of the earlier requirements (verified against the built scout, the EVE manifests and the skills, this session)

| Item | Source | State |
|---|---|---|
| Manifest names a command per run; scout runs it per epoch | live-measurement R1 | Done (`HELP.md:11`, `Benchmark.cpp:33-50`) |
| Target writes raw zone durations; scout reads them | R2 | Done (`jam::Profiler::writeSamples`, `jam_Profiler.h`) |
| Child `user` and `system` time rows | R3 | Done (`Benchmark.cpp:17-31`, `:58-73`). The `login` grandchild question is open (R9 below). |
| Build type in the report | R4 | Done (`RUN` table, `buildType`) |
| Per-call distribution and tail | R5 | Done (`_CALLS` table, 64 bins per octave, `percentile` 99.9) |
| Report location in the measured project | R6 | Done for the path. The folder of `output` must exist (F4 below). |
| Replay workload as a binary | R7 | Done (`Builds/Replay`, `EVE_USING_REPLAY`) |
| Live EVE epoch, form 1 (EVE runs one command and quits) | R8 | Done. EVE reads the manifest as its first argument and quits when the shell exits (`EVETerminalProcessor.cpp:32-35`, `:41`, `:170-180`). The profile destructor writes the samples (`EVEProcessor.cpp:58-68`). |
| Wake-path zones | R9 | Done. Seven zones exist: `deviceClock` (`EVEProcessor.cpp:37`), `ttyBytesReady` (`jam_TerminalTTY_posix.cpp:219`), `terminalProcess` (`jam_TerminalProcessor.cpp:34`), `terminalRead` (`:71`), `swapLiveLines` (`jam_TerminalSwap.cpp:67`), `swapHistoryLines` (`:95`), `editorLanes` (`jam_TextEditor.cpp:796`). `VirtualDevice::clock` and the Q_B writer have no zone of their own. |
| Dropped entries reported | R10 | Done (`dropped` row; `jam_Profiler.cpp:39`, `:44`) |
| Debug symbols for the Where lane | R11 | Open (Open Question 9) |
| One measurement at a time | R12 | A rule. It sits in the profile skill. |
| `jam_unit_test` moves to scout; JAM and KANJUT delete it | R13 | scout's user module list has no `jam_unit_test` (`scout/project-info.md`, `## user module`). The deletion in JAM and KANJUT was not checked in this RFC. |
| Detach in attach mode (`driver.lua`) | profile-skill RFC §2.10.1 | Done (`driver.lua:155-171`; both callers pass `spec`: `sample.lua:246`, `trace.lua:224`; `trace/SKILL.md` outcome lines updated) |
| Where-lane notes in the trace skill | §2.9 | Done (`trace/SKILL.md`: lane note, probe note, `-g` note) |
| Profile skill rewritten | §2.1–2.8 | Done against the built scout, with the corrections in §R10 |
| Tallies in the scout raw-file format (`sample.lua`) | §2.10.2 | Open (Open Question 8) |

### R3. Observed run (replay, this session)

`scout eve/benchmark/replay.md`, run by Pathfinder after the replay rows built (cast, cmake, ninja all exit 0): exit 0, stderr empty, `buildType` Release, 11 epochs. `epoch` median 1.8746e9 ns (errorPercent 1.94); `user` 1.0761e9 ns (1.45); `system` 1.0987e9 ns (0.36); `dropped` 0. The `_CALLS` table reports wide per-call spread (earlier run: `swapHistoryLines` errorPercent 99.998, `terminalProcess` 46.09, `editorLanes` 99.40). The report has no baseline columns because no baseline was set. The live 100M run has not been executed.

### R4. Findings on the scout command line (observed, then traced to source)

- **F1. `scout --help` prints no help.** It prints `/private/tmp` to stderr and exits 1. `Main.cpp:82-83` takes any one argument as the manifest path. The help prints only for an argument count that is not 2 (`Main.cpp:85-88`).
- **F2. A missing or empty manifest passes validation.** `scout /tmp/empty-manifest.md` (an empty file) printed `/tmp` and exited 1. `Main.cpp:60-61`: `loadFileAsString()` returns an empty string for a missing file, and `ConfigValidator::isValid` accepts the empty document. `Main.cpp:65`: `runEpochs` reads a `run` table that does not exist, so `epochs` is 0 and `output` is empty. `Main.cpp:20`: `getChildFile ("")` is the manifest folder. `Main.cpp:23-27`: `replaceWithText` fails on a folder, and the code prints the folder as the report path. The `epochs > 0` assert (`Main.cpp:37`) is off in a Release build.
- **F3. stderr messages have no newline.** `Main.cpp:26`, `:50`, `:68` use `std::fputs` with no `'\n'`. The shell prompt joins the message.
- **F4. `HELP.md` leaves the target contract undocumented.** It does not state (1) what the target writes: one `## <TYPE>` table with `key | type | value | calls | unit | bins`, a `dropped` row, and a `## RUN` table with `buildType` (written by `jam::Profiler::writeSamples`); (2) the five causes of `EXIT_FAILURE`: wrong argument count (`Main.cpp:85-88`), manifest validation failure (`Main.cpp:68-69`), a command that exits with a code other than 0 (`Benchmark.cpp:75-76`), a samples file that is missing after an epoch (`Benchmark.cpp:135`), a report that cannot be written (`Main.cpp:23-27`); (3) that the folder of `output` must exist (`replaceWithText` creates no folder, `juce_File.cpp:788-802`); (4) that scout deletes the samples file before each epoch (`Benchmark.cpp:54`).
- **F5. The `HELP.md` example mixes two owners.** It shows a `run` table and a `profiler` table (`HELP.md:85-111`). The `profiler` table belongs to EVE and EVE reads it (`EVETerminalProcessor.cpp:170-180`). The text says scout does not read other tables (`HELP.md:25`). The example does not say whose table it is.

### R5. The cast toolchain rule, and how scout can copy it

`cast/SPEC.md` §6.9 (`:869-905`):

- `## toolchain` is a reserved, optional table. Columns `argument | command | flag`. The `argument` column is optional. A table declared without it is `| command | flag |` (`:881`).
- Rows run in authored order, one child process per row (`:888-889`).
- The `command` cell is argv[0], verbatim, never split. The `flag` cell is the argument list: it splits on spaces, and a double-quoted span becomes one argument with the quotes removed. A blank `flag` cell runs the command alone (`:889-891`).
- The command resolves through the caller's environment. The engine adds no resolution, no shell, no quoting (`:899-901`).
- A row that cannot start or exits nonzero fails the run. The diagnostic carries the row's command text. The rows after it do not run (`:903-905`).
- A non-empty `--<word>` against a table with no `argument` column is unconditionally fatal (`:883-885`). The word selects among toolchains. ARCHITECT removed the word for scout (item 13): each scout manifest is one profile.

What scout has: `jam::Subprocess` is a scout user module and runs the epoch command in the manifest folder (`Benchmark.cpp:33-50`). Every other manifest path is relative to the manifest (`Main.cpp:30-35`). scout reads only the `run` table; other tables belong to the target (`HELP.md:25`).

How scout reads a `command | flag` table: `ConfigDocument::getValueTree` qualifies a table only when its header carries both a `key` and a `type` column; *"Tables without both columns are skipped."* (jam `ConfigDocument` doxygen.) So scout reads the table through the document's table cells, where each cell takes its column header as its id and its text as its `text` property (`addRow` doxygen). Observed: the two EVE manifests now carry a `## toolchain` table with the columns `command | flag`. scout parsed and validated them and ran (replay run, exit 0). scout ignored the table.

### R6. Why `.clangd` does not see profile-only code

- `~/.config/nvim/lua/core/clangd.lua:69-77` writes `.clangd` and the root `compile_commands.json` from `getCompile(ast)`, which reads the database of `ast.selection.configuration` (`core/project.lua:234-239`).
- For eve the selection is `no-sign`, which is `Builds/Release` (`eve/.project`). `eve/compile_commands.json` has 0 hits of `JAM_USING_PROFILER`. `eve/Builds/Profile/compile_commands.json` has 89.
- The selection changes only through an nvim build. `core/cast-build.lua:22-25` defines two schemes (`Debug = 'debug'`, `Release = 'no-sign'`); `core/build.lua:340` looks the scheme up there.
- nvim lists its build directories from the `cmake -B` rows of the cast `## toolchain` table (`core/project/cast.lua:121-135`). A build that is not a cast configuration is invisible to nvim.
- `.clangd` is written by nvim, not by cast. The cast binary and cast docs contain no `.clangd` text.
- Consequence: an agent profile build writes `Builds/Profile`, and nothing reads it for the editor or for the LSP. Pathfinder's LSP queries on zone code returned wrong or empty results (`findReferences` on `jam::Profiler::Zone` returned `jam_Profiler.h:1:11` only). The doxygen XML agrees with the header line numbers.
- I built an nvim Profile scheme with two keymaps in this session. ARCHITECT ruled that nvim does not build Profile (item 9). I reverted it. `~/.config/nvim` matches HEAD.

### R7. The Where lane (whatdbg `sample`)

- `driver.lua` now detaches in attach mode and terminates in launch mode (`disconnectsByMode`, `driver.lua:155-171`). whatdbg itself can leave the target running (`WhatdbgHandlers.cpp:121`, `:139-142`; `Whatdbg.cpp:406-411`; `Session_mac.cpp:380-382`).
- `sample.lua` builds per-thread `inclusive` and `leaf` tallies (`sample.lua:95`, `:110`) and prints them as `INCLUSIVE` and `LEAF` lines (`:201-213`). The output is plain text.
- scout's `ProfileReport` already writes the markdown report and the mermaid charts. A second report writer in whatdbg or in the driver would be a second copy of that code (MANIFESTO S).
- A share is a count over samples, not a time (`trace/SKILL.md`). Its rows need their own unit and stay apart from the nanosecond rows.
- The zone row keys are `juce::Identifier`s in the samples file (`Benchmark.cpp:101`, `jam_Profiler.h` `writeSamples`). A function name such as `jam::TerminalProcessor::process` is not a valid identifier. The row key scheme for tallies is therefore a naming decision.
- The EVE `profile` build has no `-g`: `debugSymbols` is Debug only (`eve/project-info.md`, `## compile option`).

### R8. The EVE launch under scout (verified by Pathfinder, this session)

- The profile build reads the manifest from its first command-line argument (`EVETerminalProcessor.cpp:41`). It opens the shell from the `profiler` table (`:170-180`).
- EVE quits when the shell exits (`EVETerminalProcessor.cpp:32-35`). The `EVEProcessor` destructor then drains the profiler and calls `writeSamples` (`EVEProcessor.cpp:58-68`). EVE does not read the `samples` key; `writeSamples` does (`jam_Profiler.cpp:159-161`).
- Without `JAM_USING_PROFILER`, EVE registers no listener and does not quit (`EVETerminalProcessor.cpp:181-193`).
- The compile definitions come from cast: `jamUsingProfiler` and `eveUsingReplay` (`eve/project-info.md:393`, `:395`; `eve/CMakeLists.txt:341-342`), gated on the CMake variables `CAST_PROFILE` and `CAST_REPLAY`. ARCHITECT's ruling (item 11): cast's contract does not change. These rows stay.
- `cast cast/spell.md --replay` was the form when the rows were in `project-info.md` (`cast/SPEC.md:893-896`). The rows are no longer there (R9).

### R9. State of the EVE manifests and `project-info.md` (this session)

- `eve/project-info.md`: the `profile` and `replay` toolchain rows are removed. The compile definition rows stay.
- `eve/benchmark/live.md` and `replay.md` each carry a `## toolchain` table with the columns `command | flag`: `cast ../jam/cast/spell.md`, `cmake -S . -B Builds/Profile ...` (or `Builds/Replay ... -DCAST_REPLAY=ON`), `ninja -C Builds/Profile` (or `Builds/Replay`). The paths are relative to the EVE project root. The manifests sit in `eve/benchmark/`.
- Until scout runs the toolchain, the agent reads the table and runs its rows from the project root, then calls scout (profile skill, `## Builds`). I ran the replay rows this way: all three exit 0.
- `live.md` uses `seq 1 100000000`; `replay.md` uses `seq 1 1000000`.

### R10. Contradictions between the earlier RFCs, and how this RFC resolves them

| Statement in an earlier RFC | Resolved as |
|---|---|
| profile-skill RFC §2.5: "Delete the scout rows… After scout RFC R1, scout is built once"; profile data lived in `project-info.md` | Profile build data lives in the scout manifest (item 10). cast keeps its contract (item 11). |
| profile-skill RFC §2.2: Pathfinder only runs scout and never builds | Under the build grant, the build is part of the scout run (Scaffold S2). Until scout implements S2, the agent runs the manifest's rows. |
| scout-builds RFC (deleted): "scout builds the profile targets; the profile schema leaves cast", written from a conditional of ARCHITECT's | Resolved by items 10–13: the build data is in the manifest, in cast's own table shape, run by scout. cast's schema is not changed. |
| scout-toolchain RFC: `--<word>` considered, then removed | Removed (item 13). |
| profile-skill RFC §2.10.1: detach "to do" | Done (R7). |
| profile-skill RFC §2.9: `-g` "until ARCHITECT selects R11" | Still open (Open Question 9). |
| live-measurement R8: form 1 or form 2 | Form 1 is built (R8). |
| An nvim Profile scheme (this session) | Rejected (item 9). Reverted. |
| profile skill `## Builds` with `cast cast/spell.md --profile` and `--replay` | Wrong now: the rows left `project-info.md`. The skill states the manifest's table. |

## Principles and Rationale

- **S, SSOT.** The workload's binary, its build and its measurement are in one file. The skill states one command and no build knowledge. The Where-lane statistics use `ProfileReport` and are not written a second time.
- **D, Deterministic.** The skill calls `scout <absolute-manifest-path>`. The same manifest gives the same build rows in the same order, then the same epochs. The report states its build type and its dropped count.
- **E, Explicit.** The manifest names every command; the `command` cell is verbatim argv[0] with no shell (R5). A failure names the cause, not a folder (F2).
- **B, Bound.** scout owns every child process it starts: the build rows and the epoch command. A row failure stops the run with the row's command in the diagnostic.
- **Encapsulation.** The product manifest (`project-info.md`) holds no measurement configuration. The editor (nvim) builds no profile target.
- **Lean.** No new flag and no new word in scout. The toolchain table is cast's table without its `argument` column and its word.
- **Considered and rejected:** an nvim Profile scheme (item 9); `scout <manifest> --<word>` (item 13); a build procedure inside the skill (item 12); a second report writer in whatdbg or the driver (R7); a function-name row key without a naming decision (R7).

## Scaffold

The contract after the change. Names of new code objects are the implementer's to propose under NAMES.md Rule 5, nearest sibling. Wording of messages is ARCHITECT's.

### S1. scout command line (fixes F1–F5)

1. `scout --help` prints `HELP.md` and exits with `EXIT_SUCCESS` (Open Question 1 selects the mechanism).
2. A manifest file that does not exist: scout prints a message that names the file and exits with `EXIT_FAILURE`. It runs no epoch. A manifest with no `run` table, or a `run` table without the five keys, fails validation with a `path:line (column)` diagnostic (Open Question 2 selects where the check lives). The `epochs > 0` assert then guards a state that validation already excludes (MANIFESTO D).
3. Each stderr diagnostic ends with a newline. One writer function writes all of them (`Main.cpp:26`, `:50`, `:68` call it), so the newline is declared once (MANIFESTO S).
4. `HELP.md` states: the samples-file format and its writer; the five causes of `EXIT_FAILURE` and what stderr carries in each; that the folder of `output` must exist; that scout deletes the samples file before each epoch; that the `profiler` table of the example belongs to the target.

### S2. scout runs the manifest's toolchain, then the epochs (no flag, no word)

1. `scout <manifest>` first runs every row of the manifest's `## toolchain` table, in order. Then it runs the epochs. A manifest with no `## toolchain` table runs the epochs only.
2. The table has the columns `command | flag`, parsed as in R5 (verbatim argv[0]; `flag` splits on spaces; a double-quoted span is one argument; a blank `flag` runs the command alone).
3. Each row runs in the manifest folder through `jam::Subprocess`. The environment is the caller's.
4. A row that cannot start or exits nonzero stops the run. scout prints the row's command text and exits with `EXIT_FAILURE`. scout runs no epoch.
5. Paths in the rows are relative to the manifest, like every other scout path. The EVE manifests change when scout implements S2. Sketch for `eve/benchmark/live.md`:

```
## toolchain

+---------+------------------------------------------------------------------------------------------------+
| command | flag                                                                                           |
+=========+================================================================================================+
| cast    | ../../jam/cast/spell.md                                                                        |
+---------+------------------------------------------------------------------------------------------------+
| cmake   | -S .. -B ../Builds/Profile -G Ninja -DCMAKE_BUILD_TYPE=Release -DCAST_SIGN=OFF -DCAST_PROFILE=ON |
+---------+------------------------------------------------------------------------------------------------+
| ninja   | -C ../Builds/Profile                                                                           |
+---------+------------------------------------------------------------------------------------------------+
```

### S3. The skills after scout implements S1 and S2

- `profile/SKILL.md`: `## Command` states `scout <absolute-manifest-path>` and nothing else. `## Builds` loses the build procedure and keeps the scout build. `## Protocol` keeps the build grant verbatim (the build is part of the run). `## Manifest` documents the `## toolchain` table. `## Limits` records the result of the first live run for the `login` grandchild (R2 row 3).
- `trace/SKILL.md`: unchanged unless Open Question 9 changes the `-g` note.

### S4. `sample.lua` writes the Where-lane tallies in the scout samples format (needs Open Question 8)

1. One sample run is one epoch. `sample.lua` also writes its `inclusive` and `leaf` tallies as a samples file. scout then reports each function's share per thread with the same statistics, markdown and mermaid.
2. The share rows are separate rows from the nanosecond rows and carry their own unit.
3. The text output stays. Pathfinder still returns it verbatim.

### S5. How clangd sees profile-only code (needs Open Question 7)

One of the options in Open Question 7. The decision is ARCHITECT's.

## BLESSED Compliance Checklist
- [ ] Bounds: scout owns every child it starts, in build and in epochs.
- [ ] Lean: no new flag; one reserved table copied from cast; no second report writer.
- [ ] Explicit: verbatim argv[0], no shell, named failure; a missing manifest is named.
- [ ] SSOT: build, binary and measurement in one manifest; the skill holds one command.
- [ ] Stateless: scout holds no build state between runs.
- [ ] Encapsulation: product manifest free of measurement data; nvim builds no profile target.
- [ ] Deterministic: one command, one order, build type and dropped count in the report.

## Open Questions

1. **Help flag (S1.1).** (a) Compare `argv[1]` with a constant list of help flags (`--help`, `-h`) before the manifest path. (b) Treat a one-argument call as a manifest only when the file exists, and print help otherwise; this also solves part of question 2. Decider: ARCHITECT.
2. **Missing or invalid manifest (S1.2).** The message text for a missing file, and where the check lives: the file check in `runManifest` (`Main.cpp:58-70`), or a required-keys rule in the `ConfigValidator` rules for the manifest. Decider: ARCHITECT.
3. **stderr writer (S1.3).** The name of the single writer function and its file. Decider: ARCHITECT (NAMES.md Rule -1), or the implementer from the nearest sibling (Rule 5).
4. **`HELP.md` wording (S1.4).** The facts are in R4 F4 and F5. Decider: the implementer writes, ARCHITECT reviews.
5. **Output of the toolchain rows (S2).** The epoch command's output is discarded (`Benchmark.cpp:41` ignores the string argument of the callback). A build that fails needs its output to be read. Options: scout forwards the row output to stderr; scout writes it to a file next to the report. Decider: ARCHITECT.
6. **The `cast` row and the working folder (S2.3).** The row `cast ../../jam/cast/spell.md` runs in the manifest folder. Whether cast resolves its manifest argument against the caller's folder is not verified here. Decider: the implementer reads `cast/SPEC.md` and confirms with one run. Windows: whether the rows of one manifest differ by platform, and how; the Windows toolchain for scout is not verified. Decider: ARCHITECT.
7. **clangd and the profile database (S5, R6).** Options: (a) the nvim writer reads a build folder that the scout manifest names; (b) the project `.clangd` always uses the Profile database when it exists, which changes the rule "follow the selection" in `core/clangd.lua:1-10`; (c) no change, and agents work from the source and the doxygen XML. Note: after R9, `Builds/Profile` and `Builds/Replay` are not `cmake -B` rows of the cast toolchain, so nvim cannot list them. Decider: ARCHITECT.
8. **Where-lane row keys, table type and unit (S4).** The table type, the row key scheme (function names are not valid identifiers, R7), and the unit of a share row. Decider: ARCHITECT (NAMES.md Rule -1). This is RFC-live-measurement open question 12.
9. **Debug symbols in the profile build.** `-g` / `/Z7` for the profile build, if the Where lane needs it (live-measurement R11). The flag would go into the `cmake` row of the manifest's toolchain. Not verified: whether function names resolve without it. Decider: ARCHITECT.
10. **Epoch size.** `live.md` uses 100M lines. One 100M epoch lasts about 65 s; eleven epochs last about 12 minutes (live-measurement §8.11). At 10M one epoch is 6.6 s (live-measurement §1), so eleven epochs last about 73 s. The 100M size is the verdict size against endless; the lanes work at any N. Decider: ARCHITECT.
11. **Report folder (R4 F4.3).** Whether scout creates the folder of `output` or requires it to exist (live-measurement open question 5). Decider: ARCHITECT.
12. **The `login` grandchild.** Whether `RUSAGE_CHILDREN` counts `seq` when `seq` is a grandchild through `/usr/bin/login` (`jam_TerminalTTY_posix.h:24`). This is a verification, not a decision: the first live run compares the `system` row with `time seq` for the same N. Owner: the first live run; the result goes in the profile skill `## Limits`.

## Handoff Notes

- Work order after ARCHITECT builds S1 and S2 in scout: `~/.carol/HANDOFF-profiling.md`.
- Files changed in this session (working tree, uncommitted): `~/.carol/CAROL.md` (RFC line, handoff line), `~/.carol/agents/oracle.md` (RFC section points to the skill), `~/.carol/commands/rfc.md` (new), `~/.carol/commands/handoff.md`, `~/.carol/skills/rfc-writer/` (new), `~/.carol/skills/handoff-writer/` (new), `~/.carol/skills/profile/SKILL.md`, `~/.carol/skills/trace/SKILL.md`, `~/.carol/skills/trace/driver.lua`, `trace.lua`, `sample.lua` (detach; before this session's edits per `git status`), `eve/project-info.md` (profile and replay rows removed), `eve/benchmark/live.md` and `replay.md` (`## toolchain` added; the other files in `eve/benchmark/` are scout outputs), `scout/RFC-profiling.md` (this file).
- Deleted: `~/.carol/RFC-profile-skill.md`, `scout/RFC-cli-help.md`, `scout/RFC-scout-toolchain.md`, and the short-lived `scout/RFC-scout-builds.md`.
- `scout/RFC-live-measurement.md` stays: it is committed and built, and §R2 maps its requirements to their state.
- Line numbers in `RFC-live-measurement.md` and `PLAN-live-measurement.md` for `EVETerminalProcessor.cpp` (`:265-279`, `:120-128`, `:79-88`) no longer match the file (234 lines; the writer is at `:136-139`). Cite by symbol when you reuse them.
- Pathfinder protocol deviation to know: one C++ text search ran in a discovery pass (`Q_B`, no output). No other.

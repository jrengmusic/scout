# PLAN: Profiling — scout identical to cast, one command per profile

**RFC:** `RFC-profiling.md`
**Date:** 2026-10-28
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + JAM (scout); Lua (nvim, trace skill); markdown (manifests, skills)

## Context

The RFC asks for one fixed profile command: `scout <absolute-manifest-path>`. That command builds the target from the manifest and then measures it. The scout command line has defects F1–F5 (RFC R4). ARCHITECT ruled the mental model this session. The plan executes that model.

## ARCHITECT rulings (verbatim, this session)

1. Scope: *"Whole RFC S1–S5"*.
2. *"correct mental model: scout is IDENTICAL to cast on everything with --help, building, manifest, etc. the only divergence are cast generate code from table-template. while scout build specifically for profiling no other options and output report in markdown with mermaid chart"*. Before that: *"read how cast handle this. should be IDENTICAL"*.
3. Toolchain code: *"Copy into scout"*. scout gets its own `Toolchain.h/.cpp`, the same as cast's.
4. OQ7 clangd: *"nvim reads manifest dir"*, then *"benchmark/ + DB wins"*.
5. OQ10 epoch size: *"Keep 100M"*.
6. OQ9 debug symbols: *"verify first"*, then *"Both"*. No `-g` now. A probe checks whether function names resolve. The S4 blocker facts are verified by a run before the wrapper design locks.
7. S4 epoch: *"Wrapper reads manifest"*. This waits on the verification (ruling 6).
8. OQ8 Where-lane rows: *"Decide after verify"*.
9. Banner: *"ARCHITECT supplies"* `scout/cast/banner.md`.
10. Epoch command failure: *"failCommand"*. Text row `failCommand` | `command failed`. stderr: `scout: <command path>: command failed`.
11. Carried from the RFC: item 9 *"profiling is always agents building"*; item 11 *"cast stays as toolchain. contract does not change"*; item 13 *"scout always profile, why do we need extra redundant flag"*. "Profiling with scout is granted with build permission" covers every build in this plan.

### RFC points that these rulings change

- RFC S2.3 and S2.5 say that the toolchain rows run in the manifest folder, with paths relative to the manifest. Ruling 2 replaces this. cast runs each row in the caller's working folder (`cast/Source/Toolchain.cpp:22`; `cast/SPEC.md` §6.9: *"The command resolves through the caller's environment. PATH, the working directory … are the caller's responsibility"*). scout does the same. The EVE manifest rows stay relative to the EVE project root (`eve/benchmark/live.md`, `replay.md`, `## toolchain`). The skill runs scout from the project root.
- RFC S1.2 asks for a required-keys validation rule. Ruling 2 replaces this with cast's behaviour:
  - A missing manifest gives help, then `not found` (`cast/Source/main.cpp:860-865`).
  - An empty manifest does nothing and exits 0 (cast code path: `Validator.cpp:22`, `:49`).
- RFC OQ5 (output of the toolchain rows): cast streams each chunk to stdout (`Toolchain.cpp:28-31`). The copy (ruling 3) carries this.
- RFC OQ6 (Windows rows): the cast toolchain table has no platform column (`cast/SPEC.md` §6.9). The same applies to scout.
- RFC OQ11 (report folder): cast creates the folders (`jam_Document.cpp:214`, `Document::Writer::toFile`). The same applies to scout.

## Overview

1. scout copies cast's command line: banner, `--help`, `--version`, `not found`, `printError`, `Fine.`, and the screen erase.
2. scout runs the manifest's `## toolchain` rows, copied from cast's Toolchain. Then scout runs the epochs and writes the report, and it creates the report folder.
3. The profile skill calls one command. nvim gives clangd the database of the profile build.
4. A verification run settles OQ9 and the S4 facts. A plan amendment then settles the OQ8 rows and the S4 wrapper.

## Language / Framework Constraints

- CODING.md *One object, one translation unit*: `Toolchain` is one header and one `.cpp`.
- CODING.md *No `==` / `!=` on strings*: compare flag words with `juce::String::compare (…) == 0`, as cast does (`main.cpp:451`).
- CODING.md *Header discipline*: copy cast's own includes for each file.
  - cast `Help.h` includes `"generated/Generated.h"` (`cast/Source/Help.h:3`).
  - cast `Toolchain.h` includes `<JuceHeader.h>` and `"generated/Generated.h"`.
- LANGUAGE.md C++/JUCE: no per-language override beyond Lean file placement.

## Dependency & API Inventory

- **jam::Subprocess**: `launch (StringArray argv, File workingDir, Completion, Chunk)` (`jam_Subprocess.h:95-98`). cast's `Toolchain::runProcess` uses it (`Toolchain.cpp:15-41`).
- **juce::StringArray::addTokens (flag, true)** with quote preservation (`juce_StringArray.cpp:341-344`). cast's `getToolchainArguments` uses it (`Toolchain.cpp:3-13`).
- **jam::MarkdownDocument table read**:
  - `getTables (Id)` (`jam_MarkdownDocument.h:96`)
  - `getTableRows` (`:169`)
  - `getTableHeaders` (`:177`)
  - `getTableValue (row, colId)` (`:231`)
  - `jam::ConfigDocument` derives from `MarkdownDocument` (`jam_ConfigDocument.h:19`).
- **jam::Document::Writer::toFile**: it creates the parent folder, then writes (`jam_Document.cpp:212-219`). `jam::MarkdownWriter` is the Writer that `ProfileReport` uses (`ProfileReport.cpp:11`).
- **jam::TerminalBuffer::writeEraseScreen (OutputStream&)** (`jam_TerminalBuffer.h:132`).
- **BinaryData::getString**, `BinaryData::fetcher` (`jam_Raw.h:81`, `:111`).
- **Id::diagnosticSeparator** `": "` (`jam/generated/jam_Core.cpp:55`). **Id::version** (`jam_Core.h:138`).
- **CAST_COMMIT** is already defined for scout (`scout/CMakeLists.txt:49-51`, `:227`). **ProjectInfo::projectName**, `versionString` (`scout/Source/generated/ProjectInfo.h:34`, `:37`).
- **cast sources to copy** (all read this session):
  - `main.cpp` 22-26, 34-37, 70-84, 429-468, 474-493, 797-805, 860-865, 1414-1485
  - `Toolchain.h/.cpp`
  - `Processor.cpp` 50-76 and 124-135
  - `Help.h/.cpp`
  - `cast/cast/text.md`, `files.md`, `spell.md` rows
- **nvim**:
  - `core/project/cast.lua` `getConfigurations` (121-135), `getCompile` (224-240), `getSources` (244-256)
  - `core/project.lua` `getCompile` (234-239)
  - `core/markdown-table.lua` `getTables`, `getRecords` (139, 181)
- **Families for new names (NAMES.md Rule 5)**:
  - Text rows `failNotFound`, `failToolchain`, `failToolchainColumn`, `failOutputWrite`, `done`: verbatim from `cast/cast/text.md`. `failCommand` is ruling 10.
  - Files row `scoutHelp`: cast's `castHelp` with the project-name prefix (`cast/Source/generated/Files.h:33`).
  - Identifiers `toolchain`, `flag`, `help`, `doubleDash`: from cast's identifiers (`cast/Source/generated/Identifiers.h`).
  - Functions `printBanner`, `printError`, `getHelpFlag`, `getVersionFlag`, `isHelp`, `isVersion`, `writeVersion`, `isTerminalOutput`, `printDone`, `printBannerAndHelp`, `runDocumentNotFound`, `runCommandLine`: verbatim from `cast/Source/main.cpp`.
  - `runToolchain`, `runToolchainRow`: cast's `Processor::run` and `runToolchainRow`. The `run*` family is in `scout/Source/Main.cpp` (`runManifest`, `runEpochs`).
  - `ProfileReport::toFile`: the `Writer::toFile` family (`jam_Document.h:886`).

## Validation Gate

COUNSELOR validates each step before the next step. The checks are MANIFESTO.md (BLESSED), NAMES.md, `~/.carol/CODING.md` and the locked decisions of this plan. No deviation and no scope drift are allowed. COUNSELOR reads every changed file at the reported lines. @Auditor runs once, after the final code step, over the whole sprint.

## Steps

### Step 0: Plan file
**Scope:** `scout/PLAN-profiling.md`
**Action:** Copy this plan to the file.
**Validation:** The file matches this plan.

### Step 1: scout vocabulary and generated layout (cast layout)
**Scope:** `scout/cast/spell.md`, `scout/cast/identifiers.md`, new `scout/cast/text.md`, new `scout/cast/files.md`, `scout/project-info.md` (`## binary`), and the generated outputs under `scout/Source/generated/`.
**Action (@Engineer):**
1. Add `text.md` with these rows, copied verbatim from `cast/cast/text.md`: `failNotFound`, `failToolchain`, `failToolchainColumn`, `failOutputWrite`, `done`. Add one row: `failCommand` | `command failed` | `The run command exited nonzero.`
2. Add `files.md` with the row `scoutHelp` | `HELP.md`, in the shape of cast's `castHelp` row.
3. Add `toolchain`, `flag`, `help` and `doubleDash` to `identifiers.md`, with cast's values.
4. Take cast's generated layout: `Identifiers.h`, `Text.h`, `Files.h`, `Generated.h`, `Generated.cpp`. scout's `Identifiers.cpp` becomes `Generated.cpp`.
   - Delete `Identifiers.cpp` first.
   - Copy the matching `cast/cast/spell.md` index and output rows.
   - COUNSELOR pastes those rows in the brief.
5. Add the `@banner` index row and the `map::banner` output row, both as in cast. The `banner.md` file is ARCHITECT's (ruling 9).
6. Run `cast cast/spell.md --no-sign`. This is under the build grant.
**Validation:**
- The generated headers declare exactly the new names.
- `Generated.cpp` defines them.
- No other name is added (CODING *No identifier latitude*).
- Every row traces to a cast sibling row.

### Step 2: Toolchain copy
**Scope:** new `scout/Source/Toolchain.h`, `scout/Source/Toolchain.cpp`
**Action:**
- Copy `cast/Source/Toolchain.h/.cpp` verbatim.
- Change the includes only where scout's generated layout requires it.
- Keep `text::Diagnostics::failToolchain` and the stdout chunk forwarding.
**Validation:**
- A diff against cast's files shows only the include lines.
- `getToolchainArguments` and `runProcess` are byte-for-byte the same as cast's.

### Step 3: Help takes text (cast Help)
**Scope:** `scout/Source/Help.h`, `scout/Source/Help.cpp`
**Action:**
- Replace `printHelp (const juce::String& helpFile)` with cast's `printHelp (const juce::String& helpText)` (`cast/Source/Help.h/.cpp`).
- Use `files::scoutHelp` as the parse origin.
**Validation:** The diff against cast's `Help.cpp` shows only `castHelp` changed to `scoutHelp`.

### Step 4: Command line (cast main)
**Scope:** `scout/Source/Main.cpp`
**Action:** Delete first: delete the current `main`, `runManifest`, `writeReport`, and the `std::fputs` writes. Then copy cast's functions, as listed in the Inventory. The behaviour table:

| Call | Result | cast source |
|---|---|---|
| `scout` | banner + help, exit 0 | `main.cpp:1442-1443` |
| `scout --help`, `scout <m> --help` | banner + help, exit 0 | `:446-453`, `:1422-1426` |
| `scout --version` | `scout 0.1.0 (<commit>)`, exit 0 | `:429-436`, `:459-468` |
| `scout <m>`, `m` missing | banner + help; stderr `scout: not found: <name>`; exit 1 | `:860-865` |
| `scout <m>`, `m` exists | the run below. On a TTY, success prints `scout: Fine.`. Failure goes to stderr as `scout: <message>\n`, exit 1. | `:34-37`, `:486-493`, `:843-850` |
| any call, on a TTY | screen erase first | `:1466-1472` |

- The manifest path resolves against the working folder, then `.getLinkedTarget()` (`:418`).
- The run, in order:
  1. Parse and `ConfigValidator::isValid` (current `Main.cpp:60-61`).
  2. Run `runToolchain`. For each `## toolchain` table, run each row in order through `runToolchainRow`. These are copies of `Processor.cpp:50-76` and `:124-135`, without the `argument` selection (ruling 11, item 13).
  3. A header without a `command` or a `flag` column fails with `failToolchainColumn`. Copy cast's check: COUNSELOR reads cast `Validator` for this rule and pastes it in the brief.
  4. When a `run` table exists, run `runEpochs`. Then write the report.
- No `run` table means no epochs and no report, exit 0. This is the same as cast with an empty manifest.
**Validation:**
- Every function traces to a cast line.
- No bail-out guards. Positive nesting.
- `not`/`and`/`or` only.
- One stderr writer (`printError`).

### Step 5: Benchmark and report diagnostics
**Scope:** `scout/Source/Benchmark.cpp`, `scout/Source/ProfileReport.h/.cpp`, `scout/Source/Main.cpp`
**Action:**
1. In `Benchmark::runEpoch`, the failure becomes `<command path> + Id::diagnosticSeparator + text::Diagnostics::failCommand`.
2. In `addEpoch`, the failure becomes `failNotFound + diagnosticSeparator + <samples path>`. This is cast's `isFound` form (`main.cpp:721`).
3. Replace `ProfileReport::getText` with `static bool toFile (const juce::ValueTree& profiles, const juce::File& outputFile)`.
   - It builds the same document.
   - It calls `jam::MarkdownWriter {…}.toFile`, which creates the folder (`jam_Document.cpp:214`).
   - Delete `getText` first. COUNSELOR confirms by a read that `Main.cpp:21` is its only caller.
   - On failure, Main prints `<path>: cannot write output` (cast `Writer.cpp:120-122`).
**Validation:**
- No `std::fputs` remains.
- No `replaceWithText` remains for the report.
- Doxygen stays untouched until Step 15.

### Step 6: HELP.md
**Scope:** `scout/Source/HELP.md`
**Action:** Write the guide in cast's HELP.md shape (`cast/Source/HELP.md:1-5`: title, version line, *"You see this guide because …"*). The content:
- the command line table of Step 4;
- the manifest: the `run` keys, and the `## toolchain` table (`command | flag`, caller's working folder, stdout stream, failure text);
- the samples-file format and its writer, `jam::Profiler::writeSamples`;
- the report;
- the exit causes;
- that scout deletes the samples file before each epoch;
- that the `profiler` table of the example belongs to the target.

The implementer writes it and ARCHITECT reviews it (RFC OQ4).
**Validation:** Every fact cites code read in this sprint.

### Step 7: Build scout and verify the command line
**Scope:** build only (grant).
**Action:**
1. Run `cast cast/spell.md --no-sign`, then the cmake and ninja rows of `no-sign`.
2. Pathfinder runs each Step 4 case and the HANDOFF cases:
   - `scout --help`
   - `scout`
   - `scout missing.md`
   - an empty manifest
   - a manifest whose `output` folder does not exist
3. Pathfinder returns exit code, stdout head and stderr verbatim.
**Validation:**
- Each result matches the Step 4 table.
- The output folder is created.
- The `macroRedefined` warning is gone (`project-info.md:261`, `:308`).

### Step 8: Replay build and measure in one call
**Scope:** run only.
**Action:** Pathfinder runs `scout /Users/jreng/Documents/Poems/dev/eve/benchmark/replay.md`, from `/Users/jreng/Documents/Poems/dev/eve`.
**Validation:**
- The toolchain output streams.
- Exit is 0.
- The `RUN` table shows `buildType` Release.
- The report is written.

### Step 9: Profile skill (S3)
**Scope:** `~/.carol/skills/profile/SKILL.md`
**Action:**
- `## Command`: `scout <absolute-manifest-path>`, run from the project root. Add the Step 4 exit table.
- `## Builds`: delete the build procedure. Keep the scout build rows.
- `## Manifest`: document `## toolchain` as scout runs it.
- `## Limits`: delete "scout creates no folder". Correct `jam_Profiler.h:30` to `:111`.
- Keep the build grant verbatim.
**Validation:** Each sentence matches the built scout (Step 7 evidence).

### Step 10: Live verdict run and the login grandchild (OQ10, OQ12)
**Scope:** run only. About 12 minutes (ruling 5).
**Action:**
1. Pathfinder runs `scout /Users/jreng/Documents/Poems/dev/eve/benchmark/live.md` from the EVE root.
2. Pathfinder then runs `time seq 1 100000000 > /dev/null`. This is one measurement at a time.
3. Record the `system` row against `time seq` in the profile skill `## Limits`.
**Validation:** Exit 0. The comparison is recorded with both numbers.

### Step 11: nvim — benchmark databases win (OQ7, ruling 4)
**Scope:** `~/.config/nvim/lua/core/project/cast.lua`, `~/.config/nvim/lua/core/project.lua`
**Action:**
1. `cast.lua` reads each `<root>/benchmark/*.md` with `markdown.getTables`.
2. It takes the `cmake -B` rows of each file's `## toolchain` table, with the same match as `getConfigurations:124-130`, keyed by the manifest file name.
3. It builds their compile entries with the same code as `getCompile:224-240`.
4. Their manifests and databases join `sources`, so the watcher reparses.
5. These entries feed the compile section only. They do not enter `configurations`, `targets` or `launch`. nvim never builds or launches a profile (ruling 11, item 9).
6. `project.getCompile` returns a benchmark compile entry when one exists, first in key order (the rule of `project.lua:231-233`). Otherwise it returns the selection as today.
**Validation:**
- `grep -c JAM_USING_PROFILER eve/compile_commands.json` returns more than 0 after nvim reloads EVE (HANDOFF item 9).
- No build path changes.

### Step 12: Verification for OQ9 and S4 (ruling 6)
**Scope:** read and run only.
**Action:**
1. Read the whatdbg launch handler (`WhatdbgHandlers.cpp` launch arguments) to verify that launch takes no program arguments. Read `EVETerminalProcessor.cpp` to verify that the manifest is argv[1].
2. Start the profile build of EVE with the live manifest. Take its pid.
3. Run `sample.lua` attach, `maxSamples` 1, `threads` all, `stackDepth` 32, from a COUNSELOR spec file (trace skill).
4. Record whether function names resolve without `-g`. Record the real frame names and thread names.
**Validation:** The evidence is verbatim.

**Stop (ARCHITECT's rulings 6–8):** COUNSELOR writes Amendment A with the evidence. The amendment carries these decisions:
- `-g` in the `cmake` rows, only if names do not resolve;
- the OQ8 row key, table type and unit;
- the S4 wrapper name and the manifest table that `sample.lua` reads.

ARCHITECT rules on Amendment A. S4 code starts only after that ruling.

### Step 13: S4 implementation (after Amendment A)
**Scope:** `~/.carol/skills/trace/sample.lua`, the new wrapper in `~/.carol/skills/trace/`, `~/.carol/skills/trace/SKILL.md`, and the EVE manifests if `-g` is ruled.
**Action:** Implement exactly what Amendment A rules.
- The text output stays (S4.3).
- Share rows keep their own unit (S4.2).
**Validation:** One Where-lane epoch under scout writes a samples file that scout reports.

### Step 14: Audit
@Auditor sweeps Steps 1–13 once. COUNSELOR filters each finding (Design by Contract Filter) and resolves it in this sprint.

### Step 15: Documentation pass (post-audit)
Doxygen for the new and changed scout headers, in cast's doxygen text where the code is a copy. Zero warnings.

## Verification (end to end)

1. Step 7 cases: each matches the Step 4 table.
2. Step 8: `scout <abs>/replay.md` from the EVE root builds and measures in one call, with exit 0.
3. Step 10: the live 100M run gives exit 0, and the `system` row is compared with `time seq`.
4. Step 11: the EVE root `compile_commands.json` holds `JAM_USING_PROFILER`.
5. Step 13: one Where-lane epoch gives a scout report.

## BLESSED Alignment

- **B:** scout owns every child process it starts: the toolchain rows and the epoch command. Each child is reaped before its exit code is read (`jam::Subprocess`).
- **L:** No flag and no word beyond cast's. The Toolchain is copied per ruling 3. No helper exists without a cast sibling.
- **E:** Every failure names its cause through one writer (`printError`). A missing manifest gets its name. The commands are verbatim argv.
- **S (SSOT):** One manifest holds the build, the binary and the measurement. Message texts live in `text.md`, and the help file name lives in `files.md`. The Toolchain copy is ARCHITECT's ruled exception to "second copy" (ruling 3).
- **S (Stateless):** scout keeps no build state between runs.
- **E (Encapsulation):** The product manifest (`project-info.md`) holds no measurement data (ruling 11). nvim reads the benchmark manifests for the compile section only.
- **D:** One command, one row order, then the epochs. The report states the build type and the dropped count.

## Risks / Open Questions

None open before approval. The Step 12 stop is ARCHITECT's ruling 8 (*"Decide after verify"*). It is not an open question of this plan.

## Amendment A — Step 12 evidence (2026-10-28)

### Verified facts

1. whatdbg launch reads only `program` and `cwd` (`whatdbg/Source/WhatdbgHandlers.cpp:26-37`). `Session::launch` takes only `program`, and it sets no arguments on `SBLaunchInfo` (`Session.h:107`, `Session_mac.cpp:278-283`). The trace driver sends no `args` field (`driver.lua:46-49`), and it rejects an unknown field (`driver.lua:101-105`).
2. EVE reads the manifest from argv[1] (`EVETerminalProcessor.cpp:41`, `getCommandLineParameterArray().getReference (0)`; `juce_ApplicationBase.cpp:225-233`). Thus a Where-lane sample of the profile build attaches by pid. Launch mode cannot pass the manifest.
3. Probe: the probe ran `scratchpad/step12/run.sh`. EVE profile build, live manifest copy, attach, 1 sample at t=20270 ms, 36 threads. Outcome `limit`, EVE exit 0.
   - Function names resolve without `-g`. Examples: `jam::TerminalProcessor::process(jam::TerminalTTY&, jam::TerminalBuffer&, jam::TerminalSwap&, jam::Colours&, jam::Function::Map<juce::Identifier, void> const&)`, `EVETerminalProcessor::handleAsyncUpdate()`, `jam::TerminalSwap::writeFromTerminalBuffer(jam::TerminalBuffer&)::$_0::operator()(int) const`.
   - Every frame location is `<no source>`. Without `-g` there is no file:line.
   - One thread frame is `??`. Some system frames are `___lldb_unnamed_symbol_<address>`.
   - Thread names: `JUCE v8.0.14: Message Thread`, `com.apple.NSEventThread`, `JUCE v8.0.14: Timer`, `CVDisplayLink`, and ten threads named `Pool`. The audio thread (it runs `EVEProcessor::processBlock`) and the work-queue threads have an empty name.
4. A `juce::Identifier` row key allows alphanumerics, `_`, `-` and `:` only (`juce_Identifier.h:134-136`). The frame names above hold spaces, `(`, `)`, `&`, `<`, `>`, `,`, `'`, `$`, `[`, `]`.
5. scout merges every non-`run` table of a samples file into one profile. Rows are keyed by row key, and the last table type wins (`Benchmark.cpp:119-130`).
6. The profile build of EVE writes the `run.samples` file at exit, through `jam::Profiler::writeSamples` (`jam_Profiler.cpp:159-161`; `EVEProcessor.cpp:58-68`). A Where-lane epoch that starts the profile build has two writers of one file.
7. Thread ids change on each EVE launch.

### Consequences of ruling 6 (no decision)

- `-g`: names resolve, so the `cmake` rows stay without `-g` (ruling 6: *"verify first"*). The `-g` note in `trace/SKILL.md` changes. Names resolve without `-g`, and file:line needs `-g`.

### ARCHITECT rulings on Amendment A (verbatim)

- Row key: *"what's the most readable, easiest to understand"*.
- Table type and unit: *"SAMPLE / count"*.
- Wrapper and table: *"sample / ## sample"*. The chosen option holds a `program` key for the target.
- Writer order: *"sample.lua writes last"*.
- Then: *"make it CONSISTENT. DESIGN BY CONTRACT"*, with MANIFESTO.md, CODING.md and NAMES.md.

### Contract of the Where-lane samples file (derived from the existing contract)

The contract is `jam::Profiler::writeSamples` (`jam_Profiler.h:93-101`). Its output is the instance below (`step12/samples.md`):

- one `## <TYPE>` table, columns `key | type | value | calls | unit | bins`;
- a count row `dropped | int | <n> | 1 | count | (blank bins)` (`samples.md:1518`);
- a `## RUN` table that holds `buildType` (`samples.md:1521-1526`).

`sample.lua` writes the same shape:

1. One `## SAMPLE` table. It has one row for each function, in the shape of the `dropped` row: `type` `int`, `value` the inclusive sample count of the function in the epoch, `calls` `1`, `unit` `count`, `bins` blank. scout reads a row with no bins as an epoch row only (`Benchmark.cpp:108-109`).
2. Row key: the frame name up to its first `(`. Each other character outside `[A-Za-z0-9:-]` becomes `-`, `_` included. That set is the identifier set (`juce_Identifier.h:134-136`) minus `_`, which markdown parses as emphasis. Examples: `jam::TerminalProcessor::process`, `EVETerminalProcessor::handleAsyncUpdate`, `jam::TerminalSwap::writeFromTerminalBuffer`, `--NSApplication-run-`, `--CFRUNLOOP-IS-CALLING-OUT-TO-A-SOURCE0-PERFORM-FUNCTION--`. This is the most readable form of the name that reads back unchanged at every hop: the samples file, scout, the report, and the baseline. jam's report writer writes keys plain (Step 13 `report.md:79`). Step 13 evidence for the rejected `_`: `-_NSApplication_run_` read back as `-NSApplicationrun`, and `__CFRUNLOOP_IS_CALLING_OUT_TO_A_SOURCE0_PERFORM_FUNCTION__` read back as `CFRUNLOOPISCALLINGOUTTOASOURCE0PERFORMFUNCTION`.
3. Threads merge into one row for each function. Thread ids change on each launch (fact 7), and thread names are empty or repeat (fact 3). The text output keeps the rows for each thread (S4.3).
4. The `INCLUSIVE` tally feeds the rows. A zone covers its callees (`deviceClock` covers `VirtualDevice::clock`, profile skill), so the inclusive count is the same measure. The `LEAF` tally stays in the text output only.
5. The `## RUN` table: the target owns `buildType`, and it writes `## RUN` at exit (`jam_Profiler.cpp:164-174`). `sample.lua` writes last (ruling), and it writes the target's `## RUN` table again, unchanged.

### Contract of the `sample` epoch (derived)

- scout runs `run.command` with one argument, the manifest path (`Benchmark.cpp:39`). `skills/trace/sample` runs `nvim --headless -l sample.lua <manifest>`.
- `## sample` is a `key | type | value` table, the shape of `## run` and `## profiler`. Its keys are `program`, `cwd`, and the existing sample spec fields `threads`, `delayMs`, `intervalMs`, `maxSamples`, `stackDepth`, `topRows`, `pauseTimeoutMs` and `timeoutMs` (`sample.lua:19-31`, `driver.lua:41-44`).
- `mode` is `attach`. `pid` is the pid of `program <manifest>`, started by `sample.lua`.
- `sample.lua` samples until `maxSamples`, the exit of the target, or `timeoutMs`. Then it waits for the target to exit, and it writes `run.samples` (relative to the manifest) last.
- The JSON spec path of the trace skill stays as it is.
- `-g`: none (Consequences above). The trace skill `-g` note states that names resolve without `-g`, and that file:line needs `-g`.

## Amendment B — audit (Step 14) dispositions, rulings, banner

### ARCHITECT rulings (verbatim)

- Audit 13/14: *"Mirror cast"*.
  - `Main.cpp` holds the command line only.
  - A `Validator` derives from `jam::ConfigValidator`. It adds the toolchain-column rule through `addRules`, the JAM extension point (`jam_ConfigValidator.h:20-40`).
  - A `Processor` runs the toolchain rows, the epochs and the report.
  - Names come from cast (`cast/Source/Validator.h`, `Processor.h`). Each object is one header and one `.cpp`.
- Audit 29: *"Split: epoch.lua"*.
  - `skills/trace/epoch.lua` is a module that holds the scout epoch adapter. It follows the shape of `driver.lua`, which is a module with `Driver.main (specPath, driverFields, runSpec)` (`driver.lua:192`).
  - `Epoch.main (manifestPath, sampleFields, runSample)` gets the sampler through its parameters. Thus it needs no circular `require`.
  - `sample.lua` sends a `.json` argument to `Driver.main` and a `.md` argument to `Epoch.main`.
  - The `sample` wrapper does not change.
- Audit 31: *"rename kind→type"*. The nvim AST field and every consumer change. This widens the scope to the consumer files.
- Audit 32: *"fix Subprocess cwd"*, then *"Native launch (TTY way)"*.
  - `jam::Subprocess` launches the child in `workingDir` without a change to the parent's working folder.
  - `juce::ChildProcess` takes no working folder (`juce_ChildProcess.h:77`, `:88`; POSIX child `juce_SharedCode_posix.h:1129-1155`; Windows `lpCurrentDirectory` is `nullptr` at `juce_Threads_windows.cpp:451-453`).
  - Thus `jam::Subprocess` launches natively, the way the JAM TTY does. POSIX uses fork, `chdir` in the child, then `execvp` (`jam_TerminalTTY_posix.cpp:156-170`). Windows uses `CreateProcessW` with `lpCurrentDirectory` (`jam_TerminalTTY_windows.cpp:604-616`).
  - KANJUT has no subprocess module (`kuassa/user_modules`: no match), so no sync applies.
  - History: an earlier `env -C` wrapper was removed in favour of the parent chdir (`cast/carol/SPRINT-LOG.md:1617`).
- Banner: ARCHITECT supplied `scout-banner.sh`, with 8 rows in one colour, 229,229,16.
  - cast's `map::banner` values hold glyph text only (`cast/Source/generated/Generated.cpp:129-136`).
  - Thus `scout/cast/banner.md` holds the 8 glyph rows, with the ANSI codes removed.
  - The keys are the 8 palette names nearest to 0xffe5e510 (`jam/cast/graphics.md`), from top to bottom: `yellowBuzzing`, `holyGrail`, `caduceusStaff`, `candlelight`, `highVoltage`, `lemonCurd`, `sourYellow`, `star`.

### Accepted findings (resolved this sprint)

- Text facts:
  - 1: `-g` lines in the profile skill.
  - 2: sample during a scout run — a Where-lane epoch is its own scout run.
  - 3: `Fine.` on a terminal only.
  - 4: a validation failure can hold more than one line.
  - 5: the HELP example is fenced, as cast fences it.
  - 6: the lane table, "what is rebuilt".
  - 7: the Where-lane `epoch`/`user`/`system` rows include the debugger pauses.
  - 8: the `epoch.lua` wait and the `program` folder.
- scout:
  - 9: a missing baseline file fails with `not found` before the epochs.
  - 11: an argument after the manifest that is not `--help` or `--version` fails with cast's `failFlagUnknown` (`cast/cast/text.md:57`).
  - 16: `Main.cpp` returns `EXIT_SUCCESS` / `EXIT_FAILURE`.
  - 17: the unused `<fcntl.h>` is removed.
  - 18: `ProfileReport.cpp` includes `generated/Generated.h`.
  - 20: `Chars::doubleDash` (JAM) replaces `Id::doubleDash`.
  - 21: `const auto*`.
  - 22: the manifest parameter is `documentFile` everywhere.
  - B4: each row holds one sample for each epoch; an absent row adds 0.
- Lua:
  - 28: the stale comments, in the documentation pass.
  - 30: the unused code-span strip is removed (YAGNI).
  - 31: the `kind`/`handler` names in driver.lua and sample.lua.
  - B1: the key is the name up to the last `(`, never empty.
  - B2: the `## sample` table is validated before launch, and the target is killed and waited on any driver failure.
  - B3: an epoch with outcome `error` or `timeout` exits with a failure code.
  - B5: unknown or missing `## sample` keys fail.
  - B6: the target's `## RUN` records are written back whole.
  - B7: a missing samples file fails with a message.

### Rejected findings (read that refutes each)

- 10, 12: MANIFESTO D, *"An assert at that single ownership boundary is verification of the fix"*. MANIFESTO E, *"Prefer assert over silent fail"*. Both asserts are at the owner.
- 15: cast's `isVersion`/`isHelp` pair has the same shape (`cast main.cpp:429-453`). Ruling 2.
- 19: `getValueTree` is the framework call that types the values (`jam_ConfigDocument.h:47-70`). Its sibling is `jam_Profiler.cpp:159`.
- 23: each `if` gates the result of one step. None encodes a decision table (MANIFESTO L).
- 24: rulings 2 and 3.
- 25: cast's single `@headers` row would include `CMakeLists.txt` in `Generated.h` (`scout/cast/spell.md:43`).
- 26: ARCHITECT, *"also fix this"*, then *"go"*.
- 27: one locator responsibility (`cast.lua:1-6`). A split only for line count is not compliance (MANIFESTO L).
- 31 `Id::node`: a JAM API name, not code that this sprint wrote.

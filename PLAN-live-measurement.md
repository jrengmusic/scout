# PLAN: Live Measurement — scout as the runner and statistics lane

**RFC:** `RFC-live-measurement.md` (R1–R13)
**Date:** 2026-10-28
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8, JAM modules (LANGUAGE.md C++/JUCE: one object, one translation unit)

Step 0 copies this file to `scout/PLAN-live-measurement.md` (goplan output location). Plan mode allows only this harness path.

## Context

scout cannot measure the gap between EVE and endless (RFC §1, §2.7). Three facts cause this:
1. Its workloads are compiled into it (`Main.cpp:95`).
2. A zone measures only time inside its own process (`jam_Profiler.cpp:7-19`).
3. Its only workload replays from memory (`TerminalBenchmark.cpp:51-69`).

After this plan:
- scout is a universal runner and statistics lane. It launches any target per epoch. It reads the samples that the target writes. It reports the statistics.
- The target owns its own configuration and its zones.
- `jam_unit_test` leaves JAM and KANJUT (R13).

## ARCHITECT rulings (verbatim, this session)

| Topic | Ruling |
|---|---|
| R8 epoch | *"Form 1: EVE runs and quits"* |
| R7 replay | *"if this still on EVE toolchain, just make the isomorphic schema Builds/Debug/ Builds/Release/ Builds/whatever/"* |
| project-info.md | *"anything related to profiler belong to profiler NOT at toolchain. project-info.md is toolchain manifest. NOT PROFILER."* |
| Universal | *"NO HARDCODED pattern … not all application need seq, it could be anything, it could feed anything"*; *"how to design this more UNIVERSAL and GENERAL for any application?"* |
| Grammar | *"Config table, one workload"* |
| Feed | *"Manifest path as argument"* |
| `## profiler` | *"No, the target's table"* (the raw-file path key moves to `## run`) |
| R10 | *"Report the drop count"*; key *"dropped"* |
| R5 | *"Per-call rows plus a percentile"*; *"p99.9 then."*; per-call rows: *"Second table, CALLS suffix"* |
| Names | `percentile` (accepted by "next question"); `user`, `system` (*"user, system"*) |
| R2 writer | *"jam_profiler adds jam_markdown"* |
| jam_subprocess | *"just extend it? what's the issue?"* then *"go"* after the reap analysis |
| R13 | *"jam_unit_test should just moved to scout, so JAM and KANJUT cleaner"* (RFC §0 item 15) |
| Builds | *"profiling with scout is granted with build permission"* (RFC §0 item 10) |

## Overview

scout reads `## run`. It launches `command` with the manifest path as its one argument, once per epoch, through `jam::Subprocess`. It reads the child's user and system time with `getrusage`. It reads the samples file that the target writes through `jam::Profiler::writeSamples`. It writes one report: a per-epoch table, and a per-call table `## <NAME>_CALLS` with a p99.9 `percentile` column. EVE reads its own `## profiler` table, runs the command in its terminal, quits when the shell exits, and writes the samples.

## Language / Framework Constraints

- LANGUAGE.md C++/JUCE: one object is one header and one `.cpp`. `ProfileReport` and `Statistics` keep this shape in `scout/Source`.
- JAM modules: no includes in submodule files (CODING Header Discipline). scout `Source/` files follow the scout pattern: `#include <JuceHeader.h>` (`TerminalBenchmark.h:1-3`).
- `getrusage (RUSAGE_CHILDREN)` is POSIX (RFC R3). Windows has no equivalent call in JUCE. The `user` and `system` rows exist under `#if JUCE_MAC or JUCE_LINUX`, the platform-guard family of `jam_TerminalTTY_posix.h:44`. This follows R3 as the RFC states it.
- Generated files are never hand-edited. Vocabulary changes go through `cast` (`jam/.claude/CLAUDE.md:106`; scout `CLAUDE.md`).
- KANJUT changes only through `cast --sync`. Sync never deletes a file. A target `kuassa_*` directory that no row declares is fatal (`cast/Source/HELP.md:69`). KANJUT files of a deleted module are deleted by hand before the sync (Sprint 179 precedent, `jam/carol/SPRINT-LOG.md:12`, `:56`).

## Dependency & API Inventory

| Owner | API this plan uses | Citation |
|---|---|---|
| jam_subprocess (provision; one user, cast) | `jam::Subprocess::launch (argv, workingDir, onComplete)`, `kill()` | `jam_Subprocess.h:91-100`; `cast/project-info.md:125`; `jam/user-modules-info.md:151` |
| JUCE | `ChildProcess::waitForProcessToFinish` (reaps through `isRunning` → `waitpid`) | `juce_ChildProcess.cpp:61-75`; `juce_SharedCode_posix.h:1176-1194` |
| JUCE | `JUCEApplicationBase::getCommandLineParameterArray()` | `juce_ApplicationBase.h:253` |
| JUCE | `JUCEApplicationBase::quit()` | `juce_ApplicationBase.h:247` |
| JUCE | `File::replaceWithText`, `getChildFile`, `getCurrentWorkingDirectory` | `juce_File.h:802`; `Main.cpp:62`, `:68`, `:100` |
| jam_markdown | `ConfigDocument::parse`, `getValueTree`, `toDocument`, `getValueTypes` (`numbers`); `MarkdownWriter::getText` | `jam_ConfigDocument.h:32`, `:45`, `:70`, `:83`; `jam_MarkdownWriter.h:31-54` |
| jam_profiler | `Profiler` (`Instance<Profiler>`), `Zone`, `prepare`, `drain`, `getDurations`, `getNanoseconds` | `jam_Profiler.h:4-43`; `jam_Instance.h:113` |
| jam_terminal | `TerminalTTY::open (… shell, args …)`, `read` returns −1 on EOF | `jam_TerminalTTY.h:89-91`, `:111-113` |
| jam_terminal | `TerminalProcessor::applyEvents` → `events.get (Id, value)` | `jam_TerminalProcessor.cpp:84-94` |
| jam_terminal vocabulary | `Id::shellExited` (declared, no C++ use) | `jam/cast/terminal.md:53`; `generated/jam_Terminal.h:47` |
| jam_core | `Id::value`, `min`, `max`, `baseline`, `name`, `type`, `text` | `jam/cast/core.md:51-142` |
| jam_markdown / jam_style vocabulary | `Id::numbers`, `marker`, `info`; `Id::mermaid` | `jam/cast/markdown.md:57-62`; `jam/cast/style.md:42` |

Established patterns this plan follows:
- Zone: `const jam::Profiler::Zone zone { Id::<key> };` under `#if JAM_USING_PROFILER` (`jam_TerminalProcessor.cpp:33-35`; `jam_TerminalSwap.cpp:66-68`).
- Report text: `ConfigDocument::toDocument` → `MarkdownWriter::getText` (`jam_ProfileReport.cpp:24-31`).
- Table type from the project: `Id::toType (juce::Identifier { ProjectInfo::projectName })` (`EVEProcessor.cpp:67`).
- Standalone-only branch: `wrapperType == juce::AudioProcessor::wrapperType_Standalone` (`EVEProcessor.cpp:29-30`; `EVETerminalProcessor.cpp:79`).
- Build-flag definition: `$<$<BOOL:${CAST_PROFILE}>:JAM_USING_PROFILER=1>` (`eve/project-info.md:382`). `CAST_PROFILE` has no option declaration. It is a `-D` cache variable only (`eve/project-info.md:146`; `eve/CMakeLists.txt:338`).
- Static free functions in `Main.cpp` (`Main.cpp:7-86`).
- Benchmark verb family, deleted with `Benchmark`, reused for the runner: `runEpoch`, `addEpoch` (`jam_Benchmark.h:13`, `:20`).

## New names (each from a family; Rule 5 source given)

| Name | Where | Family / source |
|---|---|---|
| `exitTimeoutMilliseconds` | `jam_Subprocess.cpp` | `stopTimeoutMilliseconds` (`jam_Subprocess.cpp:181`) |
| `Profiler::writeSamples` | jam_profiler | `write` family (`TerminalSwap::writeFromTerminalBuffer`); `samples` (`## run` key, ruled) |
| `Profiler::dropped` (member), `Id::dropped` | jam_profiler | ARCHITECT: *"dropped"* |
| `Id::unit` | jam_profiler | `unitLabel` (`jam_ProfileReport.cpp:80`) |
| `Id::buildType` | jam_profiler | CMake `CMAKE_BUILD_TYPE` (`eve/project-info.md:146`) |
| `nanosecondUnit`, `countUnit`, `debugBuildType`, `releaseBuildType` | `jam_Profiler.cpp` constants | `unitLabel` (`jam_ProfileReport.cpp:80`); `CMAKE_BUILD_TYPE` values |
| `jam/cast/profiler.md`, `generated/jam_Profiler.h/.cpp`, aliases `@profiler`, `@jam_Profiler`, `@jam_ProfilerSource` | JAM cast | `cast/unit_test.md` ↔ `generated/jam_UnitTest.*` ↔ `@unit_test`, `@jam_UnitTest` (`jam/cast/spell.md:49-50`, `:67`) |
| `Id::terminalRead`, `Id::ttyBytesReady` | jam_terminal zones | zone family `terminalProcess`, `swapLiveLines` (`jam/cast/terminal.md:54-56`); `onBytesReady` (`jam_TerminalTTY.h:57`) |
| `Statistics::getPercentile`, `Statistics::percentileRank` | scout | `getMedian`, `getMinimum`, `getMaximum`, `percentScale` (`jam_Statistics.h:7-14`) |
| `ProfileReport::addPercentile` | scout | `addComparison`, `addChart` (`jam_ProfileReport.h:12-13`) |
| `runEpoch`, `addEpoch`, `addChildTime` | scout `Main.cpp` statics | `Benchmark::runEpoch`, `addEpoch` (`jam_Benchmark.h:13`, `:20`); `add` verb |
| `Id::percentile`, `Id::user`, `Id::system`, `Id::command` | scout identifiers | ARCHITECT rulings; `command` = `ChildProcess::start` parameter (`juce_ChildProcess.h:77`) |
| `Id::profiler` | EVE identifiers | ARCHITECT: *"scout manifest profiler table"* |
| `Id::shell`, `Id::args` | EVE identifiers (only when no JAM id exists) | `TerminalTTY::open` parameters (`jam_TerminalTTY.h:89-91`) |
| `Id::deviceClock` | EVE identifiers (zone) | zone family (`terminal.md:54-56`); `VirtualDevice::clock` (`jam_VirtualDevice.cpp:76`) |
| `replay` argument, `Builds/Replay`, `CAST_REPLAY`, `eveUsingReplay`, `EVE_USING_REPLAY` | EVE toolchain / define | `profile`, `Builds/Profile`, `CAST_PROFILE`, `jamUsingProfiler`, `JAM_USING_PROFILER` (`eve/project-info.md:144-148`, `:382`) |
| `eve/benchmark/live.md`, `replay.md`, `*-samples.md`, `*-report.md` | manifests (data) | `eve.md` / `eve-report.md` (scout root); ARCHITECT q7 *"eve/benchmark/scout-report.md"* |

## Validation Gate

COUNSELOR validates each step before the next one. The checks are MANIFESTO.md (BLESSED), NAMES.md, `~/.carol/CODING.md` and the locked decisions of this PLAN: no deviation and no scope drift. COUNSELOR reads every changed file at the reported lines. @Auditor runs ONCE, after the final code step, and covers the whole sprint. It never runs per step.

## Steps

### Step 0: Plan file
**Scope:** `scout/PLAN-live-measurement.md`
**Action:** COUNSELOR copies this plan to the project root.
**Validation:** The file content equals this plan.

### Step 1: jam_subprocess — reap before the exit code
**Scope:** `jam/jam_subprocess/subprocess/jam_Subprocess.cpp`
**Action:**
1. Add `static constexpr int exitTimeoutMilliseconds { -1 };` next to `stopTimeoutMilliseconds` (`:181`).
2. In `Worker::run`, between `readStream (accumulation);` (`:69`) and `getExitCode` (`:71`), call `process.waitForProcessToFinish (exitTimeoutMilliseconds)`. Assert the result with `jassert`.
**Validation:** The completion fires after the reap (`juce_ChildProcess.cpp:61-75`). The kill path still ends: `terminate()` sends SIGKILL, so `isRunning` becomes false. cast `Toolchain::runProcess` (`cast/Source/Toolchain.cpp:15-41`) is unchanged and now gets the real exit code. No bail-out. Named constant. No other file changes.

### Step 2: jam_terminal — shell exit event and wake-path zones (R8, R9)
**Scope:** `jam/jam_terminal/processor/jam_TerminalProcessor.cpp`, `jam/jam_terminal/tty/native/jam_TerminalTTY_posix.cpp`, `jam/cast/terminal.md`, regenerated `jam/generated/jam_Terminal.*`
**Action:**
1. Read the owner contract of `jam::Function::Map::get` first: does `get` need a registered key? Report the file:line.
2. In `TerminalProcessor::process`, when `numRead < 0`, call `events.get (Id::shellExited, true)` (the `applyEvents` form, `:84-94`).
3. Add the zone `Id::terminalRead` around the `tty.read` call (`:37`). Use the zone pattern of `:33-35`.
4. Add the zone `Id::ttyBytesReady` around `tty.onBytesReady()` in the macOS read handler (`jam_TerminalTTY_posix.cpp:213-223`).
5. Add `terminalRead` and `ttyBytesReady` rows to `jam/cast/terminal.md` (`:54-56` family). Run `cast cast/spell.md` per `jam/.claude/CLAUDE.md:106`.
**Validation:**
- The `shellExited` event fires only on −1 (`jam_TerminalTTY.h:111-113`).
- The zones are under `#if JAM_USING_PROFILER`.
- Generated files come only from cast.
- R9 item 4 (the Q_B writer) is already timed. `EVETerminalProcessor::writePendingData` calls `swap.writeFromTerminalBuffer` (`EVETerminalProcessor.cpp:120-123`), which holds the `swapHistoryLines` zone (`jam_TerminalSwap.cpp:92-96`). That RFC line is corrected. No new zone.

### Step 3: scout — copy ProfileReport and Statistics (R13 step 1, R5)
**Scope:** new `scout/Source/ProfileReport.h/.cpp`, `scout/Source/Statistics.h/.cpp`
**Action:**
1. Copy `jam_unit_test/report/jam_ProfileReport.*` and `statistics/jam_Statistics.*` into `scout/Source`. Drop the `namespace jam` wrapper. Add `#include <JuceHeader.h>` (`TerminalBenchmark.h:1-3` pattern). Prefix jam types with `jam::`.
2. `Statistics`: add `static constexpr double percentileRank { 99.9 };` and `double getPercentile() const noexcept;`. The method uses linear interpolation over `sorted` at the position `percentileRank / percentScale × (size − 1)`. At rank 50 this equals `getMedian` (`jam_Statistics.cpp:14-20`).
3. `ProfileReport`:
   - `getProfile` writes `Id::unit` on each row from a unit map that the caller passes.
   - Add `static void addPercentile (juce::ValueTree& profile, const jam::HashMap<juce::Identifier, juce::Array<juce::var>>& samples);`. It sets `Id::percentile` on each row.
   - `addChart` writes one chart per unit in the table, with that unit as the y-axis label. This replaces the fixed `unitLabel` (`jam_ProfileReport.cpp:80`).
**Validation:**
- One object, one translation unit.
- No `namespace detail`. No out-parameters. Alternative tokens are used.
- `getPercentile` is `const` and `noexcept`.
- No string `==`.
- jam stays untouched in this step.

### Step 4: JAM — profiler vocabulary, delete jam_unit_test (R13 step 4, R2)
**Scope:** `jam/cast/profiler.md` (new), `jam/cast/unit_test.md` (delete), `jam/cast/spell.md`, `jam/user-modules-info.md`, `jam/jam_unit_test/` (delete), `jam/generated/jam_UnitTest.*` (delete), regenerated `jam/generated/jam_Profiler.*`, `jam/jam_profiler/jam_profiler.h/.cpp` (module declaration)
**Action:**
1. Create `jam/cast/profiler.md`. Mirror `jam/cast/unit_test.md:1-43`, with the `@brief` "Identifier vocabulary owned by jam_profiler." It holds these rows:
   - `calls`, `samples` (moved from `unit_test.md:32`, `:34`)
   - `manifest`, `run` (moved from `scout/cast/identifiers.md:18`, `:20`, because `writeSamples` reads `## run`)
   - `unit`, `dropped`, `buildType` (new)
2. Before you add an id, search `jam/cast/*.md` (md text search is allowed) for an existing definition. Report each result.
3. In `jam/cast/spell.md`, change `@jam_UnitTest` / `@jam_UnitTestSource` (`:49-50`), `@unit_test` (`:67`), the header block (`:296-300`) and the two output blocks (`:1409-1419`) into `@jam_Profiler`, `@jam_ProfilerSource`, `@profiler` and `jam_Profiler.h`. Delete the `@jam_unit_test` rows (`:177-178`) and the module blocks (`:3631-3644`).
4. In `jam/user-modules-info.md`:
   - Delete the `jam_unit_test` module row (`:164-169`) and its submodule rows (`:1126-1133`).
   - Replace the `cast | unit_test.md | kernel` row (`:1271`) with `cast | profiler.md | kernel`.
   - Add `jam_markdown` to the `jam_profiler` dependencies (`:144-145`).
5. The `jam_profiler` module files include `<generated/jam_Profiler.h>` and `<generated/jam_Profiler.cpp>`, in the form of `jam_unit_test.h:30` and `jam_unit_test.cpp:2`. Read the spell module-declaration block to see whether cast writes these lines. If it does, cast writes them.
6. Delete `jam/jam_unit_test/`, `jam/cast/unit_test.md` and `jam/generated/jam_UnitTest.*`. Run `cast cast/spell.md`.
**Validation:**
- No id is defined twice.
- `epoch`, `zone`, `errorPercent`, `change`, `probability` and `profile` are absent from JAM. Step 6 adds them to scout.
- The `jam_profiler.h` dependency block lists `jam_markdown`.
- The generated files come from cast.
- RFC order holds: this step runs after Step 3.

### Step 5: jam_profiler — dropped count and the samples write (R2, R4, R10)
**Scope:** `jam/jam_profiler/profiler/jam_Profiler.h/.cpp`
**Action:**
1. Add the member `std::atomic<int> dropped { 0 };`. In `addEntry` (`:21-38`), add `dropped.fetch_add (1, std::memory_order_relaxed)` on both drop paths: the slot index past `fifos.size()`, and a full ring (`blockSize1 == 0`). Keep the asserts. `prepare` and `clear` reset `dropped`.
2. Add `void writeSamples (const juce::File& manifestFile, const juce::Identifier& tableType) const;`. It works as follows:
   1. It parses `manifestFile` with `ConfigDocument::parse`. It reads `## run` `samples` through `getValueTree (Id::manifest)` and the child `Id::toType (Id::run)` (`Main.cpp:45-51` pattern). It resolves the path against `manifestFile.getParentDirectory()`.
   2. It builds one table of type `tableType`.
   3. It adds one row per zone in `durations`:
      - `value`: the sum, in ns
      - `calls`: the count
      - `samples`: each call in ns, through `getNanoseconds`
      - `unit`: `nanosecondUnit` ("ns")
   4. It adds the row `dropped`, with `value` = count and `unit` = `countUnit` ("count").
   5. It adds the row `buildType`, with `value` = `debugBuildType` ("Debug") when `JUCE_DEBUG`, else `releaseBuildType` ("Release").
   6. It writes the text with `ConfigDocument::toDocument` → `MarkdownWriter{}.getText` (`jam_ProfileReport.cpp:24-31` pattern), then `replaceWithText`, and asserts the write.
**Validation:**
- `addEntry` stays allocation-free and lock-free on a deadline thread (MANIFESTO Deadline threads). An atomic increment only.
- `writeSamples` runs off the deadline thread (destructor).
- Named constants.
- The verb `write` matches its family.
- The caller drains. `writeSamples` does not drain (one job).

### Step 6: scout — the runner (R1, R3, R5, R6, R13 step 2)
**Scope:** `scout/Source/Main.cpp`, `scout/cast/identifiers.md`, regenerated `scout/Source/generated/Identifiers.*`, `scout/project-info.md`, delete `scout/Source/TerminalBenchmark.*`, move `scout/Source/TerminalMemoryTTY.*` to `eve/Source/`
**Action:**
1. Identifiers:
   - Delete the rows `manifest`, `run` (moved to JAM in Step 4), `benchmark` and `seed` (NAMES Rule 6 bans `seed`, `NAMES.md:409`).
   - Keep `epochs` and `output`.
   - Add `epoch`, `errorPercent`, `change`, `probability`, `profile` (moved from JAM), and `percentile`, `user`, `system`, `command`.
   - Run cast.
2. `project-info.md`: delete the `jam_unit_test` row (`:143`) and `usingProfiler` (`:173`). Add the `jam_subprocess` user-module row (description from `jam_subprocess.h:9`).
3. Move `TerminalMemoryTTY.h/.cpp` to `eve/Source/` unchanged. Delete `TerminalBenchmark.h/.cpp`. Delete `TerminalBenchmark terminalBenchmark;` (`Main.cpp:95`), `getTests` (`:7-15`) and `getProfiles` (`:28-41`).
4. `runManifest` keeps the parse and the validation (`:45-48`). It reads `## run`: `epochs`, `command`, `samples`, `output`, `baseline`. Paths resolve against the manifest folder (`:62`).
5. `static int runEpoch (…)` launches `{ commandFile, manifestFile }` through `jam::Subprocess::launch` in the manifest folder. It waits on a `juce::WaitableEvent` (`Toolchain.cpp:15-34` pattern) and returns the exit code. Its two timings:
   - the epoch time: `Time::getHighResolutionTicks()` before the launch and after the completion
   - `getrusage (RUSAGE_CHILDREN)` before and after (under `#if JUCE_MAC or JUCE_LINUX`)
6. `static void addChildTime (…)` adds `user` and `system` in ns from the two `rusage` reads.
7. `static void addEpoch (…)` parses the samples file:
   - It adds each row's `value` to the epoch samples.
   - It adds each row's `samples` (parsed with `getValueTypes().get (Id::numbers, …)`, `jam_ProfileReport.cpp:61-63`) to the per-call samples.
   - It sets `calls` and the unit.
   - It copies `buildType`.
8. A non-zero exit code prints the command path to stderr and returns `EXIT_FAILURE`. A missing samples file prints its path and returns `EXIT_FAILURE` (the `Main.cpp:80-81` pattern).
9. After the epochs:
   - `getProfile` builds the per-epoch table. Its rows are the target rows plus `epoch`, `user` and `system`.
   - `getProfile` and `addPercentile` build the per-call table `## <NAME>_CALLS`, from the table type and `Id::calls` through `Id::toType`.
   - `getText` runs with the baseline, and the report is written (`:67-81`).
**Validation:**
- scout reads no key of the target's `## profiler` table.
- No hardcoded workload.
- Each function is 30 lines or fewer, with 3 branches or fewer.
- `jam::Subprocess` is used, not hand-rolled.
- Fail fast on a child failure.
- `seed` is gone.
- Statics follow the `Main.cpp` pattern.
- No new name outside the table above.

### Step 7: EVE — Form 1 run-and-quit, samples write, replay row (R2, R7, R8, R9, R13 step 3)
**Scope:** `eve/Source/EVEProcessor.cpp/.h`, `eve/Source/EVETerminalProcessor.cpp/.h`, `eve/Source/TerminalMemoryTTY.h/.cpp`, `eve/cast/identifiers.md`, `eve/project-info.md`, regenerated `eve/CMakeLists.txt` and `eve/Source/generated/*`
**Action:**
1. `project-info.md`:
   - Delete the `jam_unit_test` row (`:281-283`).
   - Add three `replay` toolchain rows after `profile` (`:144-148`): `cast ../jam/cast/spell.md`; `cmake -S . -B Builds/Replay -G Ninja -DCMAKE_BUILD_TYPE=Release -DCAST_SIGN=OFF -DCAST_PROFILE=ON -DCAST_REPLAY=ON`; `ninja -C Builds/Replay`.
   - Add the define row `eveUsingReplay | $<$<BOOL:${CAST_REPLAY}>:EVE_USING_REPLAY=1> | Replay TTY, replay builds only` after `:382`.
   - Run cast.
2. Identifiers: add `profiler`, `deviceClock`, and `shell` and `args` only if Step 4's search finds no JAM id.
3. `EVEProcessor::~EVEProcessor` (`:50-73`): replace the `ProfileReport` block with `profiler.drain();` and `profiler.writeSamples (manifestFile, Id::toType (juce::Identifier { ProjectInfo::projectName }))`. This runs in the standalone wrapper only. `manifestFile` is `juce::File::getCurrentWorkingDirectory().getChildFile (juce::JUCEApplicationBase::getCommandLineParameterArray()[0])` (`Main.cpp:100` form).
4. Add the zone `Id::deviceClock` in `onBytesReady` around `device.clock (0)` (`EVEProcessor.cpp:34`).
5. `EVETerminalProcessor::prepare` (`:253-260`): under `JAM_USING_PROFILER` in the standalone wrapper, `shell` and `args` come from the manifest table `Id::profiler`, with no integration arguments and no integration environment. The manifest is the same `manifestFile`.
6. `registerEvents` (`:64-71`): add `Id::shellExited` with `add<bool>`. In the profile standalone run it calls `juce::MessageManager::callAsync ([] { juce::JUCEApplicationBase::quit(); })`. Use the Step 2 `Function::Map::get` contract to decide whether every build registers it.
7. `TerminalMemoryTTY`:
   - Its constructor takes the platform TTY (`EVETerminalProcessor::createTTY` platform branch, `:55-62`).
   - `open` records the shell output through that TTY until `read` returns −1. Members: `tty`, `chunk`, `fixture`, `recorded`, `recordTimeoutMs`, from `TerminalBenchmark.h:15-28` and `.cpp:13-44`. Then it builds the memory stream over `fixture`.
   - `read` returns −1 at the end of the stream. This is the `TerminalTTY::read` contract (`jam_TerminalTTY.h:111-113`); today it returns 0.
   - `createTTY` returns `TerminalMemoryTTY` under `EVE_USING_REPLAY`.
**Validation:**
- No `jam_unit_test` reference remains.
- The shell and args come from the target's table, so EVE is a "feed anything" target.
- Quit goes through the existing JUCE API.
- Standalone-only branches use the established `wrapperType` test.
- Zones are under `JAM_USING_PROFILER`.
- No bail-out guards.
- A plugin build is unchanged.

### Step 8: KANJUT — mirror through sync (R13 step 4)
**Scope:** `kuassa/user_modules/kuassa_unit_test/` (delete), `kuassa/user_modules/generated/kuassa_UnitTest.*` (delete), `kuassa/user_modules/cast/unit_test.md` (delete), then `cast --sync`
**Action:**
1. Delete the three KANJUT paths by hand (`HELP.md:69`: an undeclared `kuassa_*` directory is fatal).
2. Run `cast --sync /Users/jreng/Documents/Poems/dev/jam /Users/jreng/Documents/Poems/kuassa/user_modules`.
3. Run it a second time.
**Validation:**
- The second run writes 0 lines (the pass, `HELP.md:75`).
- `kuassa_profiler` lists `kuassa_markdown`.
- No `unit_test` row remains in KANJUT `user-modules-info.md` or `spell.md`.

### Step 9: Manifests in the measured project (R6, R1)
**Scope:** `eve/benchmark/live.md`, `eve/benchmark/replay.md` (new, COUNSELOR data); delete `scout/eve.md`, `scout/eve-report.md`
**Action:** COUNSELOR writes both manifests:
- `## run` (`key | type | value`):
  - `epochs` = 11
  - `command` = the EVE Standalone binary under `../Builds/Profile` or `../Builds/Replay` (the path is read from the build output)
  - `samples` = `live-samples.md` or `replay-samples.md`
  - `output` = `live-report.md` or `replay-report.md`
  - `baseline` empty
- `## profiler`:
  - `shell` = `/bin/zsh`
  - `args` = `-c "seq 1 100000000"` for live, `-c "seq 1 1000000"` for replay (the size of `fixtureLineCount`, `TerminalBenchmark.h:13`)

The old scout-root manifest and report no longer validate after Step 6. Delete them.
**Validation:** The manifest is the only source of the command, N and the samples path. On macOS, the shell starts through `login … -c "exec -l <shell> <args>"` (`jam_TerminalTTY_posix.cpp:113-136`), so the quoted `-c` argument reaches zsh whole.

### Step 10: Builds (ARCHITECT's grant, RFC §0 item 10)
**Scope:** scout, EVE `profile` and `replay` rows, cast (jam_subprocess user)
**Action:**
- scout: `cast cast/spell.md --no-sign` (`SKILL.md:135`).
- EVE: `cast cast/spell.md --profile` and `cast cast/spell.md --replay`.
- cast: rebuild, because jam_subprocess changed.

Compiler errors go back to Engineer against the locked step that owns them.
**Validation:** All builds are clean. No warning comes from a changed file.

### Step 11: Audit
**Action:** @Auditor sweeps Steps 1–9 once, against CONTRACT and this PLAN. COUNSELOR validates each finding (Design by Contract Filter) and resolves it in this sprint.

### Step 12: Documentation pass (post-audit, Code Hygiene)
**Scope:** doxygen for the changed public API (`Profiler::writeSamples`, `dropped`; `Statistics::getPercentile`; `ProfileReport::addPercentile`; `TerminalMemoryTTY`), `scout/Source/HELP.md` (the new `## run` keys and the report columns `unit` and `percentile`, plus the `_CALLS` table), and `scout/CLAUDE.md` Key documents (eve.md and eve-report.md are gone).
**Validation:** Zero doxygen warnings. Docs live in the header only. No PLAN or RFC text appears in a comment.

## Verification (end to end, one measurement at a time — R12)

1. `scout /Users/jreng/Documents/Poems/dev/eve/benchmark/replay.md` exits with `EXIT_SUCCESS`. `replay-report.md` contains:
   - a `## EVE` table with `terminalProcess`, `swapLiveLines`, `swapHistoryLines`, `terminalRead`, `deviceClock`, `epoch`, `user`, `system`, `dropped` and `buildType = Release`
   - a `## EVE_CALLS` table with `percentile`
   - one chart per unit
2. `scout …/live.md` (about 12 minutes, RFC §8 item 11). The `dropped` median is 0, or the report shows the loss (R10).
3. Run the R3 check once: the `user` and `system` medians of the live report against `time seq 1 100000000` in EVE. A `login` grandchild that is missing from `RUSAGE_CHILDREN` shows as near-zero rows (RFC R3). Record the result.
4. Run the R11 check once: a whatdbg `sample` attach on the running `Builds/Profile` EVE (trace skill). If the frames show no function names, add `-g` / `/Z7` for the `profile` configuration (R11).
5. cast: run a toolchain row that fails. cast reports `failToolchain` (`Toolchain.cpp:36-38`).
6. A run with the replay report as `baseline` writes `baseline`, `change` and `probability`.

## BLESSED Alignment

- **B:**
  - `jam::Subprocess` owns the child.
  - scout reaps every child before the report.
  - The Profiler owns its drop count.
  - EVE owns its manifest read.
- **L:**
  - `runManifest` splits into `runEpoch`, `addEpoch` and `addChildTime`, one responsibility each.
  - The replay is a build row, not a second program (ARCHITECT).
- **E:**
  - The unit is explicit on every row.
  - The `"ns"` default is gone (`jam_ProfileReport.cpp:80`).
  - The drop count makes the silent drop loud (MANIFESTO E, *"Prefer assert over silent fail"*).
  - A child failure fails fast.
- **S (SSOT):**
  - The command, N and the samples path live only in the manifest.
  - `project-info.md` holds toolchain rows only (ARCHITECT).
  - One statistics implementation, in scout.
  - One samples writer, `jam_profiler`.
  - `run` and `samples` are declared once, in `jam_profiler`.
- **S (Stateless):**
  - scout holds the samples maps for one run only (transient).
  - `writeSamples` is `const`.
  - Targets keep no profile state beyond the existing Profiler.
- **E (Encapsulation):**
  - scout never reads target keys.
  - Each target reads its own table.
  - The layers stay unidirectional: scout → jam_profiler → jam_markdown.
- **D:** Each epoch is a fresh process from the same manifest. The reap makes `getrusage` deterministic per epoch.

## Amendment A — `buildType` lives in a `run` table (execution correction)

- Discrepancy: Steps 5.2.5 and 6.7 put `buildType` as a row of the statistics table. `getText` sends every row with a baseline row to `addComparison`, which asserts `baselineMedian > 0.0` (`scout/Source/ProfileReport.cpp:14-19`, `:77`). `addChart` builds `juce::Identifier { unit }` for every row (`:89`), and a row without a unit gives an empty identifier. A text row breaks both.
- Correction (no new name; `Id::run` is in the Step 4 vocabulary):
  - `writeSamples` writes two tables: `tableType`, with the zone rows and `dropped`, and `run`, with the property `buildType`.
  - scout copies `buildType` into a `run` table of the report.
  - Every row of a benchmark table is then a statistic with a unit.

## Amendment B — `shellExited` is a Model-lane mode (execution correction)

- Discrepancy:
  - Step 2 fires `events.get (Id::shellExited, true)` once, and Step 7 quits through `MessageManager::callAsync` inside the event.
  - The event runs on the clock thread, inside `TerminalProcessor::process`. MANIFESTO *Deadline threads*: *"does not allocate, lock, wait, or make a blocking call"*.
  - `Map::get` throws on a key nobody registered (`jam_Function.h:301`, `:311`).
  - EVE moves terminal modes from the clock thread through the Model lane: `state.storeValue` in the event setters (`EVETerminalProcessor.cpp:64-71`), flushed on the message thread (`:87`). MANIFESTO E: *"If state flows through the Model, do not invent a parallel channel."*
- Correction:
  - `TerminalProcessor::process` emits `events.get (Id::shellExited, bool { numRead < 0 })` next to `applyEvents`, inside the `numRead <= 0` branch. That is the `applyEvents` mode form (`jam_TerminalProcessor.cpp:84-94`). The value is 0 while the shell lives and 1 after EOF. A restored 1 clears on the first idle read.
  - EVE adds `Id::shellExited` to the bool list of `registerEvents` (`EVETerminalProcessor.cpp:65-66`), so every build registers it.
  - The profile standalone run quits on the message thread when the flushed `shellExited` value changes to 1. It uses the Model's existing change notification.

## Amendment C — corrections during execution and audit (names from families)

- **scout `Benchmark` object** (`scout/Source/Benchmark.h/.cpp`).
  - Cause: the static layout of Step 6 threaded the same four maps through five functions. `runEpoch` grew to 47 lines and `runEpochs` to 38. MANIFESTO L: *"Crossing them means stop and ask why. The answer is almost always wrong decomposition."*
  - The run's sample state is one concept, the deleted `jam::Benchmark` (`jam_Benchmark.h:4-25`: samples, `runEpoch`, `addEpoch`, `getProfile`).
  - Names: `Benchmark`, `runEpoch`, `addEpoch`, `addRows`, `addChildTime`, `getProfiles`. All come from that family.
  - Main.cpp keeps the manifest flow: `runManifest`, `runEpochs` (plural of `runEpoch`), `writeReport` (`write` + noun, as `writeSamples`).
- **jam_profiler**:
  - `addRow` removes a duplicated row construction. Family: `ConfigDocument::addRow`, `jam_ConfigDocument.h:97`.
  - `getSamples` keeps `writeSamples` within 30 lines.
  - `nanosecondsPerSecond` is public, so scout reads it (S, *"Hardcoded value appearing more than once"*).
  - `countUnit` is local.
  - `getDurations` is deleted, because it has no caller left.
- **EVE**:
  - `openTTY` keeps `prepare` within 30 lines. Family: `TerminalTTY::open`.
  - The `shellExited` row is in `Source/layout/parameters.md`, because `Model::storeValue` needs the parameter (`jam_Model.h:981`).
  - The quit listener registers when the profile shell opens. `Model::replaceState` replaces content in place (`jam_Model.h:802`), so a restored value must not quit.
  - `TerminalMemoryTTY::open` records once, per the `TerminalTTY::open` contract: *"When the shell already runs, the call changes nothing and returns `true`"* (`jam_TerminalTTY.h:72-73`). Runtime evidence: SIGTRAP, *"Release of a suspended object"*.
  - Source rows `terminalMemoryTTYHeader` and `terminalMemoryTTYSource` follow `terminalProcessorHeader` and `terminalProcessorSource`.
- **jam_subprocess**:
  - `Handler` becomes `Callbacks`, the replacement that NAMES Rule 6 lists.
  - `failedLaunchExitCode` names the launch failure code.
  - `kill` runs only for a live child. A reaped PID can belong to another process (`juce_SharedCode_posix.h:1227-1230`).
- **jam_terminal posix**: `open` splits by responsibility into `getArguments`, `setNonBlocking` and `addReadSource`. Each name uses the fixed verb set (NAMES Rule 1).
- **scout fail-fast**:
  - `Benchmark::runEpoch` deletes the old samples file before each epoch. JUCE reports a signal-killed child as exit code 0 (`juce_SharedCode_posix.h:1187-1193`, `:1232-1249`), so a crash then fails on the missing file (MANIFESTO D).
  - `runEpoch` and `addEpoch` return `juce::Result`.

## Amendment E — facts that differ from the Step text (audit record)

- `Id::user` comes from JAM `jam/cast/data_structures.md:59`, not from scout (Step 6.1). The scout row was a duplicate symbol, so it was removed.
- `Id::unit` comes from JAM `jam/cast/web.md:59`, not from `profiler.md` (Step 4.1). `jam_profiler` reaches it through `jam_markdown`.
- `Profiler::nanosecondUnit` is public, because scout reads it (`scout/Source/Benchmark.cpp`). The reason is the same as for `nanosecondsPerSecond` in Amendment C (MANIFESTO S, *"Hardcoded value appearing more than once"*).
- `Profiler::callsPerEpoch` is public for the same reason. scout and `getSamples` both use it.
- The profiler drain runs in the EVE view getter, on the message thread, once per frame (`EVETerminalProcessor.cpp`). Each per-thread ring has one consumer. A drain in `onProcessRequest` ran on two threads: the message thread, and the serial queue of `handleAsyncUpdate` (`jam_AsyncUpdater.h:18-20`). The two consumers corrupted the rings: an empty zone key with 187,722 calls appeared in `eve/benchmark/live-report.md:1961-1965`. The getter reaches the profiler through `Instance<Profiler>`, the access that `Zone` itself uses (`jam_Profiler.cpp:15`).

## Consequences recorded (facts, not decisions)

- The `zone` probe-cost row ends with `Benchmark::addCalibration` (`jam_Benchmark.cpp:66-77`). R13 deletes `Benchmark`.
- RFC R9 item 4 is already covered by `swapHistoryLines` (Step 2 validation).
- The CAROL profile and trace skills, and the second samples writer in `sample.lua` (R2), are parallel work outside this plan (RFC §0 item 15).

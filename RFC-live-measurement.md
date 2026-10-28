# RFC: scout as the runner and statistics lane — workloads live in the profiled project, four measurement lanes, one report format

**Date:** 2026-10-28 (rewritten the same day, after ARCHITECT's lane questions)
**Author:** COUNSELOR, at ARCHITECT's request.
**Companion:** `~/.carol/RFC-profile-skill.md` (the profile skill and the trace skill changes that follow this RFC).
**Status:** open. ARCHITECT implements this RFC first: *"i will update both first then back to you."*

This RFC states facts, requirements and options. It records no decision that ARCHITECT did not state. Every new name, field and format is ARCHITECT's (NAMES.md Rule -1).

---

## 0. ARCHITECT's words (verbatim, this session, in order)

1. *"let's start profiling eve with scout. what genuienely we could optimise to get stable, most deterministic, highest critical performance, within the limit of AUDIO ARCHITECTURE discipline with lock free, fully event driven, concurrent, at parity of endless that beats all terminal definitively?"*
2. *"is scout genuienely helpful and accurate to profile? can you synthesize deterministic solution to solve our bottleneck to beat endless?"*
3. *"It does not cover the place where you lose time??? why not? are you sure you're using it correctly? or is this scout design limitation? if it's factually scout limitation, can we fix the design so it can accurately cover whatever we need?"*
4. *"IIUC they all have benchmark tracked. should we start document our profiling to eve/benchmark/ ?"*
5. *"what do we need to fix scout to measure deterministically whatever you need? as SKILLS.md? scout engine contract changes? both? i'll fix it first."*
6. *"i dont understand the question, how can we measure accurately so it help you pinpoint the bottleneck precisely then we could implement the most deterministic solution?"*
7. *"where's scout now output report for eve? why not at eve/benchmark/scout-report.md?"*
8. *"why scout only profile jam? not eve and jam?"*
9. *"so each time we want to profile we need to rebuild scout with whatever project we're profiling? i thought it was standalone headless cli only need to be "attached""*
10. *"profiling with scout is granted with build permission"*
11. *"whatdbg"*
12. *"what gives? how do we make the profile works without rebuilding scout each time? rebuilding project that being profiled is logically make sense, but why do we need to rebuild scout each time?"*
13. *"can we just integrate with whatdbg? is scout need to be redesigned with attach process like whatdbg? or we need all to profile? each with its own lane?"*
14. *"udpate the RFC for scout and carol, comprensively with this understanding, then i will update both first then back to you."*
15. *"there are parallel work with scout and carol, you clean up jam/kanjut side if any. jam_unit_test should just moved to scout, so JAM and KANJUT cleaner no? update the scout RFC?"*

Target, from the deleted `jam/PLAN-eve-max-speed.md:24` (quoted verbatim in this session's Pathfinder report): *"END (old endless) is sub 1 min. that's the least acceptable number"*.

---

## 1. The measured gap

ARCHITECT's runs: `seq 1 N`, timed by the zsh `time` builtin inside each terminal.

| 100M run | user | system | total | source |
|---|---|---|---|---|
| endless (old END binary) | 19.31 s | 39.38 s | 59.609 s | `eve/comparison.md:58` |
| EVE, Sprint 176 best | 18.15 s | 42.12 s | 1:01.20 | `eve/comparison.md:62` |
| EVE, latest Release (2026-10-28) | 17.57 s | 46.69 s | 1:04.78 | ARCHITECT, this session |

Latest Release, all sizes (ARCHITECT, this session):

```
seq 1 1000000  0.18s user 0.47s system 99% cpu 0.655 total
seq 1 10000000  1.92s user 4.63s system 99% cpu 6.590 total
seq 1 100000000  17.57s user 46.69s system 99% cpu 1:04.78 total
```

Facts:

- The loss is in the system time of `seq`. Against Sprint 176, system time is +4.57 s and user time is −0.58 s.
- Against endless, system time is +7.31 s and total time is +5.17 s.
- The last measured EVE row in `eve/comparison.md` is Sprint 176. Sprints 177–179 have no measured row.
- The system time belongs to the `seq` process (`eve/comparison.md:10`). It is kernel time of a child process, not time inside EVE.

---

## 2. scout today

### 2.1 Build

- scout compiles only `scout/Source` (`scout/project-info.md:11`, `:146-152`) and JAM modules (`:10`, `:124-143`).
- It does not compile `eve/Source`. It has no `jam_audio_devices`, no `jam_plugin_bootstrap` and no `juce_audio_utils` (`:111-122`). `juce::AudioProcessorPlayer` lives in `juce_audio_utils` (`JUCE/modules/juce_audio_utils/players/juce_AudioProcessorPlayer.cpp`).

### 2.2 Runner

- `main` owns the one benchmark instance, `TerminalBenchmark terminalBenchmark;` (`scout/Source/Main.cpp:95`).
- scout takes one argument, the manifest path (`:97-100`).
- `getTests` finds benchmarks by name among the `juce::UnitTest` objects registered in the scout process (`:11-12`).
- `runManifest` sets the epochs, runs the tests, builds the report with the baseline, and writes it (`:43-86`).
- The report path is relative to the manifest (`:62`, `:68`). The write is `outputFile.replaceWithText (report)` (`:70`). That path opens the file with `open (…, O_RDWR | O_CREAT, 00644)` and creates no folder (`juce_File.cpp:788-802`; `juce_SharedCode_posix.h:509`). A failed write prints the path and returns `EXIT_FAILURE` (`Main.cpp:80-81`).

Consequence: each new workload is new scout code and a new scout build. This is the coupling that ARCHITECT's question 12 names.

### 2.3 Engine (JAM)

- `jam::Benchmark::runTest` wraps `runEpoch()` in one `Id::epoch` zone (`jam/jam_unit_test/benchmark/jam_Benchmark.cpp:34-37`). After each epoch it drains the Profiler and adds one sample for each zone key: the sum of that zone's durations in the epoch (`:39-40`, `:56-64`). Each epoch after the first compares `getOutput()` with the first output (`:47-48`).
- `Profiler::Zone` reads `juce::Time::getHighResolutionTicks()` at construction and destruction (`jam/jam_profiler/profiler/jam_Profiler.cpp:7-19`). It measures wall-clock time on the calling thread, in the process that owns the zone.
- `addEntry` is private (`jam_Profiler.h:32`). A sample can come only from a zone.
- Zones exist only when `JAM_USING_PROFILER` is set. The default is 0 (`jam/jam_profiler/jam_profiler.h:26-27`).
- `ProfileReport::getProfile` and `getText` build the report (`jam/jam_unit_test/report/jam_ProfileReport.h:6-9`). The report is also the next baseline (`~/.carol/skills/profile/SKILL.md:118`).

### 2.4 The `Terminal` benchmark

- `initialise()` opens a real `TerminalUnixTTY` with `seq 1 1000000` and records the bytes until EOF (`scout/Source/TerminalBenchmark.cpp:13-45`; `TerminalBenchmark.h:13`).
- `runEpoch()` builds four objects (`TerminalBenchmark.cpp:53-60`), then replays the recording through `TerminalMemoryTTY` (`:63`), a `juce::MemoryInputStream` (`TerminalMemoryTTY.h:18`). It loops `process` in place and writes history after each call (`TerminalBenchmark.cpp:65-68`).
- The epoch has no PTY, no clock wake path and no TextEditor. The `editorLanes` zone (`jam/jam_gui/widgets/jam_TextEditor.cpp:796`) never fires.

### 2.5 The last report (`scout/eve-report.md`)

| zone | median | spread | calls |
|---|---|---|---|
| `terminalProcess` | 136.7 ms | 0.57 % | 122 |
| `swapHistoryLines` | 250.0 ms | 2.10 % | 122 |
| `swapLiveLines` | 0.041 ms | 8.80 % | 122 |
| `epoch` | 403.1 ms | 3.18 % | 1 |
| `zone` (probe cost) | 62.4 µs for 10000 zones | 0.60 % | 10000 |

Sources: `scout/eve-report.md:6`, `:8`, `:10`, `:12`, `:18`. The report does not record the build type. `SKILL.md:139`: *"Only a Release build gives a performance verdict."*

### 2.6 EVE's own profile build

- The `profile` toolchain row builds Release with `-DCAST_PROFILE=ON` (`eve/project-info.md:144-149`). That sets `JAM_USING_PROFILER=1` (`:382`).
- EVE owns a `jam::Profiler` (`eve/Source/EVEProcessor.h:87-89`) and prepares it with `defaultCapacity` (`EVEProcessor.cpp:19-21`).
- The vblank getter drains it (`eve/Source/EVETerminalProcessor.cpp:85-88`).
- The destructor drains it again, converts every duration to one sample per call, and writes a report with `jam::ProfileReport` and an empty baseline into the settings folder (`EVEProcessor.cpp:50-70`).
- So EVE already measures its real chain in its own process. Its report has no baseline and no p-value (`:70`).

### 2.7 Verdict

- scout is accurate for what it measures: the spreads in §2.5 are under 3.2 % for the parse, history and epoch rows.
- scout does not measure the loss in §1, for three reasons:
  1. Its workloads are compiled into it (§2.2). It cannot run EVE's chain without a copy of EVE's code.
  2. A zone measures only wall time inside the process that owns it (§2.3). seq's kernel time is in another process.
  3. Its only workload replays from memory (§2.4).
- This is not a usage error. The manifest and the command follow the profile skill (`SKILL.md:27-31`, `:66-90`).

---

## 3. The EVE byte path that the lanes must cover

From this session's Pathfinder trace (macOS standalone). Q_A is the `VirtualDevice` serial GCD queue. Q_B is the `EVETerminalProcessor` serial GCD queue.

1. The TTY read source runs on Q_A (`jam/jam_terminal/tty/native/jam_TerminalTTY_posix.cpp:211`). Its handler suspends the source and calls `onBytesReady` (`:213-223`).
2. `onBytesReady` calls `device.clock (0)` (`eve/Source/EVEProcessor.cpp:34`).
3. `VirtualDevice::clock` calls the player callback (`jam/jam_audio_devices/audio_io/jam_VirtualDevice.cpp:76-82`). The callback is `holder->player` (`EVEProcessor.cpp:43`).
4. `juce::AudioProcessorPlayer` takes two locks on each tick (`juce_AudioProcessorPlayer.cpp:235`, `:258`), then calls `processBlock` (`:319`).
5. `EVETerminalProcessor::process` loops `terminalProcessor.process (…)` while bytes were read (`EVETerminalProcessor.cpp:265-279`).
6. Each `TerminalProcessor::process` call does one `tty.read` into the 65536-byte buffer, parses, and calls `swap.setLiveLines` (`jam/jam_terminal/processor/jam_TerminalProcessor.cpp:31-60`). `setLiveLines` writes a full frame when the frame changed (`jam_TerminalBuffer.h:316-332`).
7. `TerminalUnixTTY::read` resumes the read source on EAGAIN (`jam_TerminalTTY_posix.cpp:285-311`).
8. When the row ring is full, the parse stops (`jam_TerminalProcessor.cpp:69`). Q_B writes history and requests the next tick (`EVETerminalProcessor.cpp:124-128`). The Q_B write starts from the vblank getter (`:79-80`). Q_B maps a new file every 65536 lines (`jam_TerminalSwap.cpp:11-26`, `:105`).

Contract conflict on this path:

- `eve/SPEC.md:380`: *"❌ `setValueNotifyingHost` or any lock on the clock."*
- `juce_AudioProcessorPlayer.cpp:235`, `:258`: two `ScopedLock` on each clock tick.

endless, for comparison: one high-priority reader thread waits in `poll`, reads 65536 bytes until EAGAIN, and parses inline under one lock (`endless/Source/terminal/tty/TTY.cpp:53-88`; `ProcessorEvents.cpp:416-426`). Its history ring drops the oldest entries and does no file I/O (`endless/Source/terminal/CellFifo.h:360-418`).

---

## 4. The design: four lanes, one report format

Each lane answers one question. No lane's measurement disturbs another lane's numbers, because each lane runs alone (§6, R12).

| Lane | Tool | What is rebuilt | Question it answers |
|---|---|---|---|
| **Where** | whatdbg `sample`, attach by pid; scout reports its tallies (R2) | the target, with debug symbols | Which functions are hot on each thread? |
| **How long** | zones compiled into the target's profile build | the target only | How long does each stage take, per call? |
| **Statistics and gate** | scout | nothing, after this RFC | Median, spread, baseline, p-value for each lane's numbers. |
| **Verdict** | `time seq`, or `getrusage` on the child | nothing | The number against endless. |

Why the lanes stay separate:

- **Zones are compile-time.** Only the target's build can add them (`jam_profiler.h:26-27`). That rebuild is the logical rebuild of ARCHITECT's question 12.
- **whatdbg pauses the process.** The sample driver *"sends `pause`, waits for the `stopped` event, reads the stacks, and sends `continue`"* (`~/.carol/skills/trace/SKILL.md:87-88`). While EVE is paused, `seq` blocks on a full PTY. So wall time under sampling is not throughput. The share of samples per function is valid. Its numbers cannot enter the same statistics as zone timings.
- **Child kernel time is in another process.** Only `time` or `getrusage` on the child sees it (§5, R3).
- **One statistics implementation.** All lanes that give numbers use `jam::ProfileReport` and the scout statistics. MANIFESTO S: *"Declare once. Reference everywhere."* EVE already calls the same `ProfileReport` (`EVEProcessor.cpp:66-70`).

Prior art for the split, ghostty (`ghostty/src/benchmark/AGENTS.md`):

- Two roles: *"`ghostty-gen` generates synthetic input data"* and *"`ghostty-bench` consumes existing input data and runs a benchmark"* (`:5-6`).
- The workload binary is built from ghostty's own tree: *"`zig build -Demit-bench -Doptimize=ReleaseFast`"* (`:34`).
- An external runner times it: *"Prefer `hyperfine` to compare benchmark timings"* (`:22`), *"medians instead of single runs"* (`:25-26`).
- *"Never run multiple benchmarks in parallel on the same machine"* (`:29-30`).

---

## 5. Requirements for scout (the statistics and gate lane)

### R1 — scout runs workloads, it does not contain them

- Today the workload is a class compiled into scout (§2.2).
- After R1, the manifest names a command for each benchmark: the program, its arguments and its working folder. scout runs it as a child process for each epoch and waits for it.
- The epoch time is the child's lifetime, measured by scout, as hyperfine does.
- Input size becomes data: the arguments carry N (for example `seq 1 100000000`). This replaces the compile-time `fixtureLineCount` (`TerminalBenchmark.h:13`).
- The manifest grammar gains the command fields. ARCHITECT names them.

### R2 — the target hands its zone durations to scout

- Today EVE converts its zone durations into a finished report at destruction (`EVEProcessor.cpp:58-70`).
- After R2, a target writes its raw zone durations for the run to a file. scout reads that file after each epoch and adds the values to the epoch's samples.
- Format: the cast grid-table format, so that `jam::ConfigDocument` reads it, as it reads the manifest and the report (`SKILL.md:47-49`, `:118`). ARCHITECT sets the format and its fields.
- Path: scout tells the target where to write. Two forms. ARCHITECT selects one:
  1. a command argument;
  2. an environment variable.
- The write uses the one `jam_profiler` code path, so that every target writes the same format. Today this conversion lives in `EVEProcessor.cpp:50-70`. That is project code that each target would repeat.
- A second writer of the same format: the whatdbg sample driver (`~/.carol/skills/trace/sample.lua`) writes its per-thread function tallies in this format, one sample run per epoch. scout then reports the Where lane with the same statistics, markdown and mermaid (`~/.carol/RFC-profile-skill.md` §2.10.2). A share is a count over samples, not a time, so its rows have their own unit and stay apart from the nanosecond rows.

### R3 — child process time

- The verdict needs the user time and the system time of the measured child (§1).
- Source: `getrusage (RUSAGE_CHILDREN)`. The man page reads *"returns information describing the resources utilized by the current process, or all its terminated child processes"* (`MacOSX.sdk/usr/share/man/man2/getrusage.2:53-55`). It has *"no way to obtain information about a child process that has not yet terminated"* (`:173-174`).
- scout reads it before and after each epoch. The difference is the time of the children that the epoch reaped.
- Under R1, scout is the direct parent of the epoch command. When the command is `seq`, `seq` is a direct child, with no `login` in between.
- In the live EVE run (R8), `seq` is a descendant of EVE, not of scout. On macOS the PTY child first tries to start through `/usr/bin/login` (`jam/jam_terminal/tty/native/jam_TerminalTTY_posix.h:24`). The man page lines read here do not say whether `RUSAGE_CHILDREN` includes a grandchild. The first live run must compare the reported time with `time seq` for the same N.
- The engine must accept these values as sample rows with the same statistics as a zone: median, spread, min, max, samples and baseline comparison (`SKILL.md:100-117`). Today `addEntry` is private (`jam_Profiler.h:32`), so a non-zone value has no path into the samples.

### R4 — the build type in the report

- `SKILL.md:139`: *"Only a Release build gives a performance verdict."*
- `scout/eve-report.md` does not record the build type (§2.5).
- With R2, the target is the measured binary, so the target writes its build configuration into its raw file, and scout copies it into the report.

### R5 — per-call distribution

- EVE's live samples are *"one value per call, not per epoch"* (`SKILL.md:141-142`).
- scout's statistics are per epoch: one sum per zone per epoch (`jam_Benchmark.cpp:56-64`).
- ARCHITECT's question 1 asks for *"stable, most deterministic"* performance. Stability is the spread of each call, not only the epoch sum.
- With R2, scout has every call's duration. It can report both: the per-epoch sum, for the baseline gate, and the per-call distribution (median and tail) of each zone. ARCHITECT decides whether the tail is a report row and which percentile.

### R6 — report location in the measured project

- ARCHITECT's question 7: *"why not at eve/benchmark/scout-report.md?"*
- Paths are relative to the manifest (`SKILL.md:44`; `Main.cpp:62`, `:68`). A manifest in `eve/benchmark/` with `output` set to `scout-report.md` writes `eve/benchmark/scout-report.md` with no scout change.
- scout creates no folder (§2.2). Either the folder exists before the run, or scout creates the report's parent folder. ARCHITECT selects.
- Prior art: none of the four terminals checked tracks results in its repository (§9). ghostty: *"Keep large generated benchmark corpora outside the repository"* (`AGENTS.md:17-18`). That rule is about input corpora, not reports.

### R7 — the replay workload moves out of scout (see also R13)

- `TerminalBenchmark` exercises JAM code only (`TerminalBenchmark.cpp:53-60`).
- A JAM change needs a rebuild of the binary that contains JAM, whichever binary that is. That rebuild is the logical one.
- After R1, the replay becomes a workload binary run by scout, so scout itself is never rebuilt for it.
- JAM has no build of its own (`jam/.claude/CLAUDE.md`, BUILD NOTE). ARCHITECT decides where the replay workload binary lives.

---

## 6. Requirements for the targets and the other lanes

### R8 — the live EVE epoch

R1 needs a command that runs one live epoch and exits. Two forms. ARCHITECT selects one:

1. **EVE runs one command and quits.** The EVE profile build accepts the command (for example `seq 1 N`), runs it in its terminal, quits when the shell exits, and writes its raw zone file (R2). scout runs EVE as the epoch command.
2. **Manual runs, scout aggregates.** ARCHITECT runs the EVE profile build and `time seq 1 N` by hand. Each run writes one raw zone file. scout reads N files as N epochs and runs no command.

Form 1 adds a launch feature to EVE. Form 2 adds nothing to EVE beyond R2, and the epochs depend on hand runs.

### R9 — zones on the wake path (target code)

Today four zones exist: `terminalProcess`, `swapLiveLines`, `swapHistoryLines` and `editorLanes` (`SKILL.md:122-127`). None of them times:

- the TTY read handler (`jam_TerminalTTY_posix.cpp:213-223`);
- `VirtualDevice::clock` (`jam_VirtualDevice.cpp:76-82`), which includes the player locks (`juce_AudioProcessorPlayer.cpp:235`, `:258`);
- the `read` syscall apart from the parse (`jam_TerminalProcessor.cpp:37`);
- the Q_B writer (`EVETerminalProcessor.cpp:124-128`).

The pattern for a zone is `jam_TerminalProcessor.cpp:33-35`. The keys go in the module's `cast/<module>.md` identifiers (`SKILL.md:154-156`). ARCHITECT names them.

### R10 — profiler capacity in a long live run (engine)

- Each thread takes one slot through a `thread_local` index from a global counter (`jam_Profiler.cpp:4-5`). There are 64 slots (`jam_Profiler.h:30`).
- `addEntry` asserts the slot index and drops the entry when the index is past the slots (`jam_Profiler.cpp:23-25`). It asserts that one entry fits and writes nothing when the ring is full (`:27-35`).
- Each slot ring holds `defaultCapacity` = 65536 entries (`jam_Profiler.h:19`; `EVEProcessor.cpp:20`).
- In EVE the rings drain on the vblank getter (`EVETerminalProcessor.cpp:85-88`).
- Q_A and Q_B are GCD serial queues. GCD may run a serial queue on different worker threads over time. Each new worker thread takes a new slot. This RFC did not verify how many distinct threads a 100M run uses.
- Risk: in a Release build the asserts are off, and dropped entries leave no signal. A long live run can lose entries with no error.
- Requirement: the run must report dropped entries, or the design must show that none can drop.

### R11 — debug symbols for the Where lane (target build data)

- whatdbg needs a target *"already built with debug symbols"* (`trace/SKILL.md:44`).
- EVE sets `-g` / `/Z7` for Debug only (`eve/project-info.md:212`). Release has `-O3` (`:208`) and `-dead_strip` (`:220`). The `profile` row is Release (`:146`).
- For a Where-lane sample at Release speed, the `profile` configuration needs `-g` / `/Z7`. This RFC did not verify whether function names resolve without it.

### R13 — `jam_unit_test` moves into scout; JAM and KANJUT delete it

ARCHITECT's ruling (§0, item 15): *"jam_unit_test should just moved to scout, so JAM and KANJUT cleaner"*.

Facts:

- The module holds three objects: `Benchmark` (`jam/jam_unit_test/benchmark/jam_Benchmark.h/.cpp`), `ProfileReport` (`report/jam_ProfileReport.h/.cpp`) and `Statistics` (`statistics/jam_Statistics.h/.cpp`), plus the module files `jam_unit_test.h/.cpp`.
- Its users today:
  - scout lists it (`scout/project-info.md:143`) and compiles `jam/jam_unit_test/jam_unit_test.cpp` (scout `Builds/` object files).
  - EVE lists it (`eve/project-info.md:281`) and calls `jam::ProfileReport` once, in the destructor, under `JAM_USING_PROFILER` (`eve/Source/EVEProcessor.cpp:52-70`).
- KANJUT carries the mirror `kuassa_unit_test` as a kernel module (`kuassa/user_modules/user-modules-info.md:111-115`), with `cast/unit_test.md` and `generated/kuassa_UnitTest.h/.cpp`.
- A kernel module is the unit of `cast --sync`. The two kernel sets must correspond one to one (`cast/Source/HELP.md:69`). JAM and KANJUT must delete the module together.
- After R1 and R2, scout is the only user: workloads leave scout (R1, R7), and EVE writes raw durations through `jam_profiler`, not a report (R2).

Requirement, in this order:

1. **scout copies the code first.** `ProfileReport` and `Statistics` move into `scout/Source`, with the identifiers they read (today in `jam/cast/unit_test.md` and `generated/jam_UnitTest.*`). `Benchmark` does not move: R1 removes the compiled-in workload pattern. `scout/Source/TerminalBenchmark.h/.cpp` and `TerminalMemoryTTY.h/.cpp` are deleted with it (R7 moves the replay to a workload binary).
2. **scout drops the JAM module.** Delete the `jam_unit_test` row (`scout/project-info.md:143`) and `JAM_USING_PROFILER=1` (`:173`). scout keeps `jam_profiler` (`:142`) for the raw-file reader of R2.
3. **EVE stops using it.** The destructor report (`EVEProcessor.cpp:55-70`) becomes the one `jam_profiler` raw write (R2). Delete the `jam_unit_test` row (`eve/project-info.md:281`). Until R2 lands, an EVE `profile` build without the module does not compile, because of the `ProfileReport` call at `EVEProcessor.cpp:67`.
4. **JAM and KANJUT delete the module.** Delete `jam_unit_test/`, `cast/unit_test.md`, `generated/jam_UnitTest.*` and every row that names them, in JAM and in KANJUT, in one change (`HELP.md:69`).

Step 4 must not run before step 1. Today the JAM and KANJUT directories are the only source of `ProfileReport` and `Statistics`. `scout/Source` holds only `TerminalBenchmark` and `TerminalMemoryTTY` (glob, this session).

### R12 — one measurement at a time

- ghostty: *"Never run multiple benchmarks in parallel on the same machine, as they will interfere with each other and produce unreliable results"* (`AGENTS.md:29-30`).
- A Where-lane sample pauses the target (§4). It never runs during a How-long or Verdict run.

---

## 7. Measurement targets after the lanes exist

One variable at a time. Each change is gated against the previous report with the Mann–Whitney p-value (`SKILL.md:113`).

1. **Read size and wakes per line.** A prior plan recorded live reads of exactly 1024 B, citing `bsd/sys/tty.h:105` (deleted `jam/PLAN-eve-full-throttle.md:15`, quoted by Pathfinder). The replay reads 65536 B (122 calls per 1M lines, `scout/eve-report.md:6`). The live `terminalProcess` call count gives the real value.
2. **The live-frame publish per read** (§3 step 6).
3. **The player locks on the clock** (§3 step 4, and the SPEC conflict).
4. **The drain coupled to the history write** (§3 step 8).

From the last replay report, if it is representative:

- the parse rate is about 7.3 M lines/s (136.7 ms per 1M lines);
- the history rate is about 4.0 M lines/s (250.0 ms per 1M lines);
- `seq` at 100M ran at about 1.54 M lines/s (100M lines in 64.78 s).

So EVE's CPU work is not the limit at that rate. The limit is in how and when the drain runs. Only the live lanes can confirm this.

---

## 8. Open questions for ARCHITECT

1. R1: the manifest command fields and their names.
2. R2: the raw zone file format and fields; the path by argument or by environment variable.
3. R3: the names of the child-time rows.
4. R5: a per-call tail row, and which percentile.
5. R6: the folder exists before the run, or scout creates it.
6. R7: where the replay workload binary lives.
7. R8: form 1 (EVE runs one command and quits) or form 2 (manual runs, scout aggregates).
8. R9: the names of the wake-path zones.
9. R10: report dropped entries, or prove that none can drop.
10. R11: `-g` / `/Z7` in the `profile` configuration.
11. Duration: a live 100M epoch lasts about 65 s (§1). Eleven epochs (`SKILL.md:74`) last about 12 minutes.
12. R2: the names and the unit of the Where-lane share rows that the sample driver writes.

The whatdbg sample driver ends every attach run with a kill today (`~/.carol/skills/trace/driver.lua:51`, `:155-159`). whatdbg itself can detach and leave the target running (`whatdbg/Source/Whatdbg.cpp:406-411`; `Session_mac.cpp:380-382`). The driver change is in `~/.carol/RFC-profile-skill.md` §2.10.1. Until it lands, a Where-lane attach ends the EVE session that it samples.

---

## 9. Prior art (read this session)

- **ghostty:** `src/benchmark/` holds 18 benchmarks, among them `TerminalParser.zig` and `TerminalStream.zig`. Method: §4.
- **kitty:** `benchmark.py` has `run_parsing_benchmark` (`:83`) and `exec_under_perf` (`:156`). It also has `tools/cmd/benchmark/main.go`.
- **wezterm:** micro benchmarks only: `termwiz/benches/cell.rs`, `wezterm-char-props/benches/wcwidth.rs`, `rangeset/benches/rangeset.rs`.
- **alacritty:** no benchmark file or folder in the local checkout.
- None of the files found tracks results in its repository.

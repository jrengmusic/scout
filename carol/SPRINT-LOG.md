# SPRINT-LOG.md

**Project:** scout  
**Repository:** /Users/jreng/Documents/Poems/dev/scout  
**Started:** 2026-10-28

**Purpose:** Long-term context memory across sessions. Tracks completed work, debts paid, and debts deferred to the inter-sprint ledger (`DEBT.md`). Written by PRIMARY agents only when ARCHITECT explicitly requests.

---

## 📖 Notation Reference

**[N]** = Sprint Number (e.g., `1`, `2`, `3`...)

**Sprint:** A discrete unit of work completed by one or more agents, ending with ARCHITECT approval ("done", "good", "commit")

---

## ⚠️ CRITICAL RULES

**AGENTS BUILD CODE FOR ARCHITECT TO TEST**
- Agents build/modify code ONLY when ARCHITECT explicitly requests
- ARCHITECT tests and provides feedback

**AGENTS NEVER RUN GIT COMMANDS WITHOUT ARCHITECT'S EXPLICIT INSTRUCTION**
- Write code changes without running git commands
- Agent runs git ONLY when user explicitly requests
- Never autonomous git operations
- One exception: `git log` for a history question that bears on the task (CAROL.md Git)
- **When committing:** Always stage ALL changes with `git add -A` before commit
  - ❌ DON'T selectively stage files (agents forget/miss files)
  - ✅ DO `git add -A` to capture every modified file

**SPRINT-LOG WRITTEN BY PRIMARY AGENTS ONLY**
- **COUNSELOR** writes to SPRINT-LOG
- Only when user explicitly says: `"log sprint"`
- No intermediate summary files
- No automatic logging after every task
- Latest sprint at top, keep last 5 entries

**NAMING RULE (CODE VOCABULARY)**
- All identifiers must obey project-specific naming conventions (see NAMES.md)
- Variable names: semantic + precise (not `temp`, `data`, `x`)
- Function names: verb-noun pattern (initRepository, detectCanonBranch)
- Struct fields: domain-specific terminology (not generic `value`, `item`, `entry`)
- Type names: PascalCase, clear intent (CanonBranchConfig, not BranchData)

**BEFORE CODING: ALWAYS SEARCH EXISTING PATTERNS**
- ❌ NEVER invent new states, enums, or utility functions without checking if they exist
- ✅ Always grep/search the codebase first for existing patterns
- ✅ Check types, constants, and error handling patterns before creating new ones
- **Methodology:** Read → Understand → Find SSOT → Use existing pattern

**TRUST THE LIBRARY, DON'T REINVENT**
- ❌ NEVER create custom helpers for things the library/framework already does
- ✅ Trust the library/framework - it's battle-tested

**FAIL-FAST RULE (CRITICAL)**
- ❌ NEVER silently ignore errors (no error suppression)
- ❌ NEVER use fallback values that mask failures
- ❌ NEVER return empty strings/zero values when operations fail
- ❌ NEVER use early returns
- ✅ ALWAYS check error returns explicitly
- ✅ ALWAYS return errors to caller or log + fail fast

**⚠️ NEVER REMOVE THESE RULES**
- Rules at top of SPRINT-LOG.md are immutable
- If rules need update: ADD new rules, don't erase old ones

---

## Quick Reference

### For Agents

**When user says:** `"log sprint"`

1. **Check:** Did I (PRIMARY agent) complete work this session?
2. **If YES:** Write sprint block to SPRINT-LOG.md (latest first)
3. **Include:** Files modified, changes made, alignment check, debts paid, debts deferred
4. **Hygiene:** After writing the SPRINT-LOG entry, drain paid debts from project-root `DEBT.md` via `carol debt clear <id>` for each ID under *Debts Paid*. Receipt first, then clean the books.

### For User

**Activate PRIMARY:**
```
"@CAROL.md COUNSELOR: Rock 'n Roll"
```

**Log completed work:**
```
"log sprint"
```

**Invoke subagent:**
```
"@oracle analyze this"
"@engineer scaffold that"
"@auditor verify this"
```

**Available Agents:**
- **PRIMARY:** COUNSELOR (domain specific strategic analysis)
- **Subagents:** Pathfinder, Oracle, Engineer, Auditor, Librarian

---

<!-- SPRINT HISTORY STARTS BELOW -->
<!-- Latest sprint at top, oldest at bottom -->
<!-- Keep last 5 sprints, rotate older to git history -->

## SPRINT HISTORY

<!-- The entry format is in /log (commands/log.md). It is the only source. -->
<!-- Sprint entries go here, written by PRIMARY agents -->

## Sprint 1: Live measurement — scout as the universal runner and statistics lane ✅

**Date:** 2026-10-28
**Duration:** not recorded

### Agents Participated
- COUNSELOR — planned (`PLAN-live-measurement.md`, `PLAN-mermaid-writer.md`), wrote each Engineer brief, validated every return against CONTRACT, filtered the audit, and wrote the plan amendments and this log.
- Pathfinder — read the profiler consumers, the Mermaid parser and writer pattern, and the Document API. Ran the `forkpty` fd probes and the process checks.
- Engineer — implemented every step and correction. Ran the KANJUT syncs, the builds and the scout runs under the watchdogs, and the documentation pass.
- Auditor — one sweep over both plans: 45 findings.

### Files Modified (by repository)
- **scout**
  - `Source/Main.cpp`: the manifest flow (`runManifest`, `runEpochs`, `writeReport`). The baseline goes into `getProfiles`, and `getText` only renders.
  - `Source/Benchmark.h/.cpp` (new): runs the command once per epoch through `jam::Subprocess` (`runEpoch`, `runCommand`), times the epoch and reads `getrusage`. Reads the samples file (`addEpoch`, `addRows`) and merges the `bins` of all epochs (`addBins`).
  - `Source/ProfileReport.h/.cpp` (new): each zone is complete at creation, with one `Statistics`, the percentile, and the baseline comparison. Charts come through `jam::MermaidDiagram::toDocument` and `jam::MermaidWriter`: one median chart per unit, one distribution chart per `_CALLS` zone.
  - `Source/Statistics.h/.cpp` (new): weighted statistics over (value, count): median, p99.9, error percent, min, max, Mann-Whitney. Built from samples (count 1) or from profiler bins (bin centre).
  - `cast/identifiers.md`, `project-info.md`: the run vocabulary. `jam_subprocess` added, `jam_unit_test` removed.
  - `Source/HELP.md`, `CLAUDE.md`: the manifest, the report and the samples-file format.
  - `PLAN-live-measurement.md` (Amendments A–C, E), `PLAN-mermaid-writer.md` (Amendment D).
  - Deleted: `Source/TerminalBenchmark.*`, `Source/TerminalMemoryTTY.*` (moved to EVE), `eve.md`, `eve-report.md`.
- **JAM**
  - `jam_profiler/profiler/jam_Profiler.h/.cpp`: each zone owns `numBins` log bins (64 per octave, 40 octaves) and a tick sum. `drain` adds 1 to one bin. `writeSamples` writes constant-size rows. Also `dropped`, `callsPerEpoch`, `getBinNanoseconds (double)` and `calls` as `int`.
  - `jam_profiler/jam_profiler.h/.cpp`, `cast/profiler.md`, `cast/spell.md`, `user-modules-info.md`: the profiler vocabulary (`bins`, `buildType`, `calls`, `dropped`, `manifest`, `run`, `samples`). `jam_unit_test` deleted.
  - `jam_mermaid_diagram/diagram/jam_MermaidDiagram.h/.cpp`: `toDocument (ValueTree)`, `addProperties`, `addProperty`. This is the inverse of `parse`, with labels as spans over `addSource` text.
  - `jam_mermaid_diagram/diagram/jam_MermaidWriter.h/.cpp` (new): `MermaidWriter : Document::Writer`. It renders an xychart document as Mermaid source in fixed notation, with `max_digits10` places.
  - `jam_mermaid_diagram/jam_mermaid_diagram.h/.cpp`: the writer submodule.
  - `jam_subprocess/subprocess/jam_Subprocess.h/.cpp`:
    - It reaps the child before the exit code is read.
    - `terminate` gates on the worker thread, not on `waitpid` from the caller.
    - `~Subprocess` is defaulted, `launch` asserts there is no worker, and `appendWithCap` has 2 branches.
  - `jam_terminal/processor/jam_TerminalProcessor.h/.cpp`: the `shellExited` event, the `terminalRead` zone in a private `read`.
  - `jam_terminal/tty/native/jam_TerminalTTY_posix.h/.cpp`:
    - `open` uses `forkpty`.
    - `runChildProcess`, `setEnvironment`, `runLoginShell` and `getLoginCommand` are split out.
    - The tokens are unquoted for the direct `execvp`, and the `ttyBytesReady` zone is added.
  - `cast/terminal.md`: `terminalRead`, `ttyBytesReady`.
- **KANJUT** (`kuassa/user_modules`): mirror through `cast --sync`. `kuassa_unit_test` deleted. `user-modules-info.md` and `spell.md` hand-mirrored.
- **EVE**
  - `Source/EVEProcessor.h/.cpp`: the `deviceClock` zone. In the profile standalone build, the destructor stops the device, drains, and writes the samples.
  - `Source/EVETerminalProcessor.h/.cpp`:
    - The profile run reads its `## profiler` table and quits on `shellExited`.
    - There is one message-thread drain per frame, plus `getManifestFile`.
    - `createTTY` takes the replay TTY under `EVE_USING_REPLAY`.
  - `Source/ShellIntegration.h/.cpp` (new): the shell integration setup, split out of `EVETerminalProcessor`.
  - `Source/TerminalMemoryTTY.h/.cpp` (moved from scout): records once, then replays from memory.
  - `cast/identifiers.md`, `layout/parameters.md`, `project-info.md`: the replay toolchain row, `eveUsingReplay`, the vocabulary and the source rows.
  - `benchmark/live.md`, `benchmark/replay.md`: the manifests.

### Alignment Check
- [x] BLESSED principles followed
  - The profiler transit holds constant-size bins: 0 per-call copies, 0 growing arrays.
  - Each zone is complete at creation.
  - The Mermaid grammar has one owner, in JAM, in both directions.
- [x] NAMES.md adhered: every new name comes from a cited family (`bins`, `octave`, `MermaidWriter`, `toDocument`, `ShellIntegration`, `getLoginCommand`).
- [x] MANIFESTO.md principles applied
- [x] Audit: 45 findings.
  - Each finding was validated by a read, then fixed or rejected with a citation.
  - The rejections are 2, 3, 4, 7 (the MANIFESTO part), 9, 14, 15, 16, 21, 24 (the D part), 26, 30, 32 (the id part), 34, 35, 36, 38 (the file size), 39, 40, 42, 43, and 44 (`addReadSource`, int narrowing, `clock (0)`).
  - 3 was first accepted, then rejected after the double-consumer evidence. 40 was first accepted, then rejected after the build evidence.

### Problems Solved
- **Live run stalled at shutdown.** `writeSamples` rendered every call (about 420,000 per zone at 1M lines) through the markdown writer. The profiler now folds each call into fixed log bins, and the live 100M run completes: 11 epochs, 738 s, median epoch 67 s.
- **`calls` read as 0.** `ConfigDocument` writes an `int64` var as a hex colour (`jam_ConfigDocument.cpp:141`, `:169-171`). `calls` is now `int`.
- **Hand-built Mermaid text in scout.** The new JAM `MermaidWriter` with `MermaidDiagram::toDocument` writes the charts. The output uses fixed notation, because the Mermaid lexer reads no exponent (`jam_MermaidDiagram.cpp:912-938`).
- **Octave labels 129, 257.** `getBinNanoseconds` returned bin centres. It now maps a position, and each caller chooses the centre or the edge.
- **Empty zone key in the report.** The drain had moved to `onProcessRequest`, which runs on two threads (the message thread and the `AsyncUpdater` queue, `jam_AsyncUpdater.h:18-20`). The drain is back in the message-thread view getter, the single consumer.
- **Race on the child's exit code.** `Subprocess::terminate` read `waitpid` from the caller's thread (`juce_SharedCode_posix.h:1176-1194`). It now gates on the worker thread state.
- **Linux argument bug.** Quoted tokens reached `execvp` with their quote characters. The tokens are now unquoted.
- **Hand-rolled PTY setup.** `forkpty` replaces openpty, fork, setsid, TIOCSCTTY, dup2 and the slave close. A probe confirmed the master is closed in the child.

### State for Continuation
- Builds: scout `--no-sign`, EVE `--profile` and `--replay`, and cast `--no-sign` are clean. The JAM doxygen gives 0 warnings (`docs/warn.JAM.txt` is 0 bytes). The KANJUT sync is clean: the second run writes nothing.
- `eve/benchmark/live-report.md`, `replay-report.md`: the latest runs. Live: 11 epochs, `real` 740 s.
- `dropped` in the live run is about 4.4M of 90M calls per zone (`live-samples.md`). The rings (`defaultCapacity` 65536 per thread) overflow between per-frame drains. The report shows the loss, per the R10 ruling *"Report the drop count"*.
- Uncommitted: scout, JAM, EVE and KANJUT working trees.

### Debts Paid
- None

### Debts Deferred
- None

---

**End of SPRINT-LOG.md Template**

Copy this template to your project root as `SPRINT-LOG.md` and customize:
- Project name
- Repository URL/path
- Starting date
- Add project-specific rules to CRITICAL RULES section

Rock 'n Roll!  
**JRENG!**

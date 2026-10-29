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

## Sprint 4: Kernel Sampling — kernel and user stacks in the scout engine, one backend for each OS ✅

**Date:** 2026-10-29
**Duration:** not recorded

### Agents Participated
- COUNSELOR — consumed `RFC-kernel-sampling.md` and settled its open questions from reads (trace and profile skills, XNU, Learn, tracy). Took the rulings R1-R9 and the audit ruling "Split by job" from ARCHITECT. Wrote and locked `PLAN-kernel-sampling.md` with Amendments A-C. Wrote each Engineer brief, validated each return with a Read, filtered the audit, and wrote this log.
- Pathfinder — inventories of the scout engine, JAM `Subprocess`/`Profiler`, tracy ETW and perf code, the trace and profile skills, and the git state.
- Librarian — XNU kperf/kdebug (thread info, kdebug trigger, lost events, fork and exec events, user-stack AST), Windows job objects, ETW, TDH, DbgHelp and UAC, Linux `perf_event_open`, tracefs and capabilities.
- Engineer — Step 0 hygiene, the identifiers, the Benchmark per-table storage, `Sampler` (macOS, Linux and Windows designs and code), `Symbolizer`, the Validator rule, HELP.md, the audit fixes, the doxygen pass, and `~/.config/setup-scout.sh`.
- Auditor — one sweep over Steps 0-10. It gave 55 findings. COUNSELOR accepted 46 and rejected 9, each against a read. ARCHITECT ruled on finding 36.

### Files Modified (scout, 26 + generated)
- **Step 0 hygiene (R1)**
  - Deleted `RFC-live-measurement.md`, `RFC-profiling.md`, `PLAN-profiling.md`, `RFC-report-date.md`, `PLAN-report-date.md`.
  - `CLAUDE.md:15`, `:21`, `:23` — current sprint, `Source/` list (with `Sampler` and `Symbolizer`), `cast/` list. The `RFC-live-measurement.md` row is deleted.
  - `project-info.md` release-notes row and `RELEASE.md:7` — "runs a command once for each epoch", not `juce::UnitTest`.
  - `RFC-kernel-sampling.md` — the citations into deleted files point to their live sources. Handoff item 4 records that Step 0 fixed it.
- `PLAN-kernel-sampling.md` and `RFC-kernel-sampling.md` — the locked plan (rulings R1-R9, Amendments A-C) and its RFC. Both were deleted at the end of the sprint, on ARCHITECT's order "PLAN and RFC hygiene". Neither was in git. Session backups exist only in the session scratchpad. The stray root `tagfile.xml` from the doxygen run is deleted too.
  - Rulings, verbatim answers:
    - R1 Step 0: "Also CLAUDE/RELEASE".
    - R2: "All three, one plan".
    - R3: "scout/Source".
    - R4: "sudo whole scout", then "no password".
    - R5: "`## sample` table".
    - R6, R7: "In this plan".
    - Audit 36: "Split by job".
- `cast/identifiers.md` — `sample`, `cswitch`, `intervalMs`, `stackDepth`. `cast/text.md` — `failSampleRange`. Cast regenerated `Source/generated/Identifiers.h`, `Generated.cpp`, `Text.h`.
- `Source/Benchmark.h`, `Source/Benchmark.cpp` — storage is per table type (nested `jam::HashMap`s). `runEpoch` does the whole epoch, with no `#if`: child usage before and after, an optional `Sampler`, `drain`, `addEpoch` (samples file, sampler tables, `epoch` row), `addChildTime`, `addMissingSamples`. `getChildUsage`/`addChildTime` for each OS. On Windows, a job object owns the child accounting. `getProfiles` emits one table for each type.
- `Source/Processor.h`, `Source/Processor.cpp` — `generate` builds the manifest tree once and passes the `## sample` table to `runEpochs`.
- `Source/Sampler.h`, `Source/Sampler.cpp` (new) — the sampling session for each OS, the read thread, record parsing, the descendant filter, counting into `SAMPLE`/`CSWITCH`, and lost records into `dropped`.
  - macOS: kperf and kdebug through raw `sysctl`. Action 1 is the timer, and action 2 is the kdebug trigger on `MACH_SCHED`. The user frames are weighted by the user-stack pends of each thread.
  - Linux: per-CPU `perf_event_open`, with `inherit` on scout's thread.
  - Windows: the NT kernel logger with SampledProfile and CSwitch stack walks, Process Start descendants, and TDH payload reads.
- `Source/Symbolizer.h`, `Source/Symbolizer.cpp` (new, ruling "Split by job") — frame keys from Mach-O `LC_SYMTAB`, ELF, `/proc/kallsyms` and DbgHelp. The kernel symbols, text address and image name are built once for each process. Shared-cache and non-image files are not loaded.
- `Source/Validator.h`, `Source/Validator.cpp:27-46` — the `sample` rule checks `intervalMs` and `stackDepth` against the `Sampler` bounds.
- `Source/HELP.md` — the `## Sampling` section, the `## SAMPLE`/`## CSWITCH` tables, the rights for each OS, the macOS 100 ms look limit, `dropped`, the Windows `user`/`system` rows and the exit codes.
- `docs/xml`, `docs/tagfile.xml` — regenerated, 0 doxygen warnings.
- **Outside scout:** `~/.config/setup-scout.sh` (new). It installs `/opt/scout/scout` root-owned and adds a `NOPASSWD` sudoers rule for that binary only (macOS), or `setcap cap_perfmon,cap_dac_read_search` (Linux). `~/.config/bootstrap-macos.sh` calls it, and `bootstrap-windows.sh` notes that Windows needs nothing. `~/.config/.gitignore` whitelists `!setup-scout.sh`, because the repo ignores everything (`*`) except whitelisted root files.

### Alignment Check
- [x] BLESSED principles followed
  - B: the OS sessions are acquired in the constructor and released in the destructor. A failed Windows start stops only a session it started.
  - S: one row intake (`addRows`), one counting helper (`addCounts`), one symbol lookup.
  - D: the Windows `EventsLost` count is added after the thread joins.
- [x] NAMES.md adhered: names come from the `get`/`add`/`set`/`is`/`getOrCreate` families and the framework `start`/`stop`/`read`/`open` verbs (Amendment C.10). `Symbolizer` follows the ruling "Split by job".
- [x] MANIFESTO.md principles applied
- [x] Audit: each finding was read and named against a clause. Accepted findings are resolved. The rejections cite the refuting read:
  - 11 (D5), 12 (Sampler.cpp start order), 13 (no source), 23 (JAM private, out of scope).
  - 29 (B: release is destruction), 41 (S: one intake), 45 (offset keys), 47 (no fake carrier), 51 (no clause).
- [x] Doxygen: 0 warnings for Sampler.h, Symbolizer.h, Benchmark.h, Processor.h and Validator.h (through `~/.config/nvim/doxygen/Doxyfile.project`, with `JUCE_WINDOWS` parsed in a coverage run).
- [ ] Steps 6, 8 and 9 are not run. They are the macOS, Windows and Linux runtime checks (R9).

### Problems Solved
- **No tool showed where the kernel time inside `write` goes, cross-platform.** scout now samples kernel and user stacks of its command's process tree. It writes them through the same report path on macOS, Linux and Windows.
- **One table type for each Benchmark.** The storage is now per table, so the sampler tables sit next to the target's table.
- **`user`/`system` got a false 0 in every epoch after the first** (audit 1). The padding now runs after the child time.
- **No password at run time** (ARCHITECT: "no password"). `setup-scout.sh` configures this once for each machine. Root never reaches a manifest command on Linux. On macOS, the rule names only the root-owned binary.

### State for Continuation
- `~/.config/setup-scout.sh` is NOT applied on this Mac. `sudo` rejected the password given in chat. ARCHITECT runs `bash ~/.config/setup-scout.sh` once. Then check `sudo -n /opt/scout/scout --version` (no prompt), and check that `sudo -n true` still asks for a password.
- Step 6 (macOS) is not run. The manifest is in the session scratchpad: `kernel.md` (EVE Profile build, `seq 1 10000000`, `intervalMs` 1, `stackDepth` 64, no toolchain rows). Rewrite it if the scratchpad is gone: the EVE live manifest without `## toolchain`, `epochs` 1, and a `## sample` table. Check:
  - `PERF_CS_*` records appear, and a kernel frame resolves.
  - `seq` is kept as a descendant.
  - `proc_pidinfo` works on `/usr/bin/seq`, and the user bracket carries `PERF_TI_DATA`.
  - The kdebug trigger samples the thread that switches out.
  - No ktrace session remains after an error.
  - A derived XNU finding needs a check: the user-stack depth may fall to `MAX_UCALLSTACK_FRAMES` (osfmk_kern_thread.h:865-870).
- The macOS build of ARCHITECT's machine is clean ("scout built clean"). The Windows (Step 8) and Linux (Step 9) blocks of `Sampler.cpp`, `Symbolizer.cpp` and `Benchmark.cpp` were written on macOS and never compiled. Windows run checks: delivery lag, `SymInitializeW` with a pid key, stack walk on the same CPU, kernel stacks with `DisablePagingExecutive`, `CSwitch` stack owner, and Process Start order across CPU buffers (audit 13).
- The Step 2 check (an existing report unchanged in tables, rows and columns) is not run on `eve/benchmark/replay.md`.
- A Windows run that crashed leaves the "NT Kernel Logger" running. The next run then fails with `StartTrace: …` until `logman stop "NT Kernel Logger" -ets` (design D5).
- `git status` lists every `Builds/Debug/` file as deleted (` D`). No step of this sprint deleted them, and this log does not attribute the deletion.
- `~/.carol/skills/profile/SKILL.md` and `trace/SKILL.md` do not mention the `## sample` table or the `SAMPLE`/`CSWITCH` tables. They are outside this sprint's file set.

### Debts Paid
- None

### Debts Deferred
- None

## Sprint 3: Report Date — run date as front matter through the JAM markdown API ✅

**Date:** 2026-10-29
**Duration:** not recorded

### Agents Participated
- COUNSELOR — read the RFC, the scout report path and the JAM metadata-block path. Took the six rulings R1-R6 from ARCHITECT, then wrote and locked `PLAN-report-date.md`. Wrote each Engineer brief and validated each return with a Read. Fixed `jam_MarkdownWriter.cpp:586` (ambiguous `add`), filtered the audit into Amendment A, and wrote this log.
- Pathfinder — read the scout pipeline (`ProfileReport`, `Processor`, `Benchmark`), the JAM metadata-block parse and write paths, `ConfigDocument`, `juce::Time` and the cast lexicons. Gave the git state for the commit message.
- Engineer — Steps 1-5, the Step 6.1 build, the audit corrections (#1, #3, #5, #7, #11, #12a, #12d) and the Step 6.4 doxygen pass. Ran cast for `Id::date`.
- Auditor — one sweep over Steps 1-5. It gave 14 findings. The dispositions are in `PLAN-report-date.md` Amendment A.

### Files Modified (14 total)
- **JAM**
  - `jam_markdown/document/jam_MarkdownDocument.h:453-472` — new public `addProperties (const juce::NamedValueSet&)` and public static `getFirstProperty`. `:641` private `addProperties` takes the property set. `:647` new private `getMetadataBlock() const`. `:2075` new `BlockParser::addLines`. The `getVersion` prose now matches its body.
  - `jam_markdown/document/jam_MarkdownDocument.cpp:355-377` — `build` calls `blocks.addLines (tokens)` and `addProperties (*block, getProperties (*block))`. `:1350-1380` both `addProperties` bodies. The private body asserts one-line, non-empty values. The public body asserts that no metadata block exists, and inserts the block as the first root child in the parse shape. `:1395-1428` `getMetadataBlock`, `getFirstProperty`, `getProperties() const` and `getVersion` read the block through the two new functions. `:896-904` `isEnabled (key)` does the same. `addPropertyLines` walks through `getFirstProperty`.
  - `jam_markdown/document/jam_MarkdownBlockParser.cpp:2489-2515` — `addLines`: the token-to-line split, moved out of `build` (audit #12a).
  - `jam_markdown/document/jam_MarkdownWriter.h:526-535` — `getMetadataBlockText` and its doxygen. The `getBlocks` brief names the new renderer.
  - `jam_markdown/document/jam_MarkdownWriter.cpp:567-589` — `getMetadataBlockText` writes `---`, then `key: value` or `key:` + `- item` from the property elements, then `---`. `:1005-1006` the `metadataBlock` entry uses it (ruling R2).
- **scout**
  - `Source/Processor.cpp:92-95` — `writeReport` reads the clock once and passes `{ Id::date, "%d-%m-%Y %H:%M" }` to `ProfileReport::toFile` (rulings R3, R4, R5).
  - `Source/Processor.h:88` — the `writeReport` doc names the `date` front matter.
  - `Source/ProfileReport.h:9`, `:16-28` — `toFile` takes `properties`. The class and function docs name the front matter.
  - `Source/ProfileReport.cpp:4`, `:11` — `document.addProperties (properties)` before the write.
  - `cast/identifiers.md:44-45` — the `date` row. Cast regenerated `Source/generated/Identifiers.h:43` and `Source/generated/Generated.cpp:39`.
  - `Source/HELP.md:104-105` — scout reads the baseline after the epochs, so a self-baseline compares with the previous run (ruling R6). `:114-115` — the report starts with front matter that holds `date` as `DD-MM-YYYY HH:MM`.
  - `PLAN-report-date.md` (new) — the locked plan, rulings R1-R6, and Amendment A.

### Alignment Check
- [x] BLESSED principles followed
  - S: scout writes no `---` text. JAM writes the block from its property elements (R1, R2).
  - E: the date reaches `toFile` as a parameter. Preconditions use `jassert` at the owner.
  - L: `build` went from 46 lines to 24.
- [x] NAMES.md adhered: each new name comes from a family. `getMetadataBlockText` (from `getHtmlBlockText`), `addProperties` (overload), `addLines` (from `addLine`), `getFirstProperty` and `getMetadataBlock` (from `getProperties`, `isMetadataBlock`), `dateTimeFormat` (`jam/cast/markdown.md:48`). `date` is ARCHITECT's ruling R3.
- [x] MANIFESTO.md principles applied
- [x] Audit: each finding was read and named against a clause. Accepted findings are resolved. Each rejection cites the refuting read (Amendment A).
- [x] Doxygen: 0 warnings for scout `Source/` and for `jam_MarkdownDocument.h` and `jam_MarkdownWriter.h`. The check ran through `~/.config/nvim/doxygen/Doxyfile.project` into the scratchpad. scout has no `doxygen` ninja target.

### Problems Solved
- **A report had no date of its own.** The report now starts with front matter that holds `date` as `DD-MM-YYYY HH:MM`. scout takes the date at report write.
- **JAM read front matter but could not write it from data.** `MarkdownWriter` only echoed the parsed raw text. It now writes the block from the property elements. A public `addProperties` builds the block.
- **`lines.add (Chars::tripleDash)` did not compile.** The call matched all four `jam::Strings::add` overloads (`jam_Strings.h:228`, `:239`, `:245`, `:251`). It now calls the `std::string_view` overload.
- **The metadata-block search and the property walk appeared four and three times.** Each is now one function.

### State for Continuation
- Builds: the scout Release build with `-DCAST_SIGN=OFF` is clean. ARCHITECT has not run the new binary on a manifest. Check that the report starts with `---`, `date: …`, `---`, and that a report used as a baseline still reads back.
- Every metadata block now writes from its property elements, parsed blocks included. A parsed line that is not `key: value`, `key:` or `- item` is not written back (`jam_MarkdownDocument.cpp:1330-1345`).
- JAM types `29-10-2026 14:03` as text, not as a date (`jam_MarkdownDocument.cpp` `getPropertyType`, `dateCharacters` in `jam/cast/markdown.md:45`).
- `scout/docs/xml` is regenerated with the new `toFile` signature and `dateTimeFormat` (git diff). `jam/docs/xml` shows no change in `git status`. Check it for `getMetadataBlockText`, `getFirstProperty`, `getMetadataBlock` and `addLines` before you rely on it.
- The cast run rewrote `.project`: `CAST_COMMIT` `1e1ae90` → `2330495`, and `kind` → `type`.
- The JAM working tree also holds uncommitted changes from outside this sprint: `jam_core/concurrency/jam_AsyncUpdater.*` and `jam_terminal/tty/` (`isRunning` removed). This log does not attribute them.
- `~/.carol/skills/profile/SKILL.md` (report grammar, `:205-213`) does not mention the front matter. It was outside this sprint's file set.

### Debts Paid
- None

### Debts Deferred
- None

## Sprint 2: Profiling — scout identical to cast, one command per profile ✅

**Date:** 2026-10-28
**Duration:** not recorded

### Agents Participated
- COUNSELOR — wrote and locked `PLAN-profiling.md` with Amendments A and B, wrote each Engineer brief and each trace spec, validated every return against CONTRACT, filtered the audit, wrote the doxygen run files, and wrote this log.
- Pathfinder — read the cast sources, the nvim locator, the whatdbg launch handler and EVE. Ran the Step 7 CLI cases, the replay run, the live 100M run with `time seq`, the Step 12 attach probe, the Where-lane epoch, and the git status reads.
- Engineer — implemented each step and each correction, ran the cast regenerations and the builds, and ran the doxygen and coverage runs.
- Auditor — one sweep over Steps 1–13. Dispositions are in `PLAN-profiling.md` Amendment B.

### Files Modified (by repository)
- **scout**
  - `Source/Main.cpp`: command line only, copied from cast. Banner, `--help`, `--version`, `not found`, `printError`, `Fine.` on a terminal, and the screen erase. An extra argument fails with `failFlagUnknown`. The exits are `EXIT_SUCCESS` and `EXIT_FAILURE`.
  - `Source/Validator.h/.cpp` (new): `Validator : jam::ConfigValidator`. `addRules` adds the `toolchain` column rule (`failToolchainColumn`).
  - `Source/Processor.h/.cpp` (new): `generate` validates, runs the `## toolchain` rows (`run`, `runToolchainRow`), then runs the epochs (`runEpochs`) and writes the report (`writeReport`). A missing baseline fails before the first epoch.
  - `Source/Toolchain.h/.cpp` (new): a verbatim copy of cast's Toolchain (ruling *"Copy into scout"*).
  - `Source/Help.h/.cpp`: cast's `printHelp (helpText)`, with `files::scoutHelp`.
  - `Source/Benchmark.h/.cpp`: the `failCommand` and `not found` texts. Each row holds one sample for each epoch, and an absent row adds 0.
  - `Source/ProfileReport.h/.cpp`: `getText` is replaced by `toFile`, through `jam::MarkdownWriter`, which creates the folder.
  - `Source/Statistics.h`: documentation only.
  - `Source/HELP.md`: the guide in cast's shape.
  - `cast/text.md`, `cast/files.md`, `cast/banner.md` (new). `cast/identifiers.md`, `cast/spell.md`: cast's generated layout. `Source/generated/`: `Identifiers.cpp` deleted. `Generated.h/.cpp`, `Text.h`, `Files.h` and `HashMaps.h` are new.
  - `project-info.md:261`, `:308`: the `macroRedefined` rows (`-Wno-macro-redefined`, `/wd4005`). `CMakeLists.txt` is regenerated.
  - `PLAN-live-measurement.md`, `PLAN-mermaid-writer.md` deleted. `CLAUDE.md` lost their rows. `PLAN-profiling.md` is new.
  - `docs/` (new): the project doxygen XML.
- **JAM**
  - `jam_subprocess/subprocess/jam_ChildProcess.h` and `native/jam_ChildProcess_posix.cpp`, `native/jam_ChildProcess_windows.cpp` (new): a native launch with a working folder for the child only. POSIX uses fork, `chdir` and `execvp`. Windows uses `CreateProcessW` with `lpCurrentDirectory`. A child that a signal killed reports 128 plus the signal number (`getExitStatus`).
  - `jam_subprocess/subprocess/jam_Subprocess.h/.cpp`: the Worker uses `jam::ChildProcess`. `setAsCurrentWorkingDirectory` is gone.
  - `jam_subprocess/jam_subprocess.h/.cpp`: the platform includes.
- **CAROL** (`~/.carol`)
  - `skills/profile/SKILL.md`: one command, the exit table, `## toolchain`, the lanes, and `## Limits` with the live-run numbers.
  - `skills/trace/epoch.lua`, `skills/trace/sample` (new): the scout epoch adapter and its wrapper. They write the `## SAMPLE` and `## RUN` tables last.
  - `skills/trace/sample.lua`, `driver.lua`, `SKILL.md`: `.json` goes to `Driver.main` and `.md` goes to `Epoch.main`. `kind` is renamed to `type`. `Driver.reportSpecErrors` is shared. The `-g` note is updated.
- **nvim** (`~/.config`)
  - `nvim/lua/core/project/cast.lua`, `core/project.lua`: the benchmark compile databases (`benchmark/*.md`) win for clangd.
  - `nvim/lua/core/build.lua`, `nvim/lua/dap/launch.lua`: `kind` is renamed to `type`.
- **EVE**
  - `benchmark/live-report.md`, `live-samples.md`, `replay-report.md`, `replay-samples.md`: rewritten by the Step 8 and Step 10 runs.

### Alignment Check
- [x] BLESSED principles followed
  - scout reaps each child before it reads the exit code. The child gets its working folder without a change to the parent's folder.
  - The command line, Validator, Processor and Toolchain each trace to a cast sibling (ruling 2).
- [x] NAMES.md adhered: each new name comes from a cast or JAM family. `kind` is renamed to `type` (Rule 6).
- [x] MANIFESTO.md principles applied
- [x] Audit: each finding was read and named against a clause. The accepted findings are resolved, and the rejections cite the refuting read (`PLAN-profiling.md` Amendment B).
- [x] Doxygen: the scout project build gives 0 warnings. A coverage run (`EXTRACT_ALL = NO`, `WARN_IF_UNDOCUMENTED`, `WARN_NO_PARAMDOC`) gives 0 warnings for scout. For `jam_subprocess` it gives 0 warnings in headers. The 28 remaining warnings are members defined in `.cpp` files, and CODING.md forbids doxygen there.

### Problems Solved
- **The profile needed several hand-run commands.** `scout <absolute-manifest-path>` now runs the manifest's `## toolchain` rows, then the epochs, then the report.
- **The scout command line differed from cast's.** It is now cast's command line, with scout's texts.
- **Sample keys lost characters in markdown.** `_` became emphasis: `-_NSApplication_run_` read back as `-NSApplicationrun`. The key alphabet is now `[A-Za-z0-9:-]`.
- **A signal-killed child reported exit 0.** It now reports 128 plus the signal number.
- **`jam::Subprocess` changed the parent's working folder.** The native launch sets the folder in the child only.
- **Banner rows lost a leading space.** A CommonMark code span strips one space at each end when both ends hold a space. The trailing spaces of `highVoltage` and `star` were removed (`cast/banner.md:14`, `:17`).
- **A doxygen tag error at `Main.cpp:118`.** The period inside `` `scout: Fine.` `` ended the auto brief. The literal moved to the detail.

### State for Continuation
- Builds: scout `--no-sign` is clean. Check `Builds/Release/scout_artefacts/Release/scout --version`.
- The Windows file `jam_ChildProcess_windows.cpp` has not been built. Build it on a Windows machine before relying on it.
- tit has no project-info toolchain and has not been built against the new `jam::Subprocess`.
- `scout-banner.sh` stays at the scout root.
- Uncommitted work from before this sprint is in the same working trees (`~/.carol/HANDOFF-profiling.md:22-24`, `:57-68`):
  - `~/.carol`: `CAROL.md`, `agents/oracle.md`, `commands/goplan.md`, `commands/handoff.md`, `commands/rfc.md`, `skills/rfc-writer/`, `skills/handoff-writer/`, `skills/plan-writer/`.
  - EVE: `project-info.md`, `benchmark/live.md`, `benchmark/replay.md`.
- `~/.carol/skills/trace/trace.lua` is modified in the working tree. This log does not attribute it.
- Uncommitted: scout, JAM, EVE, `~/.carol` and `~/.config`.

### Debts Paid
- None

### Debts Deferred
- None

---

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

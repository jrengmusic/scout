# PLAN: Sampling Gaps — Symbols, Attribution, and the Incoming Process

**RFC:** none — objective from ARCHITECT prompt (session 2026-10-31)
**Date:** 2026-10-31
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8 + JAM (CODING.md "One object, one translation unit"; per-OS blocks inside the existing `Sampler.cpp` and `Symbolizer.cpp`)

## Overview

ARCHITECT, verbatim: *"scout engine need to be updated so there's no gap to be questioned. whether the time lost worth chasing need factual measurement not assumptions"*; *"must be crossplatform. ask only right questions no read cant answer"*; table name ruling: *"incoming"*.

The first macOS sampling run (`eve` session scratchpad `sample-run-report.md`, 31-10-2026 11:15) produced image names only, one bogus kernel key, and no process attribution. This plan closes each gap on macOS, Linux and Windows, so a sampling run shows, per process, its stacks, its stacks at each switch, and which process takes the CPU at each switch.

## Language / Framework Constraints

- CODING.md CRITICAL RULES and Code Hygiene: no comments before the audit; doxygen in one post-audit pass.
- One object, one translation unit: all new bodies stay in `Sampler.cpp` / `Symbolizer.cpp`, inside their existing `#if JUCE_MAC` / `#if JUCE_LINUX` / `#if JUCE_WINDOWS` blocks.
- The macOS and Linux runtime checks need root (`sudo -n scout`, `~/.config/setup-scout.sh`); Windows needs an administrator console (`Source/HELP.md:60-61`). Linux and Windows run checks wait for ARCHITECT's machines, as ruling R9 of the kernel-sampling sprint set (`carol/SPRINT-LOG.md:168`).

## Dependency & API Inventory

**Evidence of the gaps (first macOS run):**
- `## SAMPLE` / `## CSWITCH` rows are image names only (`EVE`, `seq`) and one key `-NSConcreteGlobalBlock` (354,486 switch samples). Tables have no process column (report lines 38, 56).
- EVE Profile binary and `/usr/bin/seq` start with `cafe babe` (universal). `isSymbolImage` accepts only `MH_MAGIC_64` (`Symbolizer.cpp:157-162`, `:203`; `getCommand` `:92`).
- `__NSConcreteGlobalBlock` is entry 6894 of 6894 in `nm -n /System/Library/Kernels/kernel.release.t8132`. Every kernel frame maps to the last kernel symbol (`Symbolizer.cpp:28-39`, `:388`). kperf logs kernel frames unslid (`xnu-12377.121.6 osfmk/kperf/callstack.c:385-391`, `:423-433`); `VM_KERNEL_UNSLIDE` gives 0 for a value that is not a slid kernel address (`osfmk/mach/vm_param.h:413-415`). The cause is not yet measured.
- Shared-cache addresses give an empty region path (measured: `proc_pidinfo PROC_PIDREGIONPATHINFO`, `pri_flags` 1 = submap). `Symbolizer.cpp:196` requires a non-empty path.

**Process and thread data already in each record:**
- macOS: thread `record.arg5` → `threadSlot` (`Sampler.cpp:324`); process `PERF_TI_DATA` `arg1` → `processSlot` (`:332`). Counts are keyed by frame only (`:1067-1074`).
- Linux: `PERF_SAMPLE_TID` word → process id (`Sampler.cpp:462`, `:667`).
- Windows: stack-walk payload process (`Sampler.cpp:1013`).

**Incoming thread at a switch:**
- macOS: `MACH_SCHED` `arg2` = `thread_tid(thread)`, the incoming thread; the event fires on the outgoing thread (`osfmk/kern/sched_prim.c:3265-3267`, `:3045`). Scout's kperf sample at that event is the outgoing thread (`osfmk/kperf/action.c:447`). Thread → process: live thread map `KERN_KDREADCURTHRMAP` (`bsd/kern/kdebug.c:2938-2939`, `:2069-2093`), entries `thread`, `valid` (pid), `command` (`bsd/sys/kdebug_private.h:476-488`); root passes `ktrace_read_check` (`bsd/kern/kern_ktrace.c:287-297`).
- Linux: `sched_switch` `next_pid` (`include/trace/events/sched.h:222-248`, v6.12), read through `PERF_SAMPLE_RAW` (`include/uapi/linux/perf_event.h:959-984`); field offsets from the tracefs `format` file (`Documentation/trace/events.rst:119-124`). Thread → process: `/proc/<tid>/status` `Tgid` (`Documentation/filesystems/proc.rst:174`, `:246`). Scout opens the switch event for its own thread with `inherit` (`Sampler.cpp:465`, `:587`); a CPU-wide event (`pid -1`) needs `CAP_PERFMON` (`perf_event_open(2)` scope rules), which `setup-scout.sh:51` grants.
- Windows: `CSwitch` `NewThreadId` (Microsoft Learn, CSwitch class). Thread → process: `Thread_TypeGroup1` `ProcessId`, `TThreadId` (Microsoft Learn, Thread_TypeGroup1), enabled by `EVENT_TRACE_FLAG_THREAD` (Microsoft Learn, EVENT_TRACE_PROPERTIES); scout sets `PROFILE | CSWITCH | PROCESS | IMAGE_LOAD` only (`Sampler.cpp:825`).

**Shared cache (macOS):** format in `dyld_cache_format.h` (dyld-1378): `dyld_cache_header` (`mappingOffset`, `imagesTextOffset`, `subCacheArrayOffset`, `sharedRegionStart`, `symbolFileUUID`), `dyld_cache_image_text_info`, `dyld_subcache_entry`. On this Mac: root `.../Cryptexes/OS/System/Library/dyld/dyld_shared_cache_arm64e`, 12 subcaches, no `.symbols` file (`symbolFileUUID` zero). The cache slide is `sharedCacheSlide` in `dyld_all_image_infos` (`mach-o/dyld_images.h:162`, public SDK).

**Established patterns:**
- Symbol tables from image files: Mach-O `LC_SYMTAB` (`Symbolizer.cpp:128-155`), ELF (`:235`), DbgHelp (Windows). The shared cache follows the Mach-O file pattern.
- Derived table names: `<TYPE>_CALLS` (`Benchmark.cpp:233-235`). Per-process tables follow it: `SAMPLE_<process>`, `CSWITCH_<process>`, `INCOMING_<process>`.
- Rows of all epochs merge by key (`Benchmark.cpp:158-183`), so the process part of a table name is the process name, not the pid.
- Descendant filter: macOS `descendants` (`Sampler.cpp:367`), Windows `addDescendant` (`:1049-1056`). Linux follows it once its events are CPU-wide.
- Identifiers: `cast/identifiers.md:46-48` (`sample`, `cswitch`); `incoming` joins them (ARCHITECT ruling).

## Validation Gate
Each step is validated by COUNSELOR before proceeding to the next — against
MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions
(no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering
the whole sprint — never per step.

## Steps

### Step 0: Close the research gaps
**Scope:** read-only (Librarian, Pathfinder).
**Action:** Cite, with excerpts: (a) `uthread_tid` against `thread_tid` (XNU), so the thread map keys match `MACH_SCHED` `arg2`; (b) the Linux `perf_event_tid` body and the per-task scope for `pid > 0, cpu >= 0`; (c) whether TDH resolves `NewThreadId` on a `CSwitch` record; (d) the `dyld_cache_header` fields that locate each subcache's symbol table (`dyld_cache_local_symbols_info` or the linkedit `LC_SYMTAB` of each image).
**Validation:** each item has a verbatim excerpt; a gap that stays open goes to ARCHITECT before Step 1.

### Step 1: macOS kernel frame values
**Scope:** `Source/Sampler.cpp` (macOS block), ephemeral `debug::Log`.
**Action:** Log the raw kernel frame values of the first 64 kernel brackets of one sampling run, with the kernel text address of `getKernelTextAddress` (`Symbolizer.cpp:177-185`). Run one `seq 1 1000000` sample. Remove the log lines in this step.
**Validation:** the log shows the frame values; COUNSELOR names the cause with the values and the XNU citations; the fix becomes Step 2's first item.

### Step 2: macOS symbols — kernel key, universal binaries
**Scope:** `Source/Symbolizer.cpp` (macOS block).
**Action:** (a) The kernel frame key from Step 1's cause. (b) For a file that starts with `FAT_MAGIC` (`mach-o/fat.h:54`, big-endian `:36-37`), read the `fat_arch` entries (`:62-68`) and load the symbols of the slice whose `cputype` matches the sampled process.
**Validation:** a `seq 1 1000000` sample names `seq` functions and kernel functions, not image names and not `NSConcreteGlobalBlock`.

### Step 3: macOS shared cache symbols
**Scope:** `Source/Symbolizer.cpp` (macOS block).
**Action:** For a frame whose region path is empty and whose address is inside the shared cache range, map the address through `sharedCacheSlide` and the cache header to its image (`dyld_cache_image_text_info`) and its symbol table, from the cache files on disk, in the Mach-O `LC_SYMTAB` pattern.
**Validation:** system-library frames (for example `write`) appear by name in a sample run.

### Step 4: Per-process tables on all platforms
**Scope:** `Source/Sampler.h`, `Source/Sampler.cpp` (all three blocks), `cast/identifiers.md` if a new identifier is needed.
**Action:** Count each sample into the table of its process: `SAMPLE_<process>` and `CSWITCH_<process>`, where `<process>` is the process name of the record's pid. `addCounts` takes the table type of the process.
**Validation:** a sample run gives `## SAMPLE_seq`, `## SAMPLE_EVE`, `## CSWITCH_seq`, `## CSWITCH_EVE` on macOS; the Linux and Windows code paths compile against the same table types.

### Step 5: The incoming process on all platforms
**Scope:** `Source/Sampler.h`, `Source/Sampler.cpp` (all three blocks), `cast/identifiers.md` (`incoming`).
**Action:** At each switch of a descendant thread, count the process name of the incoming thread into `INCOMING_<process>`:
- macOS: `MACH_SCHED` `arg2` via the live thread map (`KERN_KDREADCURTHRMAP`).
- Linux: CPU-wide `sched_switch` events (`pid -1`) with `PERF_SAMPLE_RAW`; `next_pid` at the `format` offset; `/proc/<tid>/status` `Tgid`; the descendant filter of macOS and Windows.
- Windows: `EVENT_TRACE_FLAG_THREAD`; `Thread_TypeGroup1` builds the thread → process map in the `addDescendant` pattern; `CSwitch` `NewThreadId` is counted.
**Validation:** a macOS sample run gives `## INCOMING_seq` whose rows name the processes that take the CPU from `seq`; Linux and Windows compile.

### Step 6: Documentation of the report
**Scope:** `Source/HELP.md` (`## Sampling`, `## Report`), `~/.carol/skills/profile/SKILL.md`.
**Action:** Describe the per-process tables, the `INCOMING_<process>` table, the Linux CPU-wide scope and its capability, and the Windows thread events.
**Validation:** each new table and each OS requirement is named once, with its source.

### Step 7: macOS runtime check
**Scope:** EVE Profile build, `sudo -n scout` with the `## sample` manifest.
**Action:** Run one `seq 1 100000000` sample epoch.
**Validation:** the report attributes `seq`'s switches to the incoming processes; the counts in `INCOMING_seq` and `seq`'s involuntary switches from the Verdict lane are compared.

### Step 8: Audit, doxygen, log
**Action:** @Auditor sweeps once; COUNSELOR filters per DCF; doxygen pass on changed headers (0 warnings); the sprint log on ARCHITECT's word only.
**Validation:** every accepted finding resolved.

## BLESSED Alignment
- **B:** the sampling sessions stay owned by `Sampler` (constructor start, destructor stop); the thread map and cache tables are owned by `Symbolizer`/`Sampler`.
- **L:** each OS keeps its block; the table choice is a key, not a branch chain.
- **E:** every constant of a kernel or file format is named (`FAT_MAGIC`, the `format` offsets, the event keys).
- **S (SSOT):** process names come from one source per OS (thread map, `/proc`, ETW thread events); field offsets come from the kernel's own `format` file and cache header.
- **S (Stateless):** counts remain the only state; maps are creation-time lookups.
- **E (Encapsulation):** the report path (`Benchmark`, `ProfileReport`) receives tables; it does not know the OS.
- **D:** Step 7 measures.

## Risks / Open Questions
- Linux and Windows runtime behaviour is checked only on ARCHITECT's machines (R9).
- The macOS thread map is read live; a thread that ends between the switch and the read has no entry. Step 5 counts it under its thread id until Step 0 (a) shows a better source.

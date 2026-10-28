# RFC — Run date in the scout report
Date: 2026-10-29
Status: open
Author: COUNSELOR (EVE session, PLAN-terminal-clock Step 1)
Targets: scout

## Problem Statement

EVE keeps its benchmark reports under fixed names and keeps their history in git. ARCHITECT, verbatim:

> *"fixed names + git. but date could be included as metadata on the report no? if scout not have the feature yet, write RFC to scout repo, i will work on it in parallel"*

A report does not record when scout measured it. A report read from git history has no date of its own.

## Research Summary

1. The report's `## RUN` table holds `buildType` only. Observed output: `eve/benchmark/live-report.md:1657-1663` and `eve/benchmark/replay-report.md:1251-1257` each hold one row, `| buildType | string | Release |`.
2. Scout does not build that table. It copies the `## RUN` table of the target's samples file:
   - `Benchmark::addEpoch` keeps the samples table whose type is `run` (`scout/Source/Benchmark.cpp:129-131`: `if (table.getType() == Id::toType (Id::run)) runTable = table;`).
   - `Benchmark::getProfiles` appends that table to the report tree (`Benchmark.cpp:161`: `profiles.appendChild (runTable, nullptr);`).
   - Each epoch replaces `runTable`, so the report keeps the table of the last epoch (`Benchmark.cpp:131`).
3. The guide documents the same content: `## RUN` — *"The `buildType` of the target."* (`scout/Source/HELP.md:117`); the samples file's run table *"holds `buildType`, `Debug` or `Release`"* (`HELP.md:96-97`).
4. Scout writes the report once, after the last epoch: `Processor::runEpochs` calls `writeReport (runTable, benchmark.getProfiles (getBaseline (baselineFile)))` after the epoch loop (`scout/Source/Processor.cpp:73-84`); `writeReport` writes `ProfileReport::toFile` (`:90-98`).
5. Baseline order (relevant to fixed names): the baseline file is read in that same call, after the epochs and before the report is written (`Processor.cpp:84`). A manifest whose `baseline` names its own `output` therefore compares each run with the previous run under one fixed file name. This is a reading of the call order; no run has tested it.
6. EVE's manifests have an empty `baseline` today (`eve/benchmark/live.md:14`, `eve/benchmark/replay.md:14`).

## Principles and Rationale

- **S (SSOT):** the date is a fact of the scout run, not of the target. Scout runs the epochs and writes the report (Research 4). The run's date belongs where the run is owned: in scout, written once at `writeReport`.
- **E (Encapsulation):** the target's `## RUN` table describes the target (`buildType`). A date written by the target would put a scout fact inside the target's samples, and the last epoch would win (Research 2).
- Considered and rejected: the target's profiler (`jam::Profiler::writeSamples`) writes the date into the samples file. Rejected under **S** and **E** above: one date per epoch, the last one kept, owned by the wrong object.

## Scaffold

The insertion point in scout today (`scout/Source/Processor.cpp:84-98`, verbatim):

```cpp
        return writeReport (runTable, benchmark.getProfiles (getBaseline (baselineFile)));
    }

    return juce::Result::fail (text::Diagnostics::failNotFound + Id::diagnosticSeparator + baselineFile.getFullPathName());
}

juce::Result Processor::writeReport (const juce::ValueTree& runTable, const juce::ValueTree& profiles)
{
    const auto outputFile { documentFile.getParentDirectory().getChildFile (runTable[Id::output].toString()) };

    if (ProfileReport::toFile (profiles, outputFile))
        return juce::Result::ok();

    return juce::Result::fail (outputFile.getFullPathName() + Id::diagnosticSeparator + text::Diagnostics::failOutputWrite);
}
```

The report tree gets its `## RUN` child at `Benchmark.cpp:161`. The date row would join that child.

## BLESSED Compliance Checklist
- [x] Bounds — scout owns the run; the date is written by the run's owner.
- [x] Lean — one row in an existing table; no new table.
- [ ] Explicit — row name and value format are open (Open Questions 1, 2).
- [x] SSOT — one date per report, written once.
- [x] Stateless — no state beyond the report itself.
- [x] Encapsulation — the target's samples file is not changed.
- [x] Deterministic — the report's measured rows do not change.

## Open Questions

1. **Row name.** A new key in the `## RUN` table. A new name is ARCHITECT's (NAMES.md Rule -1). Settles: ARCHITECT.
2. **Value and format.** Options: (a) the date only; (b) date and time; (c) date and time with time zone. The `type` cell takes `int`, `float`, `bool`, `string`, `colour` or `numbers` (`~/.carol/skills/profile/SKILL.md:133-134`), so a text value is `string`. Settles: ARCHITECT.
3. **Which moment.** Options: (a) the start of the first epoch; (b) the moment scout writes the report (`Processor.cpp:94`). Settles: ARCHITECT.
4. **Baseline.** Whether the guide documents the self-baseline under a fixed name (Research 5) after a run confirms it. Settles: ARCHITECT.

## Handoff Notes

- ARCHITECT works on scout in parallel with EVE's `PLAN-terminal-clock.md`.
- EVE's history rule: fixed report names, history in git, ARCHITECT commits each measured state.
- No scout source was edited for this RFC.

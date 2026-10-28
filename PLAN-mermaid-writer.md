# PLAN: Mermaid writer — report charts through the JAM document pattern

**RFC:** none — objective from ARCHITECT prompt. Amends `PLAN-live-measurement.md`.
**Date:** 2026-10-28
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8, JAM modules (one object, one translation unit; no includes in submodule files)

## Overview

scout writes its report charts as hand-joined strings (`ProfileReport::addChart`, local lambdas `getLine` and `getList`). ARCHITECT ruling, verbatim: *"NO FUCKING HANDROLL ANYTHING. CODEBASE ESTABLISHED PATTERN. DESIGN BY CONTRACT"*. This plan adds the missing inverse of the JAM Mermaid parser:

1. `MermaidDiagram::toDocument` builds a Mermaid document from a `juce::ValueTree`.
2. `MermaidWriter` renders a Mermaid document to Mermaid source text.

scout builds each chart as a `juce::ValueTree` and gets its text from the writer. The report also gets one distribution chart for each zone of the `_CALLS` table, from the zone's `bins`.

## Record of corrections already applied this session (Amendment D)

These corrections were made under the CONTRACT before this plan. They are recorded here so that the plan is complete.

| Correction | Cause (citation) | Applied change |
|---|---|---|
| Profiler transit | `drain` appended every call to a growing array, `getSamples` copied each call into `juce::var`, and the writer rendered every call as text. This broke MANIFESTO E: *"Data in transit has 0 allocation, 0 copy, 0 mutation, 0 temporary container, 0 new struct, and 0 shadow state."* | Each zone owns `numBins` = `numOctaves` × `binsPerOctave` (40 × 64) bin counts, and `drain` adds 1 to one bin (`jam_Profiler.cpp:88-111`). The samples file carries `bins`. scout merges the bins and computes weighted statistics (`Statistics.cpp`, `Benchmark::addBins`). The name comes from the JAM `bins` family (`jam_SpectrumProcessor.h:59`, `:196`, `:301`). |
| `calls` read as 0 | `ConfigDocument` writes an `int64` var as a hex colour (`jam_ConfigDocument.cpp:141`, `:169-171`). | `addRow` takes `int calls` (`jam_Profiler.h`, `jam_Profiler.cpp`). |

Verified: the scout, EVE `--profile` and EVE `--replay` builds are clean. The replay run exits 0 in 21 s. The live-check run exits 0 in 4.6 s and writes decimal `calls` (`live-check-samples.md:204`).

## Language / Framework Constraints

- `jam_mermaid_diagram` lists its submodules by hand in `jam_mermaid_diagram.h:42-45` and `jam_mermaid_diagram.cpp:4-38`, with no `BEGIN_SUBMODULE_INCLUDES` block. The new files join those lists in the same form.
- `user-modules-info.md` has no per-file rows for `jam_mermaid_diagram` (`:116-122`). No row changes.
- scout already uses `jam_mermaid_diagram` (`scout/project-info.md:139`). No module change.
- KANJUT mirrors through `cast --sync`, run twice. The second run must write 0 lines.

## Dependency & API Inventory

| Owner | API | Citation |
|---|---|---|
| jam_core | `Document::Writer` (`virtual juce::String getText (const Document&) const = 0`, `toFile`) | `jam_Document.h:878-887` |
| jam_core | `Document::addSource (std::string_view)` returns the byte offset of the appended text | `jam_Document.h:934-945` |
| jam_core | `Document::getText (const Token&)` returns the text of a span | `jam_Document.h:948` |
| jam_core | `Document::Token (startOffset, endOffset)`, `Element::add<T>`, `find<T>`, `get<T>`, `contains`, `Document::addChild`, `getRoot`, `Document::Numbers` | `jam_Document.h:50-189`, `:261-458`, `:898-1112` |
| jam_mermaid_diagram | `MermaidDiagram` (a `Document`). `build()` puts the diagram as a child of the document root, with `id` = diagram-type text and `Id::type` = the family ordinal | `jam_MermaidDiagram.cpp:105-147` |
| jam_mermaid_diagram | Span write primitive `addAlignedSpan (Element&, const Identifier&, const Token&)` (private static) | `jam_MermaidDiagram.h:113` |
| jam_mermaid_diagram | xychart tree written by the parser: root `Id::title`, `Id::xAxisLabel`, `Id::xAxisCategories` (spans), `Id::yAxisLabel` (span), `Id::yAxisMin`/`Id::yAxisMax` (double); one `Id::node` child per series with `Id::label` (span), `Id::type` (`MermaidXy::Keyword`), `Id::shape`, `Id::values` (`Document::Numbers`) | `jam_MermaidXychart.cpp:20`, `:43`, `:57`, `:99`, `:104`, `:106`, `:189-194` |
| jam_mermaid_diagram | Span read: `MermaidLayout::getText (element, source, key)`, `getTokenText` | `jam_MermaidLayout.h:20-33`, `.cpp:5-23` |
| jam_mermaid_diagram vocabulary | `map::MermaidDiagramType` (`xychartBeta` = `xychart-beta`), `map::MermaidXy::Keyword` (`bar` 0, `line` 1), `map::MermaidOperator::forward`, `Id::title`, `Id::xAxis` (`x-axis`), `Id::yAxis` (`y-axis`) | `mermaid.md:420-451`, `:869-876`, `:247-253`; `generated/jam_Mermaid.h:3793-3813` |
| jam_markdown | Pattern for the builder: `ConfigDocument::toDocument (const juce::ValueTree&)` — the inverse of `getValueTree`, typed by the var type through one lookup (`getTypeName`) | `jam_ConfigDocument.h:72-83`; `jam_ConfigDocument.cpp:136-162`, `:294-305` |
| jam_markdown | Pattern for the writer: `MarkdownWriter : Document::Writer`, one `Function::Map` dispatch (`getBlocks`) keyed by `Id::type` | `jam_MarkdownWriter.h:18-45`; `jam_MarkdownWriter.cpp:12-20`, `:962-993`, `:1028-1034` |
| jam_markdown | A mermaid block renders its stored `Id::text` between fences | `jam_MarkdownWriter.cpp:540-552` |
| jam_profiler | `Profiler::binsPerOctave`, `numBins`, `getBinNanoseconds` | `jam_Profiler.h:44-50` |

## New names (each from a family; Rule 5 source)

| Name | Where | Family / source |
|---|---|---|
| `MermaidWriter` | `jam_mermaid_diagram/diagram/jam_MermaidWriter.h/.cpp` | `MarkdownWriter` (`jam_MarkdownWriter.h:18`), the `Document::Writer` family |
| `MermaidDiagram::toDocument (const juce::ValueTree& diagram)` | `jam_MermaidDiagram.h` + `jam_MermaidDiagram.cpp` | `ConfigDocument::toDocument` (`jam_ConfigDocument.h:83`), `XmlDocument::toDocument` |
| `MermaidWriter::getDiagrams()` | private static `Function::Map<int, juce::String>` keyed by `map::MermaidDiagramType` | `MarkdownWriter::getBlocks()` (`jam_MarkdownWriter.cpp:962`) |
| `MermaidWriter::getXychartText (const Document&, const Element&)` | private static | `getHeadingText`/`getListText` (`jam_MarkdownWriter.h`); `addXychart` family (`jam_MermaidDiagram.h:2089`) |
| `ProfileReport::addDistribution` | scout | `addChart`, `addPercentile`, `addComparison` (`ProfileReport.h`) |

## Validation Gate

COUNSELOR checks each step before the next one. The checks are MANIFESTO.md (BLESSED), NAMES.md, `~/.carol/CODING.md` and the locked decisions of this plan, with no deviation and no scope drift. COUNSELOR reads every changed file at the reported lines. @Auditor runs once, after the final code step, over the whole sprint (this plan and `PLAN-live-measurement.md`).

## Steps

### Step 1: `MermaidDiagram::toDocument`
**Scope:** `jam/jam_mermaid_diagram/diagram/jam_MermaidDiagram.h`, `jam_MermaidDiagram.cpp`
**Action:**
1. Declare `static MermaidDiagram toDocument (const juce::ValueTree& diagram);` in the public section, after the `parse` overloads (`jam_MermaidDiagram.h:42`).
2. Build the diagram as `build()` does (`jam_MermaidDiagram.cpp:116-119`, `:137`). The diagram element is a child of the document root. Its `id` is the `diagram` tree type. Its `Id::type` is `diagramType->get (diagram.getType())` (the ordinal from `map::MermaidDiagramType`).
3. Convert each property of `diagram`, and of each child tree, by its var type, through one lookup in the `getTypeName` form (`jam_ConfigDocument.cpp:136-162`):
   - a string → `addSource` the text, then `addAlignedSpan` with the `Token` over the returned offset;
   - an array of strings → one span for each string, under the same key (the `Id::xAxisCategories` list shape, `jam_MermaidXychart.cpp:43`);
   - a double → `add<double>`;
   - an int → `add<int>`;
   - an array of numbers → `add<Document::Numbers>`.
4. Each child tree becomes an `addChild` element whose `id` is the child type (`Id::node` for a series, as at `jam_MermaidXychart.cpp:189`).
**Validation:**
- The tree holds the same ids and value types that the parser writes (Dependency table).
- Spans come only from `addSource` and `addAlignedSpan`, with no hand-computed offset (CODING "Use the framework API fully").
- The method contains no xychart-specific code.
- No branch chain of more than 3 cases: the var type dispatch is a lookup.
- The tree is complete at creation, and no step changes it later.

### Step 2: `MermaidWriter`
**Scope:** new `jam/jam_mermaid_diagram/diagram/jam_MermaidWriter.h/.cpp`; `jam_mermaid_diagram.h` (include after `jam_MermaidGraphics.h`, `:45`); `jam_mermaid_diagram.cpp` (include after `jam_MermaidGraphics.cpp`, `:8`)
**Action:**
1. Declare `struct MermaidWriter : Document::Writer` with `juce::String getText (const Document& document) const override;`.
2. `getText` renders each child of the document root through `getDiagrams()`, keyed by the child's `Id::type` (`MarkdownWriter::getText`, `jam_MarkdownWriter.cpp:12-20`, `:1028-1034`).
3. `getDiagrams()` holds one entry for `map::MermaidDiagramType::xychart` and one for `xychartBeta`, both mapped to `getXychartText`. It uses the static table form of `jam_MarkdownWriter.cpp:962-993`.
4. `getXychartText` writes one line for each statement, in the order the parser reads them (`jam_MermaidXychart.cpp:216-245`):
   1. the diagram-type text from `map::MermaidDiagramType`;
   2. `Id::title` with the quoted title;
   3. `Id::xAxis` with the quoted `Id::xAxisLabel`, then the bracketed, quoted `Id::xAxisCategories`;
   4. `Id::yAxis` with the quoted `Id::yAxisLabel`, then `Id::yAxisMin`, the `map::MermaidOperator::forward` spelling, and `Id::yAxisMax`, when the range is present;
   5. for each `Id::node`: the `map::MermaidXy::Keyword` text of its `Id::type`, its quoted `Id::label`, and its bracketed `Id::values`.

   Each part is present only when its key is present. Span text comes from `MermaidLayout::getText` and `getTokenText` (`jam_MermaidLayout.cpp:5-23`). Every keyword, operator and character comes from the vocabulary maps, `Id::` or `Chars::`.
**Validation:**
- The output round-trips: `MermaidDiagram::parse` of the written text yields the same ids and values as the source tree.
- No literal keyword text.
- Each function ≤ 30 lines and ≤ 3 branches.
- One object, one translation unit.
- The submodule files have no includes.

### Step 3: scout charts through the writer
**Scope:** `scout/Source/ProfileReport.h/.cpp`
**Action:**
1. `addChart (document, benchmark, unit)` builds one `juce::ValueTree` of type `xychart-beta` with these parts:
   - `Id::title` = the table type;
   - `Id::xAxisCategories` = the zone keys of that unit;
   - `Id::yAxisLabel` = the unit;
   - one `Id::node` child with `Id::type` = `MermaidXy::Keyword::bar` and `Id::values` = the zone medians.

   The block text is `MermaidWriter{}.getText (jam::MermaidDiagram::toDocument (chart))`. Delete `getLine`, `getList`, `fenceWidth`'s string join, and `chartIndent`. Keep the fenced-block element (`Id::marker`, `Id::info`, `Id::text`), which `MarkdownWriter` renders (`jam_MarkdownWriter.cpp:540-552`).
2. Add `static void addDistribution (jam::ConfigDocument& document, const juce::ValueTree& zone);` and call it once for each zone of the `_CALLS` table. It adds one chart with these parts:
   - `Id::title` = the zone key;
   - `Id::xAxisCategories` = the lower edge in ns of each octave, `getBinNanoseconds (octave * binsPerOctave)` rounded to an integer, from the first to the last octave with a count;
   - `Id::yAxisLabel` = `Id::calls`;
   - one `bar` node with the sum of the `binsPerOctave` counts of each octave.

   It uses the same block construction as `addChart`.
**Validation:**
- scout builds no Mermaid text itself.
- A chart is complete at creation.
- Each function ≤ 30 lines and ≤ 3 branches.
- No magic number: octave width = `jam::Profiler::binsPerOctave`.

### Step 4: KANJUT sync, builds, runs
**Action:**
1. Run `cast --sync /Users/jreng/Documents/Poems/dev/jam /Users/jreng/Documents/Poems/kuassa/user_modules` twice. The second run must write 0 lines.
2. Build scout with `cast cast/spell.md --no-sign`.
3. Build EVE with `--profile` and with `--replay`.

Each command runs under a 600-second watchdog, with no build-fix loop. Then run scout on `replay.md` and `live-check.md`.

Run `live.md` under ARCHITECT's 15-minute watchdog. The `live.md` run already started in this session is part of this step. If it ended before this step, run it again.
**Validation:**
- All builds are clean.
- Each report has the median charts and one distribution chart for each `_CALLS` zone.
- Each chart block parses back through `MermaidDiagram::parse`, the same path that `MarkdownComponent::getOrCreateDiagram` uses (`jam_MarkdownComponent.cpp:282-291`).

### Step 5: Audit
@Auditor sweeps this plan and `PLAN-live-measurement.md` once. COUNSELOR validates each finding (Design by Contract Filter) and resolves it in this sprint.

### Step 6: Documentation pass
Write doxygen for the changed public API:
- `Profiler` bins members, `getBinNanoseconds`, `writeSamples` (now `bins`), `addRow`;
- `Statistics` (weighted), `ProfileReport` (`getProfile` overload, `addPercentile`, `addDistribution`, `getZone`, `getStatistics`), `Benchmark::addBins`;
- `MermaidDiagram::toDocument`, `MermaidWriter`.

Update `scout/Source/HELP.md`:
- the samples file holds `bins`, not `samples`;
- the `_CALLS` table has a `bins` column;
- one distribution chart for each zone.

Zero doxygen warnings. Docs live in the header only.

### Step 7: Delete temporary files, log
Delete `eve/benchmark/replay-check*` and `live-check*`. Then log the sprint per `/log` and write the commit messages in chat.

## BLESSED Alignment

- **B:** The `MermaidDiagram` document owns its source and every span. The writer holds no state.
- **L:**
  - One writer, one dispatch table.
  - `toDocument` is generic by var type, with no per-diagram code.
  - The distribution is one function.
- **E:**
  - Every keyword comes from the vocabulary maps.
  - Every value comes from `Id::`, `Chars::` or the profiler constants.
- **S (SSOT):**
  - The Mermaid grammar has one owner (`jam_mermaid_diagram`) in both directions.
  - scout no longer holds a copy of the grammar.
- **S (Stateless):** `MermaidWriter::getText` is `const` and reads the document only.
- **E (Encapsulation):** scout tells the writer what to render and never builds Mermaid syntax.
- **D:**
  - The same chart tree gives the same text.
  - The round trip through `parse` proves it.

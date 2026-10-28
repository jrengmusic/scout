# PLAN: Report Date

**RFC:** RFC-report-date.md
**Date:** 2026-10-29
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + JAM (scout is a JAM project; JAM is the in-house framework)

## Overview

scout writes the run date into the report as markdown front matter. JAM gets the write path for
front matter, so that the framework writes the block, not scout. The guide documents the date and
the self-baseline.

## ARCHITECT Rulings (verbatim)

| # | Decision | ARCHITECT's words |
|---|---|---|
| R1 | Front matter carries the date, through the framework API | *"our markdown support frontmatter metadata, this is should not be handrolled. should be automatically using framework API"* |
| R2 | JAM change: MarkdownWriter renders the metadata block from its property children. A public MarkdownDocument call adds that block. Scout calls it in ProfileReport::toFile. | Answer: *"MarkdownWriter from properties"* |
| R3 | Property key | Answer: *"date"* |
| R4 | Value text | Answer: *"DD-MM-YYYY HH::MM"*, clarified as *"29-10-2026 14:03"* |
| R5 | Moment | Answer: *"Report write"* |
| R6 | RFC Open Question 4 | Answer: *"Document in HELP.md"* |

RFC mapping:
- RFC Open Question 1 (row name in `## RUN`) is replaced by R1 and R3. The date is a front-matter property, not a `## RUN` row. RFC Scaffold `Benchmark.cpp:161` does not change.
- RFC Open Question 2 → R4. Open Question 3 → R5. Open Question 4 → R6.
- RFC "Principles": the date is written by scout at report write (`Processor.cpp:90-98`). The target's samples file does not change.

## Language / Framework Constraints

- One object, one translation unit (CODING.md). `MarkdownDocument` bodies stay in `jam_MarkdownDocument.cpp`. `MarkdownWriter` bodies stay in `jam_MarkdownWriter.cpp`.
- JAM is a module. Submodule files hold no includes (CODING.md Header Discipline).
- Comments and doxygen are written last, in Step 6, after the audit (CAROL.md Code Hygiene).
- Ignore LSP diagnostics (CAROL.md Build Environment).

## Dependency & API Inventory

**JAM — read path that exists (unchanged):**
- `MarkdownDocument::BlockParser::isMetadataBlock` accepts the block only at source offset 0, between two `---` lines (`jam_MarkdownBlockParser.cpp:1187-1206`).
- Parse shape of the block: `Id::type` = `map::BlockType::metadataBlock` (`jam_MarkdownBlockParser.cpp:84-89`, `:1232`). First child `Id::text` holds the raw lines (`jam_MarkdownInlineParser.cpp:49-53`). Then one child per property, built by `addProperties (Element&)` (`jam_MarkdownDocument.cpp:1351-1363`, called from `build`, `:392-394`).
- Readers walk the property children from `block.firstChild->nextSibling`: `getProperties() const` (`jam_MarkdownDocument.cpp:1386`), `addPropertyLines` (`:1494`).
- Static `getProperties (const Element&)` parses `key: value` and `key:` + `- item` lines (`jam_MarkdownDocument.cpp:1319-1349`).
- `ConfigDocument::getValueTree` reads tables only (`jam_ConfigDocument.cpp:119-132`). A front-matter block does not change the baseline read (`Processor.cpp:3-9`).

**JAM — write path today:**
- `MarkdownWriter` renders `metadataBlock` with `getHtmlBlockText`, which echoes the first child's `Id::text` (`jam_MarkdownWriter.cpp:560-565`, `:981-982`).
- `ConfigDocument::toDocument` adds headings and tables only (`jam_ConfigDocument.cpp:294-305`).
- No public call adds a metadata block (`jam_MarkdownDocument.h`: `addPropertyLines` :609 and `addProperties` :619 are private; `addMetadataBlock` :1391 is `BlockParser`-private).

**Framework API this plan commits to:**
- `Document::addChild (Element& parent, Element* after, const juce::Identifier& id)` — `after` = `nullptr` makes the first child (`jam_Document.h:1030-1037`).
- `Document::addChild (Element& parent, const juce::Identifier& id)` (`jam_Document.h:1027`), `Element::add<ValueType>` (`jam_Document.h:452-458`), `Element::find<juce::String>` (used at `jam_MarkdownDocument.cpp:1367`).
- `map::BlockTag::getInstance()->get (blockType)` for the element id (`jam_MarkdownBlockParser.cpp:86`).
- `Chars::tripleDash` (`jam/generated/jam_Core.h:379`), `Chars::colon`, `Chars::dash`, `Chars::space`, `Chars::newline` (used at `jam_MarkdownDocument.cpp:1321-1322`, `jam_MarkdownWriter.cpp:551`).
- `jam::Strings` + `joinIntoString (newline, 0, -1)` — the sibling `getFencedBlockText` (`jam_MarkdownWriter.cpp:540-552`).
- `juce::NamedValueSet (std::initializer_list<NamedValue>)` (`juce_NamedValueSet.h:67`).
- `juce::Time::getCurrentTime()` (`juce_Time.h:109`), `juce::Time::formatted (const String&)` with strftime codes `%d %m %Y %H %M` (`juce_Time.h:239-270`).

**Established patterns:**
- Writer renderer family: `getHtmlBlockText`, `getFencedBlockText` (`jam_MarkdownWriter.h:510`, `:524`) → new `getMetadataBlockText` (NAMES Rule 5).
- Document add family: `addProperties (Element&)` (`jam_MarkdownDocument.h:619`), `ConfigDocument::addRow` (public, `jam_ConfigDocument.h:97`) → public overload `addProperties (const juce::NamedValueSet&)` (NAMES Verb Contract: `add` inserts into the document).
- Format constant family: JAM `dateFormat` (`jam/cast/markdown.md:46`) → scout local `dateFormat`.
- Scout identifiers come from `cast/identifiers.md` (`scout/cast/spell.md:146-158`). `date` is absent from JAM `@id` rows (`jam/cast/core.md`, `jam/cast/markdown.md`) and from scout (`scout/cast/identifiers.md:18-42`).
- String building with `+ Chars::x` (`scout/Source/Main.cpp:95-100`, `Processor.cpp:51`).

**Consequence of R2 (stated, not open):** every metadata block, parsed or built, now renders from its property children. A parsed line that is not `key: value`, `key:` or `- item` adds no property (`jam_MarkdownDocument.cpp:1336-1345`). The writer therefore does not write it back.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against
MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions
(no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering
the whole sprint — never per step.

## Steps

### Step 1: JAM — public `addProperties`
**Scope:** `jam/jam_markdown/document/jam_MarkdownDocument.h`, `jam/jam_markdown/document/jam_MarkdownDocument.cpp`
**Action:**
1. Change the private `void addProperties (Element& block);` (`.h:619`) to `void addProperties (Element& block, const juce::NamedValueSet& properties);`. The body (`.cpp:1351-1363`) loops over `properties` in place of `getProperties (block)`. Nothing else in the body changes.
2. In `build` (`.cpp:392-394`), the call becomes `addProperties (*block, getProperties (*block));`.
3. Add a public `void addProperties (const juce::NamedValueSet& properties);` next to `getProperties() const` (`.h:451`). Body, in `.cpp` after the private overload:
   - `addChild (*getRoot(), nullptr, map::BlockTag::getInstance()->get (map::BlockType::metadataBlock))` makes the block the first child of the root.
   - `add<int> (Id::type, map::BlockType::metadataBlock)` on the block.
   - `addChild (*block, Id::text)` with `add<juce::String> (Id::text, juce::String())`, so the block has the parse shape. The readers at `.cpp:1386` and `:1494` start at `firstChild->nextSibling`.
   - `addProperties (*block, properties);`
**Validation:** Names are only `addProperties`, `properties`, `block`. No new local struct. No bail-out guard. `not`/`and`/`or`. Brace init. No comments. The parse path gives the same children as before.

### Step 2: JAM — `MarkdownWriter::getMetadataBlockText`
**Scope:** `jam/jam_markdown/document/jam_MarkdownWriter.h`, `jam/jam_markdown/document/jam_MarkdownWriter.cpp`
**Action:**
1. Declare `static juce::String getMetadataBlockText (const Element& block);` in the private section, after `getHtmlBlockText` (`.h:524`).
2. Define it in `.cpp` after `getHtmlBlockText` (`:560-565`), in the shape of `getFencedBlockText` (`:540-552`):
   - `jam::Strings lines` starts with `Chars::tripleDash`.
   - For each property from `block.firstChild->nextSibling`: when `property->find<juce::String> (Id::value)` gives a value, add `id + Chars::colon + Chars::space + value`. Otherwise add `id + Chars::colon`, then one line `Chars::dash + Chars::space + item value` for each child `Id::value`.
   - Add `Chars::tripleDash`. Return `lines.joinIntoString (juce::String::charToString (Chars::newline), 0, -1)`.
3. In `getBlocks` (`.cpp:981-982`), the `metadataBlock` entry calls `getMetadataBlockText (block)`. The `htmlBlock` entry stays on `getHtmlBlockText`.
**Validation:** Output is the exact form that the static `getProperties (const Element&)` parses (`jam_MarkdownDocument.cpp:1334-1345`) and that `isMetadataBlock` accepts (`jam_MarkdownBlockParser.cpp:1187-1206`). Names are only `getMetadataBlockText`, `lines`, `property`, `item`, `value`. No string `==`. No comments.

### Step 3: Scout — `Id::date`
**Scope:** `scout/cast/identifiers.md`, then the cast output `scout/Source/generated/Identifiers.h`, `scout/Source/generated/Generated.cpp`
**Action:** Add the row `| @id  | date | `date` | |` after `help` (`identifiers.md:42`), in the existing grid format. Run cast for scout (spell `cast/spell.md`). Do not edit generated files by hand.
**Validation:** `Identifiers.h` declares `date`. `Generated.cpp` defines it. No other generated line changes.

### Step 4: Scout — date at report write
**Scope:** `scout/Source/Processor.cpp`, `scout/Source/ProfileReport.h`, `scout/Source/ProfileReport.cpp`
**Action:**
1. `ProfileReport::toFile` becomes `static bool toFile (const juce::ValueTree& profiles, const juce::NamedValueSet& properties, const juce::File& outputFile);` (`.h:25`). In the body (`.cpp:4-12`), after `toDocument` and before the write, call `document.addProperties (properties);`.
2. In `Processor::writeReport` (`Processor.cpp:90-98`), declare `static constexpr const char* const dateTimeFormat { "%d-%m-%Y %H:%M" };` (renamed from `dateFormat` by Amendment A #7). Call `ProfileReport::toFile (profiles, { { Id::date, juce::Time::getCurrentTime().formatted (dateTimeFormat) } }, outputFile)`.
**Validation:** The time is read once, at report write (R5). Text form `29-10-2026 14:03` (R4). The only new names are `properties` and `dateTimeFormat`. `dateTimeFormat` is local (CODING.md narrowest scope). `Benchmark.cpp` does not change.

### Step 5: Scout — guide
**Scope:** `scout/Source/HELP.md`
**Action:**
1. Report section (`HELP.md:99-166`), after the `output` paragraph (`:107-109`): one statement that the report starts with front matter. It holds `date`, the time scout writes the report, as `DD-MM-YYYY HH:MM`.
2. After the baseline paragraph (`HELP.md:101-102`): one statement that scout reads the baseline after the epochs and before it writes the report. A manifest whose `baseline` names its own `output` thus compares each run with the previous run.
STE prose. No other section changes.
**Validation:** Text matches R3, R4, R5, R6 and `Processor.cpp:84`. Only `HELP.md` changes.

### Step 6: Build, audit, doxygen
**Scope:** the files of Steps 1-5
**Action:**
1. @Engineer builds scout Release with `CAST_SIGN` `OFF` (CLAUDE.md Build). ARCHITECT runs a manifest and tests.
2. @Auditor sweeps Steps 1-5 once.
3. COUNSELOR resolves each validated finding.
4. Doxygen pass: the public `addProperties (const juce::NamedValueSet&)`, the private `addProperties (Element&, const juce::NamedValueSet&)`, `getMetadataBlockText`, the `getBlocks` brief (`jam_MarkdownWriter.h:863`), and `ProfileReport::toFile` `@param properties`. Zero doxygen warnings.
**Validation:** Build is clean. Every finding is resolved or rejected with a read. Doxygen gives 0 warnings.

## BLESSED Alignment

- **B:** the run owns the date. `Processor` reads the clock once, at report write. JAM owns the block shape and the text form.
- **L:** one public call and one renderer. No new table, struct or file.
- **E:** the clock value passes as a parameter (`toFile` `properties`). Named format constant. Positive nesting.
- **S (SSOT):** the property children are the one model of the block. The writer renders from them. Scout writes no `---` text (R1).
- **S (Stateless):** no new member field.
- **E (Encapsulation):** scout calls a public JAM API. The target's samples file does not change.
- **D:** the measured rows and the baseline read do not change (`jam_ConfigDocument.cpp:119-132`).

## Risks / Open Questions

None. Each decision has an ARCHITECT ruling (R1-R6).

## Amendment A — Audit Dispositions

Execution corrections, before the audit:
- Step 5 anchors were wrong. The Report section is `HELP.md:99-166`. The paragraphs are at `:104-105` and `:114-115`.
- `jam_MarkdownWriter.cpp:586`: `lines.add (Chars::tripleDash)` was ambiguous over the four `add` overloads (`jam_Strings.h:228`, `:239`, `:245`, `:251`). It now uses the `std::string_view` overload (`:239`).

Accepted and resolved:
1. **#1** No assert on the precondition "no metadata block yet" (CAROL Code Contract: *"Preconditions use assert."*). Public `addProperties` asserts `getMetadataBlock() == nullptr`.
2. **#3** The walk `firstChild->nextSibling` appeared three times (MANIFESTO S: *"If the same logic appears more than twice, it must be abstracted."*). It is now the public static `MarkdownDocument::getFirstProperty`. Its callers are `getProperties() const`, `addPropertyLines` and `MarkdownWriter::getMetadataBlockText`.
3. **#5** Round trip was not stable for an empty value, an empty item, or a value with a newline (MANIFESTO D). The private `addProperties` asserts that each stored text is non-empty and on one line. A parse never gives such a value (`jam_MarkdownDocument.cpp:1330-1345`).
4. **#7** NAMES Rule 5. The family splits `dateFormat` from `dateTimeFormat` (`jam/cast/markdown.md:46`, `:48`). The constant holds a date and a time, so Step 4's `dateFormat` is now `dateTimeFormat`.
5. **#11** NAMES Rule 5. The non-const sibling bodies use `root` (`jam_MarkdownDocument.cpp:357`, `:408`, `:1500`). Public `addProperties` now uses `*root`.
6. **#12d** The search for the metadata block appeared four times (MANIFESTO S, more than twice). It is now the private `getMetadataBlock() const`. Its callers are `getProperties() const`, `getVersion`, `isEnabled (key)` and the #1 assert. The creation loop in `build` (`:392-394`) stays, so the logic appears twice.
7. **#12a** `MarkdownDocument::build` was 46 lines with two jobs (MANIFESTO L: *"30 lines per function — function is doing more than one thing."*). The ladder answers questions 1-4 with nothing. The token-to-line split moves into the block parser as `BlockParser::addLines (const Document::Tokens&)`. The name comes from the `addLine` family (`jam_MarkdownBlockParser.cpp:2463`, NAMES Rule 1 plural).
8. **#13** Doc-pass items go to Step 6.4. Also included: the `Processor::writeReport` brief, the `ProfileReport` class brief, and the stale `getVersion` prose (`jam_MarkdownDocument.h:455-461`).

Rejected:
- **#2** (empty `Id::text` child as a second truth). The built child holds `""`, which is not a copy of a property (`jam_MarkdownDocument.cpp:1369`). Its one reader, static `getProperties (const Element&)`, has one call site. That call site is `build` (`:394`), which runs on parsed blocks only. In a parsed block, the parse writes the text and the property children at creation. No code changes either one after creation, so the two cannot drift (MANIFESTO S: *"the same truth existing in two places that can drift"*).
- **#4** (add a child, then set `Id::type`). Only two sites take the id from `BlockTag` and set the same type: `jam_MarkdownBlockParser.cpp:86-87` (append) and `jam_MarkdownDocument.cpp:1367-1368` (insert at the head, `jam_Document.h:1037`). The other sites pass their own ids (`jam_ConfigDocument.cpp:190-193` row key, `:205` `HeadingTag`, `ProfileReport.cpp:169` `Id::mermaid`). The logic does not appear more than twice.
- **#6** Each token has one definition: `Chars::tripleDash` (`jam_Core.h:379`), and `Chars::dash`, `Chars::space` and `Chars::colon`. The parser and the writer both read those definitions. The composite `- ` appears twice, which is not more than twice.
- **#8** The `NamedValueSet` carries the date into the report document while `toFile` creates it. Creation is outside the transit rule (MANIFESTO E: *"Creation can use multiple passes, look-ahead, reverse steps, work buffers"*).
- **#9** `toFile` creates the report document. The document never leaves `toFile` before the write. MANIFESTO E: *"Creation does not change a complete object, because no complete object exists yet."*
- **#10** Both overloads add properties: one to the document, one to a given block. The overloads differ by their target parameter. `ConfigDocument::addProperty` is a private member of another object, with a different arity.
- **#12b, #12c** (file size of `jam_MarkdownDocument.cpp` and `jam_MarkdownWriter.cpp`). The findings list jobs (ladder question 5) but give no answers to questions 1-4. counselor.md Lean: *"Reject a finding that orders a split before the earlier questions are answered."*
- **#14** `HELP.md` uses one term, "front matter", at one site. That is ARCHITECT's word (R1).

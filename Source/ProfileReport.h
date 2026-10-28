#pragma once
#include <JuceHeader.h>
#include "Statistics.h"

/**
 * @brief Builds the profile trees and the report text of a scout run.
 *
 * A profile is a table. Its rows are zones. The report writes each profile as a markdown table.
 * The report starts with front matter.
 * It adds one mermaid median chart for each unit, and one distribution chart for each zone that holds bins.
 * The chart text comes from `jam::MermaidWriter` over `jam::MermaidDiagram::toDocument`.
 */
class ProfileReport
{
public:
    /**
     * @brief Writes the report of @p profiles to @p outputFile.
     *
     * The function builds the report document from @p profiles and adds the charts of each profile table.
     * It writes the document through `jam::MarkdownWriter`, which creates the parent folder of @p outputFile.
     * It adds @p properties as the front matter of the document.
     *
     * @param profiles    Tree of the profile tables that Benchmark::getProfiles() returns.
     * @param properties  The front matter properties of the report.
     * @param outputFile  File that receives the report.
     * @returns @c true when the write succeeds, or @c false when it fails.
     */
    static bool toFile (const juce::ValueTree& profiles, const juce::NamedValueSet& properties, const juce::File& outputFile);

    /**
     * @brief Returns one profile table with one row for each zone of @p samples.
     *
     * Each zone is complete when the function creates it. It holds one Statistics of the samples of the zone.
     * Each row holds `value` (the median), `errorPercent`, `min`, `max`, `calls`, `unit`, and `samples`.
     * A row whose zone the baseline profile also holds adds the comparison with the baseline zone of the same type:
     * `baseline`, `change` and `probability`.
     *
     * @param benchmarkType    Type of the table.
     * @param samples          Samples of each zone.
     * @param calls            Number of calls of each zone.
     * @param units            Unit text of each zone.
     * @param baselineProfile  Table of an earlier report. It may be invalid.
     * @returns The profile table.
     */
    static juce::ValueTree getProfile (const juce::Identifier& benchmarkType,
                                       const jam::HashMap<juce::Identifier, juce::Array<juce::var>>& samples,
                                       const jam::HashMap<juce::Identifier, int>& calls,
                                       const jam::HashMap<juce::Identifier, juce::String>& units,
                                       const juce::ValueTree& baselineProfile);

    /**
     * @brief Returns one profile table with one row for each zone of @p bins.
     *
     * Each zone is complete when the function creates it. It holds one Statistics of the bins of the zone.
     * Each row holds `value` (the median), `errorPercent`, `min`, `max`, `calls` (the sum of the bin counts), `unit`, `percentile` and `bins`.
     * A row whose zone the baseline profile also holds adds the comparison with the baseline zone of the same type:
     * `baseline`, `change` and `probability`.
     *
     * @param benchmarkType    Type of the table.
     * @param bins             Bin counts of each zone. The bin centres are in ns.
     * @param units            Unit text of each zone.
     * @param baselineProfile  Table of an earlier report. It may be invalid.
     * @returns The profile table.
     */
    static juce::ValueTree getProfile (const juce::Identifier& benchmarkType,
                                       const jam::HashMap<juce::Identifier, jam::Array<juce::int64>>& bins,
                                       const jam::HashMap<juce::Identifier, juce::String>& units,
                                       const juce::ValueTree& baselineProfile);

private:
    /**
     * @brief Returns one zone row with `value`, `errorPercent`, `min`, `max`, `calls` and `unit`.
     *
     * @param zoneKey     Type of the zone.
     * @param statistics  Statistics of the zone.
     * @param calls       Number of calls of the zone.
     * @param unit        Unit text of the zone.
     * @returns The zone row.
     */
    static juce::ValueTree getZone (const juce::Identifier& zoneKey, const Statistics& statistics, int calls, const juce::String& unit);

    /**
     * @brief Returns the Statistics of a baseline zone.
     *
     * The function reads the `bins` property when the zone holds it. Otherwise it reads the `samples` property.
     *
     * @param baselineZone  Zone of the baseline.
     * @returns The Statistics of @p baselineZone.
     */
    static Statistics getStatistics (const juce::ValueTree& baselineZone);

    /**
     * @brief Adds `baseline`, `change` and `probability` to a zone.
     *
     * @param zone          Zone of the current run.
     * @param statistics    Statistics of the zone.
     * @param baselineZone  Zone of the baseline with the same type.
     */
    static void addComparison (juce::ValueTree& zone, const Statistics& statistics, const juce::ValueTree& baselineZone);

    /**
     * @brief Adds the charts of one profile table.
     *
     * The function adds one median chart for each unit that the zones of @p benchmark use.
     * It then adds one distribution chart for each zone that holds bins.
     *
     * @param document   Document that receives the charts.
     * @param benchmark  Profile table.
     */
    static void addChart (jam::ConfigDocument& document, const juce::ValueTree& benchmark);

    /**
     * @brief Adds one mermaid bar chart of the zones that have @p unit.
     *
     * The bars are the `value` of each zone. The chart title is the type of @p benchmark. The y axis is @p unit.
     *
     * @param document   Document that receives the chart.
     * @param benchmark  Profile table.
     * @param unit       Unit of the zones to chart.
     */
    static void addChart (jam::ConfigDocument& document, const juce::ValueTree& benchmark, const juce::Identifier& unit);

    /**
     * @brief Adds one mermaid bar chart.
     *
     * The chart text comes from `jam::MermaidWriter` over `jam::MermaidDiagram::toDocument`.
     * The document holds the text in a mermaid code block.
     *
     * @param document    Document that receives the chart.
     * @param title       Title of the chart.
     * @param categories  Label of each bar on the x axis.
     * @param yAxisLabel  Label of the y axis.
     * @param values      Height of each bar, in the order of @p categories.
     */
    static void addChart (jam::ConfigDocument& document,
                          const juce::String& title,
                          const juce::Array<juce::var>& categories,
                          const juce::String& yAxisLabel,
                          const juce::Array<juce::var>& values);

    /**
     * @brief Adds one bar chart of the calls of a zone for each octave.
     *
     * The function sums the bins of each octave. The chart runs from the first to the last octave that holds a call.
     * The x axis is the lower edge of each octave, in ns. The y axis is `calls`. The title is the type of @p zone.
     *
     * @param document  Document that receives the chart.
     * @param zone      Zone of a `_CALLS` table. It holds the `bins` property.
     */
    static void addDistribution (jam::ConfigDocument& document, const juce::ValueTree& zone);
};

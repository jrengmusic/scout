#include "ProfileReport.h"
#include "generated/Generated.h"

bool ProfileReport::toFile (const juce::ValueTree& profiles, const juce::File& outputFile)
{
    auto document { jam::ConfigDocument::toDocument (profiles) };

    for (const auto& benchmark : profiles)
        addChart (document, benchmark);

    return jam::MarkdownWriter { jam::MarkdownWriter::defaultLineWrap, { { Id::samples, jam::Profiler::samplesColumnWidth }, { Id::bins, jam::Profiler::samplesColumnWidth } } }.toFile (document, outputFile);
}

juce::ValueTree ProfileReport::getProfile (const juce::Identifier& benchmarkType,
                                           const jam::HashMap<juce::Identifier, juce::Array<juce::var>>& samples,
                                           const jam::HashMap<juce::Identifier, int>& calls,
                                           const jam::HashMap<juce::Identifier, juce::String>& units,
                                           const juce::ValueTree& baselineProfile)
{
    juce::ValueTree profile { benchmarkType };

    for (const auto& [zoneKey, zoneSamples] : samples)
    {
        const Statistics statistics { zoneSamples };
        auto zone { getZone (zoneKey, statistics, calls.at (zoneKey), units.at (zoneKey)) };
        zone.setProperty (Id::samples, juce::var (zoneSamples), nullptr);

        const auto baselineZone { baselineProfile.getChildWithName (zoneKey) };

        if (baselineZone.isValid())
            addComparison (zone, statistics, baselineZone);

        profile.appendChild (zone, nullptr);
    }

    return profile;
}

juce::ValueTree ProfileReport::getProfile (const juce::Identifier& benchmarkType,
                                           const jam::HashMap<juce::Identifier, jam::Array<juce::int64>>& bins,
                                           const jam::HashMap<juce::Identifier, juce::String>& units,
                                           const juce::ValueTree& baselineProfile)
{
    juce::ValueTree profile { benchmarkType };

    for (const auto& [zoneKey, zoneBins] : bins)
    {
        juce::Array<juce::var> binCounts;

        for (const auto binCount : zoneBins)
            binCounts.add (juce::var (binCount));

        const Statistics statistics { zoneBins };
        auto zone { getZone (zoneKey, statistics, static_cast<int> (statistics.getCount()), units.at (zoneKey)) };
        zone.setProperty (Id::percentile, statistics.getPercentile(), nullptr);
        zone.setProperty (Id::bins, juce::var (binCounts), nullptr);

        const auto baselineZone { baselineProfile.getChildWithName (zoneKey) };

        if (baselineZone.isValid())
            addComparison (zone, statistics, baselineZone);

        profile.appendChild (zone, nullptr);
    }

    return profile;
}

juce::ValueTree ProfileReport::getZone (const juce::Identifier& zoneKey, const Statistics& statistics, int calls, const juce::String& unit)
{
    juce::ValueTree zone { zoneKey };
    zone.setProperty (Id::value, statistics.getMedian(), nullptr);
    zone.setProperty (Id::errorPercent, statistics.getErrorPercent(), nullptr);
    zone.setProperty (Id::min, statistics.getMinimum(), nullptr);
    zone.setProperty (Id::max, statistics.getMaximum(), nullptr);
    zone.setProperty (Id::calls, calls, nullptr);
    zone.setProperty (Id::unit, unit, nullptr);

    return zone;
}

Statistics ProfileReport::getStatistics (const juce::ValueTree& baselineZone)
{
    const auto distributionKey { baselineZone.hasProperty (Id::bins) ? Id::bins : Id::samples };
    const auto distributionText { baselineZone[distributionKey].toString() };
    auto distributionView { jam::Strings::getView (distributionText) };
    const auto result { jam::ConfigDocument::getValueTypes().get (Id::numbers, distributionView) };
    const auto& numbers { *result.getArray() };

    if (baselineZone.hasProperty (Id::bins))
    {
        jam::Array<juce::int64> zoneBins;

        for (const auto& number : numbers)
            zoneBins.add (static_cast<juce::int64> (number));

        return Statistics { zoneBins };
    }

    return Statistics { numbers };
}

void ProfileReport::addComparison (juce::ValueTree& zone, const Statistics& statistics, const juce::ValueTree& baselineZone)
{
    const auto median { static_cast<double> (zone[Id::value]) };
    const auto baselineMedian { static_cast<double> (baselineZone[Id::value]) };

    zone.setProperty (Id::baseline, baselineMedian, nullptr);

    zone.setProperty (Id::change,
                      juce::exactlyEqual (baselineMedian, 0.0)
                          ? juce::var { juce::String() }
                          : juce::var { (median - baselineMedian) / baselineMedian * Statistics::percentScale },
                      nullptr);

    zone.setProperty (Id::probability, Statistics::getProbability (statistics, getStatistics (baselineZone)), nullptr);
}

void ProfileReport::addChart (jam::ConfigDocument& document, const juce::ValueTree& benchmark)
{
    juce::Array<juce::Identifier> units;

    for (const auto& zone : benchmark)
        units.addIfNotAlreadyThere (juce::Identifier { zone[Id::unit].toString() });

    for (const auto& unit : units)
        addChart (document, benchmark, unit);

    for (const auto& zone : benchmark)
        if (zone.hasProperty (Id::bins))
            addDistribution (document, zone);
}

void ProfileReport::addChart (jam::ConfigDocument& document, const juce::ValueTree& benchmark, const juce::Identifier& unit)
{
    juce::Array<juce::var> zones;
    juce::Array<juce::var> medians;

    for (const auto& zone : benchmark)
    {
        if (juce::Identifier { zone[Id::unit].toString() } == unit)
        {
            zones.add (zone.getType().toString());
            medians.add (zone[Id::value]);
        }
    }

    addChart (document, benchmark.getType().toString(), zones, unit.toString(), medians);
}

void ProfileReport::addChart (jam::ConfigDocument& document,
                              const juce::String& title,
                              const juce::Array<juce::var>& categories,
                              const juce::String& yAxisLabel,
                              const juce::Array<juce::var>& values)
{
    static constexpr int fenceWidth { 3 };

    static const jam::SharedInstance<map::MermaidDiagramType> diagramTypes { std::in_place };

    const juce::ValueTree chart { diagramTypes->get (map::MermaidDiagramType::xychartBeta),
                                  { { Id::title, title },
                                    { Id::xAxisCategories, juce::var (categories) },
                                    { Id::yAxisLabel, yAxisLabel } },
                                  { { Id::node,
                                      { { Id::type, int { map::MermaidXy::Keyword::bar } },
                                        { Id::values, juce::var (values) } } } } };

    auto* block { document.addChild (*document.getRoot(), Id::mermaid) };
    block->add<int> (Id::type, map::BlockType::mermaid);
    block->add<juce::String> (Id::marker, juce::String::repeatedString (juce::String::charToString (Chars::backtick), fenceWidth));
    block->add<juce::String> (Id::info, Id::mermaid.toString());

    auto* chartText { document.addChild (*block, Id::text) };
    chartText->add<juce::String> (Id::text, jam::MermaidWriter {}.getText (jam::MermaidDiagram::toDocument (chart)));
}

void ProfileReport::addDistribution (jam::ConfigDocument& document, const juce::ValueTree& zone)
{
    const auto& binCounts { *zone[Id::bins].getArray() };
    std::array<juce::int64, jam::Profiler::numOctaves> octaveCounts {};

    for (int bin { 0 }; bin < binCounts.size(); ++bin)
        octaveCounts.at (static_cast<size_t> (bin / jam::Profiler::binsPerOctave)) += static_cast<juce::int64> (binCounts.getReference (bin));

    const auto isOccupied { [] (juce::int64 count) { return count > 0; } };
    const auto firstOctave { static_cast<int> (std::distance (octaveCounts.begin(), std::find_if (octaveCounts.begin(), octaveCounts.end(), isOccupied))) };
    const auto lastOctave { static_cast<int> (std::distance (octaveCounts.begin(), std::find_if (octaveCounts.rbegin(), octaveCounts.rend(), isOccupied).base())) - 1 };

    jassert (firstOctave <= lastOctave);

    juce::Array<juce::var> categories;
    juce::Array<juce::var> counts;

    for (int octave { firstOctave }; octave <= lastOctave; ++octave)
    {
        categories.add (juce::String (static_cast<juce::int64> (jam::Profiler::getBinNanoseconds (octave * jam::Profiler::binsPerOctave))));
        counts.add (static_cast<double> (octaveCounts.at (static_cast<size_t> (octave))));
    }

    addChart (document, zone.getType().toString(), categories, Id::calls.toString(), counts);
}

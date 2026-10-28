#include "Statistics.h"

Statistics::Statistics (const juce::Array<juce::var>& samples)
{
    static constexpr juce::int64 sampleCount { 1 };

    jassert (not samples.isEmpty());

    for (const auto& sample : samples)
        sorted.add (static_cast<double> (sample));

    sorted.sort();

    for (int index { 0 }; index < sorted.size(); ++index)
        counts.add (sampleCount);
}

Statistics::Statistics (const jam::Array<juce::int64>& bins)
{
    jassert (bins.size() == jam::Profiler::numBins);

    for (int bin { 0 }; bin < jam::Profiler::numBins; ++bin)
    {
        if (bins.at (bin) > 0)
        {
            sorted.add (jam::Profiler::getBinNanoseconds (static_cast<double> (bin) + half));
            counts.add (bins.at (bin));
        }
    }

    jassert (not sorted.isEmpty());
}

double Statistics::getPercentile (const jam::Array<double>& sortedValues, const jam::Array<juce::int64>& valueCounts, double rank) noexcept
{
    const auto total { std::accumulate (valueCounts.begin(), valueCounts.end(), juce::int64 { 0 }) };
    const auto position { rank / percentScale * static_cast<double> (total - 1) };
    const auto lowerIndex { static_cast<juce::int64> (std::floor (position)) };
    const auto upperIndex { static_cast<juce::int64> (std::ceil (position)) };
    const auto fraction { position - std::floor (position) };

    const auto getSortedValue { [&sortedValues, &valueCounts] (juce::int64 virtualIndex)
    {
        juce::int64 cumulative { 0 };

        for (int index { 0 }; index < sortedValues.size(); ++index)
        {
            cumulative += valueCounts.at (index);

            if (virtualIndex < cumulative) return sortedValues.at (index);
        }

        return sortedValues.last();
    } };

    const auto lower { getSortedValue (lowerIndex) };
    const auto upper { getSortedValue (upperIndex) };

    return lower + (upper - lower) * fraction;
}

double Statistics::getMedian() const noexcept
{
    return getPercentile (sorted, counts, medianRank);
}

double Statistics::getPercentile() const noexcept
{
    return getPercentile (sorted, counts, percentileRank);
}

double Statistics::getErrorPercent() const
{
    const auto median { getMedian() };
    const auto getDeviation { [this, median] (int index)
    {
        return std::abs (sorted.at (index) - median);
    } };

    jam::Array<int> order;

    for (int index { 0 }; index < sorted.size(); ++index)
        order.add (index);

    std::sort (order.begin(), order.end(), [&getDeviation] (int first, int second) { return getDeviation (first) < getDeviation (second); });

    jam::Array<double> deviations;
    jam::Array<juce::int64> deviationCounts;

    for (const auto index : order)
    {
        deviations.add (getDeviation (index));
        deviationCounts.add (counts.at (index));
    }

    if (not juce::exactlyEqual (median, 0.0)) return getPercentile (deviations, deviationCounts, medianRank) / median * percentScale;
    return 0.0;
}

juce::int64 Statistics::getCount() const noexcept
{
    return std::accumulate (counts.begin(), counts.end(), juce::int64 { 0 });
}

double Statistics::getMinimum() const noexcept
{
    return sorted.first();
}

double Statistics::getMaximum() const noexcept
{
    return sorted.last();
}

double Statistics::getProbability (const Statistics& current, const Statistics& baseline)
{
    static constexpr double rankOffset { 1.0 };
    static constexpr double varianceDivisor { 12.0 };

    const auto getRanks { [] (const Statistics& statistics, double value)
    {
        const auto [first, last] { std::equal_range (statistics.sorted.begin(), statistics.sorted.end(), value) };
        const auto firstCount { statistics.counts.begin() + (first - statistics.sorted.begin()) };
        const auto lastCount { statistics.counts.begin() + (last - statistics.sorted.begin()) };

        return std::pair<juce::int64, juce::int64> { std::accumulate (statistics.counts.begin(), firstCount, juce::int64 { 0 }),
                                                     std::accumulate (firstCount, lastCount, juce::int64 { 0 }) };
    } };

    const auto currentCount { static_cast<double> (current.getCount()) };
    const auto baselineCount { static_cast<double> (baseline.getCount()) };

    double rankSum { 0.0 };

    for (int index { 0 }; index < current.sorted.size(); ++index)
    {
        const auto [currentBelow, currentTied] { getRanks (current, current.sorted.at (index)) };
        const auto [baselineBelow, baselineTied] { getRanks (baseline, current.sorted.at (index)) };

        rankSum += static_cast<double> (current.counts.at (index))
                 * (static_cast<double> (currentBelow + baselineBelow) + (static_cast<double> (currentTied + baselineTied) + rankOffset) * half);
    }

    const auto pairCount { currentCount * baselineCount };
    const auto deviation { std::sqrt (pairCount * (currentCount + baselineCount + rankOffset) / varianceDivisor) };
    const auto standardScore { (rankSum - currentCount * (currentCount + rankOffset) * half - pairCount * half) / deviation };

    return std::erfc (std::abs (standardScore) / juce::MathConstants<double>::sqrt2);
}

#pragma once
#include <JuceHeader.h>

/**
 * @brief Statistics of weighted values.
 *
 * The object holds sorted values. Each value has a count.
 * A list of samples gives a count of 1 for each sample. The bins of a profiler give the bin centre, in ns, with the bin count.
 * Each getter reads the sorted values and the counts.
 */
class Statistics
{
public:
    /** @brief Number that converts a percentage to a fraction. */
    static constexpr double percentScale { 100.0 };

    /** @brief Rank, in percent, that `getPercentile` reads. */
    static constexpr double percentileRank { 99.9 };

    /**
     * @brief Converts the samples to doubles and sorts them.
     *
     * @param samples  Numeric samples. The list must not be empty (jassert).
     */
    explicit Statistics (const juce::Array<juce::var>& samples);

    /**
     * @brief Converts the bins of a profiler to values with counts.
     *
     * Each bin with a count above 0 gives one value, the centre of the bin in ns, with the count of the bin.
     *
     * @param bins  One count for each bin. The size must be `jam::Profiler::numBins` (jassert). One count must be above 0 (jassert).
     */
    explicit Statistics (const jam::Array<juce::int64>& bins);

    /**
     * @brief Returns the median of the weighted values.
     *
     * The rank of the median is `medianRank`.
     *
     * @returns The median.
     */
    double getMedian() const noexcept;

    /**
     * @brief Returns the weighted value at `percentileRank`.
     *
     * The position in the weighted values is `percentileRank / percentScale * (total count - 1)`.
     * The result is the linear interpolation between the two values around that position.
     *
     * @returns The value at `percentileRank`.
     */
    double getPercentile() const noexcept;

    /**
     * @brief Returns the weighted median of the absolute deviations from the median, as a percentage of the median.
     *
     * @returns The error in percent of the median, or 0 when the median is 0.
     */
    double getErrorPercent() const;

    /**
     * @brief Returns the sum of the counts of all values.
     *
     * @returns The total count.
     */
    juce::int64 getCount() const noexcept;

    /**
     * @brief Returns the smallest sample.
     *
     * @returns The first sorted value.
     */
    double getMinimum() const noexcept;

    /**
     * @brief Returns the largest sample.
     *
     * @returns The last sorted value.
     */
    double getMaximum() const noexcept;

    /**
     * @brief Returns the probability that two weighted distributions are the same.
     *
     * The function runs the Mann-Whitney U test with the normal approximation. Tied values get the mean rank.
     * Each value counts as often as its count. It returns the two-sided p-value.
     *
     * @param current   Statistics of the current run.
     * @param baseline  Statistics of the baseline run.
     * @returns The two-sided p-value.
     */
    static double getProbability (const Statistics& current, const Statistics& baseline);

private:
    /** @brief Half of one: the offset of a bin centre from the start of its bin, and the factor of the mean rank in the Mann-Whitney U test. */
    static constexpr double half { 0.5 };

    /** @brief Rank, in percent, of the median. */
    static constexpr double medianRank { 50.0 };

    /**
     * @brief Returns the weighted value at a rank.
     *
     * The function reads each value as often as its count and interpolates linearly between the two neighbours of the rank position.
     *
     * @param sortedValues  Values in ascending order.
     * @param valueCounts   Count of each value, in the order of @p sortedValues.
     * @param rank          Rank in percent.
     * @returns The interpolated value at @p rank.
     */
    static double getPercentile (const jam::Array<double>& sortedValues, const jam::Array<juce::int64>& valueCounts, double rank) noexcept;

    /** @brief Values in ascending order. */
    jam::Array<double> sorted;

    /** @brief Count of each value, in the order of `sorted`. */
    jam::Array<juce::int64> counts;
};

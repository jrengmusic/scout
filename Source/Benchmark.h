#pragma once
#include <JuceHeader.h>

#if JUCE_MAC or JUCE_LINUX
#include <sys/resource.h>
#endif

/**
 * @brief Runs a command once for each epoch and collects its samples.
 *
 * Each epoch runs the command and reads the samples file that the command writes.
 * The object keeps the epoch samples of each row, and the merged bins of the calls of all epochs. `getProfiles` builds the profiles from them.
 */
class Benchmark
{
public:
    /**
     * @brief Sets the files of the run.
     *
     * The constructor also sets one call and the `ns` unit for the `epoch`, `user` and `system` rows.
     *
     * @param newCommandFile   Executable that each epoch runs.
     * @param newSamplesFile   File that the command writes in each epoch.
     * @param newManifestFile  Manifest file. It is the one argument of the command, and its folder is the working folder.
     */
    Benchmark (const juce::File& newCommandFile, const juce::File& newSamplesFile, const juce::File& newManifestFile);

    /**
     * @brief Runs one epoch.
     *
     * The function deletes the old samples file. It launches `command manifest` in the manifest folder and waits until the command ends.
     * It adds the wall time of the epoch, in ns, to the `epoch` samples.
     * On macOS and Linux it also adds the user and system time of the child, in ns, to the `user` and `system` samples.
     *
     * @return  Ok when the command exits with 0. Otherwise fail with `\<command path\>: command failed`.
     */
    juce::Result runEpoch();

    /**
     * @brief Reads the samples file of the last epoch.
     *
     * The function parses the file. Each table except `run` adds its rows with `addRows`.
     * The type of the last such table becomes the profile type.
     * The `run` table is kept for the report.
     * Every row holds one sample for each epoch. A row that the file does not hold adds 0 for this epoch.
     * A new row starts with 0 for each earlier epoch.
     *
     * @return  Ok when the file exists. Otherwise fail with `not found: \<samples path\>`.
     */
    juce::Result addEpoch();

    /**
     * @brief Returns the profiles of all epochs.
     *
     * The tree holds three tables in this order. The epoch table has the profile type and one row for each zone.
     * The call table has the type `\<TYPE\>_CALLS` and one row for each zone that holds bins, with the `percentile` property.
     * Its statistics come from the merged bins of all epochs.
     * The `run` table is the one that the samples file holds.
     * Each zone that the matching table of @p baseline also holds adds the comparison properties.
     *
     * @param baseline  Tree of an earlier report. It may hold no matching table.
     *
     * @return  Tree of the three tables.
     */
    juce::ValueTree getProfiles (const juce::ValueTree& baseline) const;

private:
    /**
     * @brief Runs `command manifest` in the manifest folder and waits until it ends.
     *
     * @return  Exit code of the command.
     */
    int runCommand();

    /**
     * @brief Adds the rows of a table to the epoch samples, the bins, the calls and the units.
     *
     * A row adds its `value` to the epoch samples, and sets the calls and the unit of its zone.
     * A row with a `bins` text also adds its bins with `addBins`.
     *
     * @param table  Table of the samples file.
     */
    void addRows (const juce::ValueTree& table);

    /**
     * @brief Adds the bin counts of a row to the merged bins of the row.
     *
     * The function parses the text and adds each count to the bin of the same index. The text holds `jam::Profiler::numBins` counts (jassert).
     *
     * @param rowKey    Type of the row.
     * @param binsText  Text of the `bins` column of the row.
     */
    void addBins (const juce::Identifier& rowKey, const juce::String& binsText);
#if JUCE_MAC or JUCE_LINUX
    /**
     * @brief Adds the user and system time that the children used between two readings.
     *
     * @param before  Resource usage of the children before the epoch.
     * @param after   Resource usage of the children after the epoch.
     */
    void addChildTime (const rusage& before, const rusage& after);
#endif

    /** @brief Executable that each epoch runs. */
    const juce::File commandFile;

    /** @brief File that the command writes in each epoch. */
    const juce::File samplesFile;

    /** @brief Manifest file. It is the one argument of the command. */
    const juce::File manifestFile;

    /** @brief One sample for each epoch, for each row. */
    jam::HashMap<juce::Identifier, juce::Array<juce::var>> epochSamples;

    /** @brief The bin counts of the calls of all epochs, merged, for each row that holds bins. */
    jam::HashMap<juce::Identifier, jam::Array<juce::int64>> bins;

    /** @brief Number of calls of each row. */
    jam::HashMap<juce::Identifier, int> calls;

    /** @brief Unit text of each row. */
    jam::HashMap<juce::Identifier, juce::String> units;

    /** @brief The `run` table of the samples file. */
    juce::ValueTree runTable;

    /** @brief Type of the table that holds the zone rows. */
    juce::Identifier tableType;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Benchmark)
};

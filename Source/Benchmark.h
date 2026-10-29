#pragma once
#include <JuceHeader.h>

#if JUCE_MAC or JUCE_LINUX
#include <sys/resource.h>
#endif

/**
 * @brief Runs a command once for each epoch and collects its samples.
 *
 * Each epoch runs the command and reads the samples file that the command writes.
 * When the manifest has a `## sample` table, each epoch also runs a Sampler.
 * The object keeps one set of data for each table type: the table type of the samples file, `sample` and `cswitch`.
 * For each table type it keeps the epoch samples, the calls, the units and the merged bins of each row.
 * The `epoch`, `user` and `system` rows belong to the table type of the samples file.
 * `getProfiles` builds the profiles from them.
 */
class Benchmark
{
public:
    /**
     * @brief Sets the files of the run.
     *
     * @param newCommandFile   Executable that each epoch runs.
     * @param newSamplesFile   File that the command writes in each epoch.
     * @param newManifestFile  Manifest file. It is the one argument of the command, and its folder is the working folder.
     * @param newSampleTable   The `sample` table of the manifest. When it is not valid, no epoch runs a Sampler.
     */
    Benchmark (const juce::File& newCommandFile, const juce::File& newSamplesFile, const juce::File& newManifestFile, const juce::ValueTree& newSampleTable);

    /**
     * @brief Runs one epoch.
     *
     * The function deletes the old samples file. It starts a Sampler when the manifest has a `## sample` table.
     * It launches `command manifest` in the manifest folder and waits until the command ends.
     * It then drains the Sampler and reads the samples file through `addEpoch`.
     * It adds the wall time of the epoch, in ns, to the `epoch` samples.
     * On all systems it also adds the user and system time of the child, in ns, to the `user` and `system` samples.
     * It last pads each row that has no sample in this epoch with zero.
     *
     * @return  Ok when all steps succeed. Otherwise the first failure:
     *          `\<command path\>: command failed` when the command does not exit with 0,
     *          the failure of the Sampler when it cannot start, read or stop,
     *          or `not found: \<samples path\>` when the command writes no samples file.
     */
    juce::Result runEpoch();

    /**
     * @brief Returns the profiles of all epochs.
     *
     * The tree holds one table for each table type, with one row for each zone.
     * For each table type that holds bins, it also holds a table of the type `\<TYPE\>_CALLS`.
     * That table has one row for each zone that holds bins, with the `percentile` property.
     * Its statistics come from the merged bins of all epochs.
     * The last table is the `run` table that the samples file holds.
     * Each zone that the matching table of @p baseline also holds adds the comparison properties.
     *
     * @param baseline  Tree of an earlier report. It may hold no matching table.
     *
     * @return  Tree of the profile tables.
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
     * A row adds its `value` to the epoch samples of its table type, and sets the calls and the unit of its zone.
     * A row that is new in this table type first receives a zero sample for each earlier epoch.
     * A row with a `bins` text also adds its bins with `addBins`.
     *
     * @param benchmarkType  Table type that receives the rows.
     * @param table          Table of the samples file or of the Sampler.
     */
    void addRows (const juce::Identifier& benchmarkType, const juce::ValueTree& table);

    /**
     * @brief Reads the samples file and adds its rows, the Sampler rows and the wall time of the epoch.
     *
     * The `run` table of the file is kept for the report. Each other table adds its rows through `addRows`.
     *
     * @param epochNanoseconds  Wall time of the epoch, in ns.
     * @param samplerSamples    Samples of the Sampler. The tree is not valid when no Sampler ran.
     *
     * @return  Ok when the samples file exists. Otherwise `not found: \<samples path\>`.
     */
    juce::Result addEpoch (double epochNanoseconds, const juce::ValueTree& samplerSamples);

    /**
     * @brief Adds one sample to a row of the table type of the samples file.
     *
     * The function also sets the calls of the row to the calls of one epoch, and its unit to `ns`.
     *
     * @param rowKey       Type of the row.
     * @param nanoseconds  Value of the sample, in ns.
     */
    void addRow (const juce::Identifier& rowKey, double nanoseconds);

    /**
     * @brief Adds a zero sample to each row that has fewer samples than the number of epochs.
     */
    void addMissingSamples();

    /**
     * @brief Adds the bin counts of a row to the merged bins of the row.
     *
     * The function parses the text and adds each count to the bin of the same index. The text holds `jam::Profiler::numBins` counts (jassert).
     *
     * @param benchmarkType  Table type of the row.
     * @param rowKey         Type of the row.
     * @param binsText       Text of the `bins` column of the row.
     */
    void addBins (const juce::Identifier& benchmarkType, const juce::Identifier& rowKey, const juce::String& binsText);
#if JUCE_MAC or JUCE_LINUX
    /**
     * @brief Returns the resource usage of all children that ended.
     *
     * @return  Resource usage of the children.
     */
    rusage getChildUsage() const;

    /**
     * @brief Adds the user and system time that the children used between two readings.
     *
     * The function adds the `user` and `system` rows, in ns.
     *
     * @param before  Resource usage of the children before the epoch.
     * @param after   Resource usage of the children after the epoch.
     */
    void addChildTime (const rusage& before, const rusage& after);
#elif JUCE_WINDOWS
    /**
     * @brief Returns the accounting information of the job object, without the time of this process.
     *
     * The job object holds this process and its children.
     *
     * @return  Accounting information of the children.
     */
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION getChildUsage() const;

    /**
     * @brief Adds the user and kernel time that the children used between two readings.
     *
     * The function adds the `user` row and the `system` row, in ns. The system time is the kernel time.
     *
     * @param before  Accounting information before the epoch.
     * @param after   Accounting information after the epoch.
     */
    void addChildTime (const JOBOBJECT_BASIC_ACCOUNTING_INFORMATION& before, const JOBOBJECT_BASIC_ACCOUNTING_INFORMATION& after);
#endif

    /** @brief Executable that each epoch runs. */
    const juce::File commandFile;

    /** @brief File that the command writes in each epoch. */
    const juce::File samplesFile;

    /** @brief Manifest file. It is the one argument of the command. */
    const juce::File manifestFile;

#if JUCE_WINDOWS
    /** @brief Job object that holds this process and its children. It accounts the time of the children. */
    std::unique_ptr<void, BOOL (WINAPI*) (HANDLE)> job;
#endif

    /** @brief The `sample` table of the manifest. When it is not valid, no epoch runs a Sampler. */
    const juce::ValueTree sampleTable;

    /** @brief One sample for each epoch, for each row, for each table type. */
    jam::HashMap<juce::Identifier, jam::HashMap<juce::Identifier, juce::Array<juce::var>>> epochSamples;

    /** @brief The bin counts of the calls of all epochs, merged, for each row that holds bins, for each table type. */
    jam::HashMap<juce::Identifier, jam::HashMap<juce::Identifier, jam::Array<juce::int64>>> bins;

    /** @brief Number of calls of each row, for each table type. */
    jam::HashMap<juce::Identifier, jam::HashMap<juce::Identifier, int>> calls;

    /** @brief Unit text of each row, for each table type. */
    jam::HashMap<juce::Identifier, jam::HashMap<juce::Identifier, juce::String>> units;

    /** @brief The `run` table of the samples file. */
    juce::ValueTree runTable;

    /** @brief Type of the table of the samples file that holds the zone rows. */
    juce::Identifier tableType;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Benchmark)
};

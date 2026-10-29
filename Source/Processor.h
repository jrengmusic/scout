#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Validator.h"
#include "Toolchain.h"
#include "Benchmark.h"
#include "ProfileReport.h"

/**
 * @struct Processor
 * @brief Owns the parsed manifest of one scout run and drives the run
 *        through it.
 *
 * Processor parses the manifest file once, at construction. generate() then
 * validates the manifest and runs the `## toolchain` rows. When the manifest
 * holds a `run` table, generate() also runs the epochs and writes the report.
 * When the manifest also holds a `## sample` table, each epoch samples the
 * stacks of the command.
 */
struct Processor
{
    /**
     * @brief Parses @p newDocumentFile into the manifest.
     *
     * @param newDocumentFile  The manifest file. Its folder resolves the
     *                         paths of the `run` table.
     */
    explicit Processor (const juce::File& newDocumentFile);

    /**
     * @brief Runs the manifest.
     *
     * The run has three steps, in this order. The function validates the
     * manifest through Validator. It runs every `## toolchain` row through
     * run(). It then runs the epochs and writes the report, when the manifest
     * holds a `run` table. Each step runs only when the earlier steps
     * succeed.
     *
     * @returns juce::Result::ok() when every step succeeds, or the first
     *          failure.
     */
    juce::Result generate();

private:
    /**
     * @brief Runs every `## toolchain` row, in table order and row order.
     *
     * Each row runs in the working folder of the caller, not in the folder of
     * the manifest. The run stops at the first row that fails.
     *
     * @returns juce::Result::ok() when every row exits with zero, or the
     *          first failure.
     */
    juce::Result run();

    /**
     * @brief Runs the `command` of @p row, followed by its `flag`, through
     *        Toolchain::runProcess().
     *
     * The command line in a failure is the `command`, followed by the `flag`
     * when the row declares one.
     *
     * @param row  The `## toolchain` row whose `command` and `flag` run.
     * @returns juce::Result::ok() when the process starts and exits with
     *          zero, or a failure that names its command line.
     */
    juce::Result runToolchainRow (const jam::ConfigDocument::Element& row);

    /**
     * @brief Runs the epochs of @p runTable and writes the report.
     *
     * The paths of @p runTable resolve against the folder of the manifest.
     * When the `baseline` cell names a file that does not exist, the function
     * fails before it runs any epoch. An empty `baseline` cell means no
     * baseline. Each epoch runs the `command` through Benchmark::runEpoch().
     * That function also reads the `samples` file, and it runs a Sampler when
     * @p sampleTable is valid. The function runs `epochs` epochs, then passes
     * the profiles to writeReport().
     *
     * @param runTable     The `run` table of the manifest.
     * @param sampleTable  The `sample` table of the manifest. It is not valid
     *                     when the manifest has no `## sample` table.
     * @returns juce::Result::ok() when every epoch and the report succeed, or
     *          the first failure. A missing baseline gives
     *          `not found: \<baseline path\>`.
     */
    juce::Result runEpochs (const juce::ValueTree& runTable, const juce::ValueTree& sampleTable);

    /**
     * @brief Writes the report of @p profiles to the `output` file of
     *        @p runTable, through ProfileReport::toFile().
     *
     * The front matter of the report holds `date`, the time of the write, as `DD-MM-YYYY HH:MM`.
     * The `output` path resolves against the folder of the manifest.
     *
     * @param runTable  The `run` table of the manifest.
     * @param profiles  The profiles that Benchmark::getProfiles() returns.
     * @returns juce::Result::ok() when the write succeeds, or a failure
     *          `\<output path\>: cannot write output`.
     */
    juce::Result writeReport (const juce::ValueTree& runTable, const juce::ValueTree& profiles);

    /** @brief The manifest file. Its folder resolves the paths of the `run` table. */
    const juce::File documentFile;

    /** @brief The parsed manifest. */
    const jam::ConfigDocument manifest;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Processor)
};

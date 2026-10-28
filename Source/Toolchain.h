#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @struct Toolchain
 * @brief Tokenizes the cells of a @c ## toolchain row into an argv array
 *        and runs child processes.
 *
 * Toolchain keeps no state.
 */
struct Toolchain
{
    /**
     * @brief Tokenizes @p command and @p flag into one argv array.
     *
     * The @p flag text splits on white space. A span in double quotes stays
     * one argument. The function removes the quote marks and drops empty
     * tokens. The @p command text is not split.
     *
     * @param command The toolchain row's own @c command, placed at
     *                argument index zero.
     * @param flag    The toolchain row's own @c flag, tokenized after
     *                @p command.
     * @returns @p command followed by the arguments of @p flag.
     */
    static juce::StringArray getToolchainArguments (const juce::String& command, const juce::String& flag);

    /**
     * @brief Runs @p arguments as a child process in the current working
     *        directory, streaming its output to stdout and blocking until
     *        it exits.
     *
     * @param arguments      The process argv, index zero the executable.
     * @param diagnosticLine The command line named in a failure, when the
     *                       process exits non-zero.
     * @returns juce::Result::ok() when the process exits zero, or a
     *          failure naming @p diagnosticLine.
     */
    static juce::Result runProcess (const juce::StringArray& arguments, const juce::String& diagnosticLine);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Toolchain)
};

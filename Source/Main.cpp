/**
 * @file Main.cpp
 * @brief `scout` CLI entry point: banner/help rendering and dispatch of
 *        the version, help, and manifest requests.
 */

#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Help.h"
#include "Processor.h"
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

/// @brief Prints the generated banner rows to stdout.
static void printBanner()
{
    for (const auto& [name, text] : map::banner)
        printf ("%s\n", text.toRawUTF8());
}

/**
 * @brief Prints @p message to stderr, prefixed with the project name and
 *        the diagnostic separator.
 *
 * @param message The diagnostic text to print.
 */
static void printError (const juce::String& message)
{
    fprintf (stderr, "%s\n", (ProjectInfo::projectName + Id::diagnosticSeparator + message).toRawUTF8());
}

static constexpr int flagArgIndex { 1 };///< argv's own index for the first argument, a flag or the manifest.
static constexpr int postFlagArgIndex { 2 };///< argv's own index for a flag authored after the manifest argument.
static constexpr int flagOnlyArgumentCount { 2 };///< argc's own value when argv carries one argument and nothing else.
static constexpr int postFlagArgumentCount { 3 };///< argc's own value when argv carries a manifest and a following flag.

/**
 * @brief The `--version` flag word.
 * @returns The `--version` flag text.
 */
static const juce::String& getVersionFlag()
{
    static const juce::String versionFlag { juce::String (Chars::doubleDash) + Id::version.toString() };
    return versionFlag;
}

/**
 * @brief The `--help` flag word.
 * @returns The `--help` flag text.
 */
static const juce::String& getHelpFlag()
{
    static const juce::String helpFlag { juce::String (Chars::doubleDash) + Id::help.toString() };
    return helpFlag;
}

/**
 * @brief Answers whether @p argv requests the @c --version flag, either
 *        alone or after the manifest argument.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @c true when @p argv requests @c --version.
 */
static bool isVersion (int argc, char* argv[])
{
    return (argc == flagOnlyArgumentCount and juce::String::fromUTF8 (argv[flagArgIndex]).compare (getVersionFlag()) == 0)
           or (argc == postFlagArgumentCount and juce::String::fromUTF8 (argv[postFlagArgIndex]).compare (getVersionFlag()) == 0);
}

/**
 * @brief Answers whether @p argv requests the @c --help flag, either
 *        alone or after the manifest argument.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @c true when @p argv requests @c --help.
 */
static bool isHelp (int argc, char* argv[])
{
    return (argc == flagOnlyArgumentCount and juce::String::fromUTF8 (argv[flagArgIndex]).compare (getHelpFlag()) == 0)
           or (argc == postFlagArgumentCount and juce::String::fromUTF8 (argv[postFlagArgIndex]).compare (getHelpFlag()) == 0);
}

/**
 * @brief Prints the project name, version string, and commit hash to
 *        stdout.
 */
static void writeVersion()
{
    const auto versionLine { ProjectInfo::projectName
                             + juce::String::charToString (Chars::space)
                             + ProjectInfo::versionString
                             + Chars::space
                             + Chars::openParen + CAST_COMMIT
                             + Chars::closeParen };
    printf ("%s\n", versionLine.toRawUTF8());
}

/**
 * @brief Whether stdout is a terminal -- the success line prints only then.
 * @returns @c true when stdout is a TTY.
 */
static bool isTerminalOutput() noexcept
{
#ifdef _WIN32
    return _isatty (_fileno (stdout)) != 0;
#else
    return isatty (fileno (stdout)) != 0;
#endif
}

/**
 * @brief Prints the done line to stdout when stdout is a terminal.
 *
 * The done line is `scout: Fine.`
 */
static void printDone()
{
    if (isTerminalOutput())
    {
        const auto doneLine { ProjectInfo::projectName + Id::diagnosticSeparator + text::Diagnostics::done };
        printf ("%s\n", doneLine.toRawUTF8());
    }
}

/**
 * @brief Prints the banner and help text to stdout.
 *
 * Opens its own JUCE GUI initialiser scope, because printHelp() renders
 * through the terminal graphics context.
 */
static void printBannerAndHelp()
{
    static const auto newlineText { juce::String::charToString (Chars::newline) };
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    printBanner();
    printf ("%s", newlineText.toRawUTF8());
    printHelp (BinaryData::getString (files::scoutHelp));
}

/**
 * @brief Runs Processor::generate() on @p documentFile and prints the
 *        resulting error to stderr on failure.
 *
 * Opens its own JUCE GUI initialiser scope. Prints the done line on
 * success.
 *
 * @param documentFile The manifest file to process.
 * @returns @c EXIT_SUCCESS when Processor::generate() succeeds, or
 *          @c EXIT_FAILURE after printing the failure's error message.
 */
static int runDocument (const juce::File& documentFile)
{
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;

    const auto result { Processor { documentFile }.generate() };

    if (result.wasOk())
    {
        printDone();
        return EXIT_SUCCESS;
    }

    printError (result.getErrorMessage());
    return EXIT_FAILURE;
}

/**
 * @brief Prints the banner and help text, then reports @p documentFile as
 *        not found on stderr.
 *
 * @param documentFile The manifest file that was not found.
 * @returns @c EXIT_FAILURE.
 */
static int runDocumentNotFound (const juce::File& documentFile)
{
    printBannerAndHelp();
    printError (text::Diagnostics::failNotFound + Id::diagnosticSeparator + documentFile.getFileName());
    return EXIT_FAILURE;
}

/**
 * @brief Resolves the manifest argument of @p argv against the current
 *        working directory, then follows a symbolic link to its target with
 *        juce::File::getLinkedTarget().
 *
 * A path that is not a link comes back unchanged on macOS and Linux.
 *
 * @pre @p argv holds the manifest argument at flagArgIndex.
 *
 * @param argv The CLI argument vector.
 * @returns The resolved manifest file.
 */
static juce::File getDocumentFile (char* argv[])
{
    return juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[flagArgIndex])).getLinkedTarget();
}

/**
 * @brief Dispatches @p argv's own version, help, or manifest request.
 *
 * One argument that names an existing file goes to runDocument(). One
 * argument that names no file goes to runDocumentNotFound(). More than one
 * argument prints `\<argument\>: unknown flag` for the argument at
 * postFlagArgIndex and returns @c EXIT_FAILURE. With no argument, the
 * function prints the banner and the guide and returns @c EXIT_SUCCESS,
 * the same as the help request.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @c EXIT_SUCCESS when the selected dispatch succeeds, or
 *          @c EXIT_FAILURE after printing its own failure.
 */
static int runCommandLine (int argc, char* argv[])
{
    if (isVersion (argc, argv))
    {
        writeVersion();
        return EXIT_SUCCESS;
    }

    if (isHelp (argc, argv))
    {
        printBannerAndHelp();
        return EXIT_SUCCESS;
    }

    if (argc == flagOnlyArgumentCount)
    {
        const auto documentFile { getDocumentFile (argv) };

        if (documentFile.existsAsFile())
            return runDocument (documentFile);

        return runDocumentNotFound (documentFile);
    }

    if (argc > flagOnlyArgumentCount)
    {
        printError (juce::String::fromUTF8 (argv[postFlagArgIndex]) + Id::diagnosticSeparator + text::Diagnostics::failFlagUnknown);
        return EXIT_FAILURE;
    }

    printBannerAndHelp();
    return EXIT_SUCCESS;
}

/**
 * @brief `scout`'s own entry point -- sets up UTF-8 console output on
 *        Windows, clears the terminal when stdout is a terminal, then
 *        dispatches @p argv to runCommandLine().
 *
 * @param argc The argument count.
 * @param argv The argument vector.
 * @returns runCommandLine()'s own result.
 */
int main (int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP (CP_UTF8);
#endif

    if (isTerminalOutput())
    {
        juce::MemoryOutputStream erase;
        jam::TerminalBuffer::writeEraseScreen (erase);
        fwrite (erase.getData(), sizeof (char), erase.getDataSize(), stdout);
        fflush (stdout);
    }

    return runCommandLine (argc, argv);
}

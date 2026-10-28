#include <JuceHeader.h>
#include "generated/Identifiers.h"
#include "ProfileReport.h"
#include "Benchmark.h"
#include "Help.h"

static juce::ValueTree getBaseline (const juce::File& directory, const juce::String& baselineName)
{
    if (baselineName.isNotEmpty())
    {
        const auto file { directory.getChildFile (baselineName) };
        return jam::ConfigDocument::parse (file.loadFileAsString(), file.getFullPathName()).getValueTree (Id::profile);
    }

    return juce::ValueTree {};
}

static int writeReport (const juce::File& directory, const juce::ValueTree& run, const juce::ValueTree& profiles)
{
    const auto outputFile { directory.getChildFile (run[Id::output].toString()) };
    const auto report { ProfileReport::getText (profiles) };

    if (outputFile.replaceWithText (report))
        return EXIT_SUCCESS;

    std::fputs (outputFile.getFullPathName().toRawUTF8(), stderr);
    return EXIT_FAILURE;
}

static int runEpochs (const juce::ValueTree& run, const juce::File& file)
{
    const auto directory { file.getParentDirectory() };
    const auto commandFile { directory.getChildFile (run[Id::command].toString()) };
    const auto samplesFile { directory.getChildFile (run[Id::samples].toString()) };
    const auto epochs { static_cast<int> (run[Id::epochs]) };

    jassert (epochs > 0);

    Benchmark benchmark { commandFile, samplesFile, file };

    for (int epoch { 0 }; epoch < epochs; ++epoch)
    {
        auto result { benchmark.runEpoch() };

        if (result.wasOk())
            result = benchmark.addEpoch();

        if (result.failed())
        {
            std::fputs (result.getErrorMessage().toRawUTF8(), stderr);
            return EXIT_FAILURE;
        }
    }

    return writeReport (directory, run, benchmark.getProfiles (getBaseline (directory, run[Id::baseline].toString())));
}

static int runManifest (const juce::File& file)
{
    const auto manifest { jam::ConfigDocument::parse (file.loadFileAsString(), file.getFullPathName()) };
    const auto result { jam::ConfigValidator {}.isValid (manifest) };

    if (result.wasOk())
    {
        return runEpochs (manifest.getValueTree (Id::manifest).getChildWithName (Id::toType (Id::run)), file);
    }

    std::fputs (result.getErrorMessage().toRawUTF8(), stderr);
    return EXIT_FAILURE;
}

int main (int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP (CP_UTF8);
#endif

    juce::ScopedJuceInitialiser_GUI libraryInitialiser;

    static constexpr int manifestArgumentCount { 2 };

    if (argc == manifestArgumentCount)
        return runManifest (juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[1])));

    static const juce::String helpFile { "HELP.md" };

    printHelp (helpFile);
    return EXIT_FAILURE;
}

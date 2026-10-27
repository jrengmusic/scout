#include <JuceHeader.h>
#include "generated/ProjectInfo.h"
#include "generated/Identifiers.h"
#include "TerminalBenchmark.h"
#include "Help.h"

static juce::Array<juce::UnitTest*> getTests (const jam::ConfigDocument& manifest)
{
    juce::Array<juce::UnitTest*> tests;

    for (const auto* row : manifest.getTableRows (Id::benchmark))
        tests.addArray (juce::UnitTest::getTestsWithName (manifest.getTableValue (*row, Id::name)));

    return tests;
}

static juce::ValueTree getBaseline (const juce::File& directory, const juce::String& baselineName)
{
    if (baselineName.isNotEmpty())
    {
        const auto file { directory.getChildFile (baselineName) };
        return jam::ConfigDocument::parse (file.loadFileAsString(), file.getFullPathName()).getValueTree (Id::profile);
    }

    return juce::ValueTree {};
}

static juce::ValueTree getProfiles (const juce::Array<juce::UnitTest*>& tests)
{
    juce::ValueTree profiles { Id::profile };

    for (auto* test : tests)
    {
        jassert (dynamic_cast<jam::Benchmark*> (test) != nullptr);

        if (auto* benchmark { dynamic_cast<jam::Benchmark*> (test) })
            profiles.appendChild (benchmark->getProfile(), nullptr);
    }

    return profiles;
}

static int runManifest (const juce::File& file)
{
    const auto manifest { jam::ConfigDocument::parse (file.loadFileAsString(), file.getFullPathName()) };
    const auto result { jam::ConfigValidator {}.isValid (manifest) };

    if (result.wasOk())
    {
        const auto config { manifest.getValueTree (Id::manifest) };
        const auto run { config.getChildWithName (Id::toType (Id::run)) };
        const auto tests { getTests (manifest) };

        for (auto* test : tests)
        {
            jassert (dynamic_cast<jam::Benchmark*> (test) != nullptr);

            if (auto* benchmark { dynamic_cast<jam::Benchmark*> (test) })
                benchmark->setEpochs (static_cast<int> (run[Id::epochs]));
        }

        const auto directory { file.getParentDirectory() };

        juce::UnitTestRunner runner;
        runner.runTests (tests, static_cast<juce::int64> (run[Id::seed]));

        const auto report { jam::ProfileReport::getText (getProfiles (tests), getBaseline (directory, run[Id::baseline].toString())) };
        const auto outputFile { directory.getChildFile (run[Id::output].toString()) };

        if (outputFile.replaceWithText (report))
        {
            int failures { 0 };

            for (int i { 0 }; i < runner.getNumResults(); ++i)
                failures += runner.getResult (i)->failures;

            return failures > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
        }

        std::fputs (outputFile.getFullPathName().toRawUTF8(), stderr);
        return EXIT_FAILURE;
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
    TerminalBenchmark terminalBenchmark;

    static constexpr int manifestArgumentCount { 2 };

    if (argc == manifestArgumentCount)
        return runManifest (juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[1])));

    static const juce::String helpFile { "HELP.md" };

    printHelp (helpFile);
    return EXIT_FAILURE;
}

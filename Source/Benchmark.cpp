#include "Benchmark.h"
#include "generated/Generated.h"
#include "ProfileReport.h"

Benchmark::Benchmark (const juce::File& newCommandFile, const juce::File& newSamplesFile, const juce::File& newManifestFile)
    : commandFile { newCommandFile },
      samplesFile { newSamplesFile },
      manifestFile { newManifestFile }
{
    for (const auto& rowKey : { Id::epoch, Id::user, Id::system })
    {
        calls[rowKey] = jam::Profiler::callsPerEpoch;
        units[rowKey] = jam::Strings::fromView (jam::Profiler::nanosecondUnit);
    }
}

#if JUCE_MAC or JUCE_LINUX
void Benchmark::addChildTime (const rusage& before, const rusage& after)
{
    static constexpr double nanosecondsPerMicrosecond { 1.0e3 };

    const auto getNanoseconds { [] (const timeval& time)
    {
        return static_cast<double> (time.tv_sec) * jam::Profiler::nanosecondsPerSecond
             + static_cast<double> (time.tv_usec) * nanosecondsPerMicrosecond;
    } };

    epochSamples[Id::user].add (juce::var (getNanoseconds (after.ru_utime) - getNanoseconds (before.ru_utime)));
    epochSamples[Id::system].add (juce::var (getNanoseconds (after.ru_stime) - getNanoseconds (before.ru_stime)));
}
#endif

int Benchmark::runCommand()
{
    juce::WaitableEvent finished;
    int exitCode { EXIT_FAILURE };

    jam::Subprocess subprocess;
    subprocess.launch (juce::StringArray { commandFile.getFullPathName(), manifestFile.getFullPathName() },
                       manifestFile.getParentDirectory(),
                       [&finished, &exitCode] (int processExitCode, const std::string&)
                       {
                           exitCode = processExitCode;
                           finished.signal();
                       });

    finished.wait();

    return exitCode;
}

juce::Result Benchmark::runEpoch()
{
    [[maybe_unused]] const auto isDeleted { samplesFile.deleteFile() };

    jassert (isDeleted);

#if JUCE_MAC or JUCE_LINUX
    rusage before {};
    getrusage (RUSAGE_CHILDREN, &before);
#endif

    const auto begin { juce::Time::getHighResolutionTicks() };
    const auto exitCode { runCommand() };
    const auto end { juce::Time::getHighResolutionTicks() };

    epochSamples[Id::epoch].add (juce::var (jam::Profiler::getNanoseconds (end - begin)));

#if JUCE_MAC or JUCE_LINUX
    rusage after {};
    getrusage (RUSAGE_CHILDREN, &after);
    addChildTime (before, after);
#endif

    if (exitCode == EXIT_SUCCESS) return juce::Result::ok();
    return juce::Result::fail (commandFile.getFullPathName() + Id::diagnosticSeparator + text::Diagnostics::failCommand);
}

void Benchmark::addBins (const juce::Identifier& rowKey, const juce::String& binsText)
{
    auto view { jam::Strings::getView (binsText) };
    const auto result { jam::ConfigDocument::getValueTypes().get (Id::numbers, view) };
    const auto& binCounts { *result.getArray() };

    jassert (binCounts.size() == jam::Profiler::numBins);

    auto [binEntry, isCreated] = bins.try_emplace (rowKey);
    auto& [binKey, zoneBins] = *binEntry;

    if (isCreated)
        zoneBins.resize (jam::Profiler::numBins);

    for (int bin { 0 }; bin < jam::Profiler::numBins; ++bin)
        zoneBins.at (bin) += static_cast<juce::int64> (binCounts.getReference (bin));
}

void Benchmark::addRows (const juce::ValueTree& table)
{
    const auto epochCount { epochSamples[Id::epoch].size() };

    for (const auto& row : table)
    {
        const auto rowKey { row.getType() };
        const auto binsText { row[Id::bins].toString() };

        auto [sampleEntry, isCreated] = epochSamples.try_emplace (rowKey);
        auto& [sampleKey, rowSamples] = *sampleEntry;

        if (isCreated)
            rowSamples.insertMultiple (0, juce::var (0), epochCount - 1);

        rowSamples.add (row[Id::value]);
        calls[rowKey] = static_cast<int> (row[Id::calls]);
        units[rowKey] = row[Id::unit].toString();

        if (binsText.isNotEmpty())
            addBins (rowKey, binsText);
    }
}

juce::Result Benchmark::addEpoch()
{
    if (samplesFile.existsAsFile())
    {
        const auto samplesTree { jam::ConfigDocument::parse (samplesFile.loadFileAsString(), samplesFile.getFullPathName()).getValueTree (Id::samples) };

        for (const auto& table : samplesTree)
        {
            if (table.getType() == Id::toType (Id::run))
            {
                runTable = table;
            }
            else
            {
                tableType = table.getType();
                addRows (table);
            }
        }

        const auto epochCount { epochSamples[Id::epoch].size() };

        for (auto& [rowKey, rowSamples] : epochSamples)
            if (rowSamples.size() < epochCount)
                rowSamples.add (juce::var (0));

        return juce::Result::ok();
    }

    return juce::Result::fail (text::Diagnostics::failNotFound + Id::diagnosticSeparator + samplesFile.getFullPathName());
}

juce::ValueTree Benchmark::getProfiles (const juce::ValueTree& baseline) const
{
    const juce::Identifier callTableType { tableType.toString()
                                         + juce::String::charToString (Chars::underscore)
                                         + Id::toType (Id::calls).toString() };

    juce::ValueTree profiles { Id::profile };
    profiles.appendChild (ProfileReport::getProfile (tableType, epochSamples, calls, units, baseline.getChildWithName (tableType)), nullptr);
    profiles.appendChild (ProfileReport::getProfile (callTableType, bins, units, baseline.getChildWithName (callTableType)), nullptr);
    profiles.appendChild (runTable, nullptr);

    return profiles;
}

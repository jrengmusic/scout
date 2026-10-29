#include "Benchmark.h"
#include "Sampler.h"
#include "generated/Generated.h"
#include "ProfileReport.h"

Benchmark::Benchmark (const juce::File& newCommandFile, const juce::File& newSamplesFile, const juce::File& newManifestFile, const juce::ValueTree& newSampleTable)
    : commandFile { newCommandFile },
      samplesFile { newSamplesFile },
      manifestFile { newManifestFile },
#if JUCE_WINDOWS
      job { CreateJobObjectW (nullptr, nullptr), CloseHandle },
#endif
      sampleTable { newSampleTable }
{
#if JUCE_WINDOWS
    jassert (job != nullptr);

    [[maybe_unused]] const auto isAssigned { AssignProcessToJobObject (job.get(), GetCurrentProcess()) };

    jassert (isAssigned);
#endif
}

void Benchmark::addRow (const juce::Identifier& rowKey, double nanoseconds)
{
    epochSamples[tableType][rowKey].add (juce::var (nanoseconds));
    calls[tableType][rowKey] = jam::Profiler::callsPerEpoch;
    units[tableType][rowKey] = jam::Strings::fromView (jam::Profiler::nanosecondUnit);
}

#if JUCE_MAC or JUCE_LINUX
rusage Benchmark::getChildUsage() const
{
    rusage usage {};
    getrusage (RUSAGE_CHILDREN, &usage);

    return usage;
}

void Benchmark::addChildTime (const rusage& before, const rusage& after)
{
    static constexpr double nanosecondsPerMicrosecond { 1.0e3 };

    const auto getNanoseconds { [] (const timeval& time)
    {
        return static_cast<double> (time.tv_sec) * jam::Profiler::nanosecondsPerSecond
             + static_cast<double> (time.tv_usec) * nanosecondsPerMicrosecond;
    } };

    addRow (Id::user, getNanoseconds (after.ru_utime) - getNanoseconds (before.ru_utime));
    addRow (Id::system, getNanoseconds (after.ru_stime) - getNanoseconds (before.ru_stime));
}
#elif JUCE_WINDOWS
JOBOBJECT_BASIC_ACCOUNTING_INFORMATION Benchmark::getChildUsage() const
{
    const auto getTicks { [] (const FILETIME& time)
    {
        return static_cast<LONGLONG> (ULARGE_INTEGER { { time.dwLowDateTime, time.dwHighDateTime } }.QuadPart);
    } };

    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION usage {};
    FILETIME creationTime {};
    FILETIME exitTime {};
    FILETIME kernelTime {};
    FILETIME userTime {};

    [[maybe_unused]] const auto isQueried { QueryInformationJobObject (job.get(), JobObjectBasicAccountingInformation, &usage, sizeof (usage), nullptr) };
    [[maybe_unused]] const auto hasTimes { GetProcessTimes (GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime) };

    jassert (isQueried and hasTimes);

    usage.TotalUserTime.QuadPart -= getTicks (userTime);
    usage.TotalKernelTime.QuadPart -= getTicks (kernelTime);

    return usage;
}

void Benchmark::addChildTime (const JOBOBJECT_BASIC_ACCOUNTING_INFORMATION& before, const JOBOBJECT_BASIC_ACCOUNTING_INFORMATION& after)
{
    static constexpr double nanosecondsPerTick { 100.0 };

    addRow (Id::user, static_cast<double> (after.TotalUserTime.QuadPart - before.TotalUserTime.QuadPart) * nanosecondsPerTick);
    addRow (Id::system, static_cast<double> (after.TotalKernelTime.QuadPart - before.TotalKernelTime.QuadPart) * nanosecondsPerTick);
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

    const auto before { getChildUsage() };
    const auto sampler { sampleTable.isValid() ? std::make_unique<Sampler> (sampleTable) : nullptr };
    const auto begin { juce::Time::getHighResolutionTicks() };
    const auto exitCode { runCommand() };
    const auto end { juce::Time::getHighResolutionTicks() };
    const auto after { getChildUsage() };

    auto result { exitCode == EXIT_SUCCESS ? juce::Result::ok() : juce::Result::fail (commandFile.getFullPathName() + Id::diagnosticSeparator + text::Diagnostics::failCommand) };
    juce::ValueTree samplerSamples;

    if (result.wasOk() and sampler != nullptr)
    {
        result = sampler->drain();
        samplerSamples = sampler->getSamples();
    }

    if (result.wasOk())
        result = addEpoch (jam::Profiler::getNanoseconds (end - begin), samplerSamples);

    if (result.wasOk())
    {
        addChildTime (before, after);
        addMissingSamples();
    }

    return result;
}

void Benchmark::addBins (const juce::Identifier& benchmarkType, const juce::Identifier& rowKey, const juce::String& binsText)
{
    auto view { jam::Strings::getView (binsText) };
    const auto result { jam::ConfigDocument::getValueTypes().get (Id::numbers, view) };
    const auto& binCounts { *result.getArray() };

    jassert (binCounts.size() == jam::Profiler::numBins);

    auto [binEntry, isCreated] = bins[benchmarkType].try_emplace (rowKey);
    auto& [binKey, zoneBins] = *binEntry;

    if (isCreated)
        zoneBins.resize (jam::Profiler::numBins);

    for (int bin { 0 }; bin < jam::Profiler::numBins; ++bin)
        zoneBins.at (bin) += static_cast<juce::int64> (binCounts.getReference (bin));
}

void Benchmark::addRows (const juce::Identifier& benchmarkType, const juce::ValueTree& table)
{
    const auto epochCount { epochSamples[tableType][Id::epoch].size() };
    auto& benchmarkSamples { epochSamples[benchmarkType] };
    auto& benchmarkCalls { calls[benchmarkType] };
    auto& benchmarkUnits { units[benchmarkType] };

    for (const auto& row : table)
    {
        const auto rowKey { row.getType() };
        const auto binsText { row[Id::bins].toString() };

        auto [sampleEntry, isCreated] = benchmarkSamples.try_emplace (rowKey);
        auto& [sampleKey, rowSamples] = *sampleEntry;

        if (isCreated)
            rowSamples.insertMultiple (0, juce::var (0), epochCount);

        rowSamples.add (row[Id::value]);
        benchmarkCalls[rowKey] = static_cast<int> (row[Id::calls]);
        benchmarkUnits[rowKey] = row[Id::unit].toString();

        if (binsText.isNotEmpty())
            addBins (benchmarkType, rowKey, binsText);
    }
}

void Benchmark::addMissingSamples()
{
    const auto epochCount { epochSamples[tableType][Id::epoch].size() };

    for (auto& [benchmarkType, benchmarkSamples] : epochSamples)
        for (auto& [rowKey, rowSamples] : benchmarkSamples)
            if (rowSamples.size() < epochCount)
                rowSamples.add (juce::var (0));
}

juce::Result Benchmark::addEpoch (double epochNanoseconds, const juce::ValueTree& samplerSamples)
{
    if (samplesFile.existsAsFile())
    {
        const auto samplesTree { jam::ConfigDocument::parse (samplesFile.loadFileAsString(), samplesFile.getFullPathName()).getValueTree (Id::samples) };

        for (const auto& table : samplesTree)
        {
            if (table.getType() == Id::toType (Id::run))
                runTable = table;
            else
                tableType = table.getType();
        }

        for (const auto& table : samplesTree)
            if (table.getType() != Id::toType (Id::run))
                addRows (tableType, table);

        for (const auto& table : samplerSamples)
            addRows (table.getType(), table);

        addRow (Id::epoch, epochNanoseconds);

        return juce::Result::ok();
    }

    return juce::Result::fail (text::Diagnostics::failNotFound + Id::diagnosticSeparator + samplesFile.getFullPathName());
}

juce::ValueTree Benchmark::getProfiles (const juce::ValueTree& baseline) const
{
    juce::ValueTree profiles { Id::profile };

    for (const auto& [benchmarkType, benchmarkSamples] : epochSamples)
        profiles.appendChild (ProfileReport::getProfile (benchmarkType, benchmarkSamples, calls.at (benchmarkType), units.at (benchmarkType), baseline.getChildWithName (benchmarkType)), nullptr);

    for (const auto& [benchmarkType, benchmarkBins] : bins)
    {
        const juce::Identifier callTableType { benchmarkType.toString()
                                             + juce::String::charToString (Chars::underscore)
                                             + Id::toType (Id::calls).toString() };

        profiles.appendChild (ProfileReport::getProfile (callTableType, benchmarkBins, units.at (benchmarkType), baseline.getChildWithName (callTableType)), nullptr);
    }

    profiles.appendChild (runTable, nullptr);

    return profiles;
}

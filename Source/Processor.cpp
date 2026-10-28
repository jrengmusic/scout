#include "Processor.h"

static juce::ValueTree getBaseline (const juce::File& baselineFile)
{
    if (baselineFile.existsAsFile())
        return jam::ConfigDocument::parse (baselineFile.loadFileAsString(), baselineFile.getFullPathName()).getValueTree (Id::profile);

    return juce::ValueTree {};
}

Processor::Processor (const juce::File& newDocumentFile)
    : documentFile { newDocumentFile },
      manifest { jam::ConfigDocument::parse (newDocumentFile.loadFileAsString(), newDocumentFile.getFullPathName()) }
{
}

juce::Result Processor::generate()
{
    auto result { Validator {}.isValid (manifest) };

    if (result.wasOk())
        result = run();

    if (result.wasOk())
    {
        const auto runTable { manifest.getValueTree (Id::manifest).getChildWithName (Id::toType (Id::run)) };

        if (runTable.isValid())
            result = runEpochs (runTable);
    }

    return result;
}

juce::Result Processor::run()
{
    for (auto* table : manifest.getTables (Id::toolchain))
        for (auto* row : manifest.getTableRows (*table))
            if (const auto result { runToolchainRow (*row) }; not result.wasOk())
                return result;

    return juce::Result::ok();
}

juce::Result Processor::runToolchainRow (const jam::ConfigDocument::Element& row)
{
    const auto command { manifest.getTableValue (row, Id::command) };
    const auto flag { manifest.getTableValue (row, Id::flag) };
    const auto arguments { Toolchain::getToolchainArguments (command, flag) };
    const auto diagnosticLine { flag.isNotEmpty()
                                    ? command + Chars::space + flag
                                    : command };

    return Toolchain::runProcess (arguments, diagnosticLine);
}

juce::Result Processor::runEpochs (const juce::ValueTree& runTable)
{
    const auto directory { documentFile.getParentDirectory() };
    const auto baselineName { runTable[Id::baseline].toString() };
    const auto baselineFile { directory.getChildFile (baselineName) };

    if (baselineName.isEmpty() or baselineFile.existsAsFile())
    {
        const auto commandFile { directory.getChildFile (runTable[Id::command].toString()) };
        const auto samplesFile { directory.getChildFile (runTable[Id::samples].toString()) };
        const auto epochs { static_cast<int> (runTable[Id::epochs]) };

        jassert (epochs > 0);

        Benchmark benchmark { commandFile, samplesFile, documentFile };

        for (int epoch { 0 }; epoch < epochs; ++epoch)
        {
            auto result { benchmark.runEpoch() };

            if (result.wasOk())
                result = benchmark.addEpoch();

            if (result.failed())
                return result;
        }

        return writeReport (runTable, benchmark.getProfiles (getBaseline (baselineFile)));
    }

    return juce::Result::fail (text::Diagnostics::failNotFound + Id::diagnosticSeparator + baselineFile.getFullPathName());
}

juce::Result Processor::writeReport (const juce::ValueTree& runTable, const juce::ValueTree& profiles)
{
    const auto outputFile { documentFile.getParentDirectory().getChildFile (runTable[Id::output].toString()) };

    if (ProfileReport::toFile (profiles, outputFile))
        return juce::Result::ok();

    return juce::Result::fail (outputFile.getFullPathName() + Id::diagnosticSeparator + text::Diagnostics::failOutputWrite);
}

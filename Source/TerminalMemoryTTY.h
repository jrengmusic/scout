#pragma once
#include <JuceHeader.h>

class TerminalMemoryTTY : public jam::TerminalTTY
{
public:
    explicit TerminalMemoryTTY (const juce::MemoryBlock& recording);

    bool open (jam::AsyncUpdater::Context context, int maximumBlockSize, jam::Cell::Rectangle dims, const juce::String& shell,
               const juce::String& args, const juce::String& workingDirectory,
               const juce::StringPairArray& environment) override;
    void close() override;
    int read (char* buf, int maxBytes) override;
    int write (const char* buf, int len) override;
    void setWinsize (jam::TerminalWinsize ws) override;

private:
    juce::MemoryInputStream stream;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TerminalMemoryTTY)
};

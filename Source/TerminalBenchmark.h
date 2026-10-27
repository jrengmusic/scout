#pragma once
#include <JuceHeader.h>
#include "TerminalMemoryTTY.h"

class TerminalBenchmark : public jam::Benchmark,
                          private jam::AsyncUpdater
{
public:
    TerminalBenchmark();

private:
    static constexpr int blockSize { 65536 };
    static constexpr int fixtureLineCount { 1000000 };
    static constexpr int historyCapacity { 10000 };
    static constexpr int recordTimeoutMs { 60000 };
    static constexpr bool shouldUseInlineImages { true };
    static constexpr jam::TerminalWinsize winsize { 80, 24, 0, 0 };

    void initialise() override;
    void shutdown() override;
    void runEpoch() override;
    juce::MemoryBlock getOutput() const override;
    void handleAsyncUpdate() override;

    std::unique_ptr<jam::TerminalTTY> tty;
    juce::HeapBlock<char> chunk;
    juce::MemoryBlock fixture;
    juce::WaitableEvent recorded;
    jam::Function::Map<juce::Identifier, void> events;
    std::unique_ptr<jam::TerminalBuffer> terminalBuffer;
    std::unique_ptr<jam::TerminalSwap> swap;
    std::unique_ptr<jam::Colours> trueColours;
    std::unique_ptr<jam::TerminalProcessor> terminalProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TerminalBenchmark)
};

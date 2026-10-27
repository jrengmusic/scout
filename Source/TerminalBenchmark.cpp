#include "TerminalBenchmark.h"

TerminalBenchmark::TerminalBenchmark()
    : jam::Benchmark ("Terminal", "terminal")
{
    for (const auto& id : { Id::applicationCursor, Id::bracketedPaste, Id::mouseSgr, Id::cursorVisible, Id::alternateScreen, Id::isRecording })
        events.add<bool> (id, [] (bool) {});

    for (const auto& id : { Id::mouseTracking, Id::cursor })
        events.add<int> (id, [] (int) {});
}

void TerminalBenchmark::initialise()
{
    beginTest (getName());

#if JUCE_WINDOWS
    tty = std::make_unique<jam::TerminalWindowsTTY> (juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory());
#else
    tty = std::make_unique<jam::TerminalUnixTTY>();
#endif

    chunk.malloc (blockSize);
    tty->onBytesReady = [this] { triggerAsyncUpdate(); };

    const auto isOpen { tty->open (getContext(), blockSize, winsize.toCellRect(), "seq", "1 " + juce::String (fixtureLineCount)) };

    expect (isOpen);
    expect (recorded.wait (recordTimeoutMs));
}

void TerminalBenchmark::handleAsyncUpdate()
{
    auto bytesRead { tty->read (chunk.get(), blockSize) };

    while (bytesRead > 0)
    {
        fixture.append (chunk.get(), static_cast<size_t> (bytesRead));
        bytesRead = tty->read (chunk.get(), blockSize);
    }

    if (bytesRead < 0)
        recorded.signal();
}

void TerminalBenchmark::shutdown()
{
    tty->close();
}

void TerminalBenchmark::runEpoch()
{
    terminalBuffer = std::make_unique<jam::TerminalBuffer> (shouldUseInlineImages);
    terminalBuffer->prepare (winsize.cols, winsize.rows, blockSize);
    swap = std::make_unique<jam::TerminalSwap>();
    swap->prepare (winsize);
    swap->setCapacity (historyCapacity);
    trueColours = std::make_unique<jam::Colours> (map::ColourId::terminalColourId);
    terminalProcessor = std::make_unique<jam::TerminalProcessor>();
    terminalProcessor->prepare (blockSize);
    terminalProcessor->reset (*terminalBuffer);

    TerminalMemoryTTY replay { fixture };

    while (terminalProcessor->process (replay, *terminalBuffer, *swap, *trueColours, events))
        swap->writeFromTerminalBuffer (*terminalBuffer);

    swap->writeFromTerminalBuffer (*terminalBuffer);
}

juce::MemoryBlock TerminalBenchmark::getOutput() const
{
    const auto lines { swap->getLines() };
    juce::MemoryOutputStream output;

    output.writeInt64 (lines.getStart());
    output.writeInt64 (lines.getEnd());

    for (int row { 0 }; row < winsize.rows; ++row)
        jam::TerminalBuffer::writeRow (output, std::as_const (*terminalBuffer).getRow (jam::Cell { row }), terminalBuffer->getImagePlacements (jam::Cell { row }));

    return output.getMemoryBlock();
}

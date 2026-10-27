#include "TerminalMemoryTTY.h"

TerminalMemoryTTY::TerminalMemoryTTY (const juce::MemoryBlock& recording)
    : stream (recording, false)
{
}

bool TerminalMemoryTTY::open (jam::AsyncUpdater::Context, int, jam::Cell::Rectangle, const juce::String&,
                              const juce::String&, const juce::String&,
                              const juce::StringPairArray&)
{
    return true;
}

void TerminalMemoryTTY::close()
{
}

int TerminalMemoryTTY::read (char* buf, int maxBytes)
{
    return stream.read (buf, maxBytes);
}

int TerminalMemoryTTY::write (const char*, int len)
{
    return len;
}

void TerminalMemoryTTY::setWinsize (jam::TerminalWinsize)
{
}

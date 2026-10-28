#include "Help.h"

void printHelp (const juce::String& helpText)
{
    jam::SharedInstance<jam::SharedDocuments> documents { std::in_place };
    jam::Typeface typeface;

    juce::MemoryOutputStream frame;
    jam::TerminalBuffer buffer;
    const jam::GlyphArrangement glyphArrangement { jam::TerminalBuffer::getCellLookup(), jam::TerminalBuffer::getCellMeasure() };
    jam::StyleManager styleManager { { BinaryData::fetcher }, jam::Strings { files::markdownStyleSheet, files::mermaidStyleSheet } };
    jam::StyleMarkdown styleMarkdown;

    const auto columnWidth { styleMarkdown.getColumnWidth() };

    jam::MarkdownComponent component;
    component.setLookAndFeel (&styleMarkdown);
    component.setBounds (0, 0, columnWidth, 0);
    component.setDocument (jam::MarkdownDocument::parse (helpText, files::scoutHelp));

    const auto rows { component.getViewedComponent()->getHeight() };

    {
        buffer.prepare (columnWidth, rows, rows);

        jam::TerminalGraphicsContext context { buffer, *jam::Colours::getInstance() };
        juce::Graphics g { context };
        component.getViewedComponent()->paintEntireComponent (g, false);
    }

    buffer.process();
    buffer.writeToStream (frame, styleMarkdown);

    fwrite (frame.getData(), sizeof (char), frame.getDataSize(), stdout);
}

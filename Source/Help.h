#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @brief Renders the HELP.md text to stdout through the terminal graphics
 *        context.
 *
 * The function loads the `files::styleSheet` style sheet into the style manager and parses
 * @p helpText as markdown. The width is the value of `jam::StyleMarkdown::getColumnWidth()`, which is the
 * `maxLineWidth` metric plus the `margin` metric on each side, rounded up. The component
 * is laid out at that width. The row count is the height of the viewed component.
 * The function prepares a terminal buffer for that grid: the width is the column count, and the row count is the transit room.
 * It paints the viewed component through a terminal graphics context. The context takes the `jam::Colours` instance.
 * `buffer.writeToStream (frame, styleMarkdown)` then writes the frame as VT bytes to a memory stream.
 * The function writes the stream to stdout.
 *
 * @param helpText The HELP.md text, parsed as markdown.
 */
void printHelp (const juce::String& helpText);

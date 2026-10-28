#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @brief Renders the HELP.md text to stdout through the terminal graphics
 *        context.
 *
 * The function loads the markdown and mermaid stylesheets into one style manager and parses
 * @p helpText as markdown. The width, in cells, is the column width of the markdown
 * style: the `maxLineWidth` metric plus the `margin` metric on both sides, rounded up. The component
 * is laid out once at that width. The row count is the height of the viewed component.
 * The function prepares a terminal buffer for that grid, with the row count as transit room.
 * It paints the viewed component through a terminal graphics context. The context takes the `Colours` of the style manager.
 * `buffer.writeToStream (frame, styleMarkdown)` then writes the frame as VT bytes to a memory stream.
 * The function writes the stream to stdout.
 *
 * @pre The JUCE GUI initialiser scope is open.
 *
 * @param helpText The HELP.md text, parsed as markdown.
 */
void printHelp (const juce::String& helpText);

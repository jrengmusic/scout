/*******************************************************************************
                        Codegen Annotated Source of Truth
————————————————————————————————————————————————————————————————————————————————

            ░░████████████░░████████████░░████████████░░████████████
            ░░████  ░░████░░████  ░░████░░████  ░░████    ░░████
            ░░████        ░░████  ░░████░░████            ░░████
            ░░████        ░░████████████░░████████████    ░░████
            ░░████        ░░████  ░░████        ░░████    ░░████
            ░░████  ░░████░░████  ░░████░░████  ░░████    ░░████
            ░░████████████░░████  ░░████░░████████████    ░░████

————————————————————————————————————————————————————————————————————————————————
                         FOR YOUR EYES ONLY, DO NOT EDIT
********************************************************************************/

/**
 * @file Identifiers.h
 * @brief SCOUT manifest vocabulary.
 */

#pragma once

namespace Id
{
/*_____________________________________________________________________________*/

/** @brief SCOUT manifest vocabulary — the tables and keys of a scout manifest. */

extern const juce::Identifier epochs;
extern const juce::Identifier output;
extern const juce::Identifier command;
extern const juce::Identifier profile;
extern const juce::Identifier epoch;
extern const juce::Identifier errorPercent;
extern const juce::Identifier change;
extern const juce::Identifier probability;
extern const juce::Identifier percentile;
extern const juce::Identifier system;
extern const juce::Identifier toolchain;   ///< Reserved toolchain manifest table.
extern const juce::Identifier flag;        ///< Toolchain table flag column.
extern const juce::Identifier help;

/**______________________________END OF NAMESPACE______________________________*/
}// namespace Id

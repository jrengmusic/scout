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

#include <JuceHeader.h>
#include "Identifiers.h"

namespace Id
{
/*_____________________________________________________________________________*/

/** @brief SCOUT manifest vocabulary — the tables and keys of a scout manifest. */

    const juce::Identifier manifest  { juce::String::fromUTF8 ("manifest") };
    const juce::Identifier run       { juce::String::fromUTF8 ("run") };
    const juce::Identifier benchmark { juce::String::fromUTF8 ("benchmark") };
    const juce::Identifier epochs    { juce::String::fromUTF8 ("epochs") };
    const juce::Identifier seed      { juce::String::fromUTF8 ("seed") };
    const juce::Identifier output    { juce::String::fromUTF8 ("output") };

/**______________________________END OF NAMESPACE______________________________*/
}// namespace Id

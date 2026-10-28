/*
    IMPORTANT! This file is auto-generated.
    If you alter its contents, your changes may be overwritten!

    This is the header file that your files should include in order to get all the
    JUCE library headers. You should avoid including the JUCE headers directly in
    your own source files, because that wouldn't pick up the correct configuration
    options for your app.

*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <jam_core/jam_core.h>
#include <jam_document/jam_document.h>
#include <jam_markdown/jam_markdown.h>
#include <jam_style/jam_style.h>
#include <jam_graphics/jam_graphics.h>
#include <jam_freetype/jam_freetype.h>
#include <jam_data_structures/jam_data_structures.h>
#include <jam_web/jam_web.h>
#include <jam_animation/jam_animation.h>
#include <jam_gui/jam_gui.h>
#include <jam_terminal/jam_terminal.h>
#include <juce_animation/juce_animation.h>
#include <jam_mermaid_diagram/jam_mermaid_diagram.h>
#include <jam_markdown_graphics/jam_markdown_graphics.h>
#include <jam_profiler/jam_profiler.h>
#include <jam_subprocess/jam_subprocess.h>


#if JUCE_TARGET_HAS_BINARY_DATA
 #include "BinaryData.h"
#endif

#if ! DONT_SET_USING_JUCE_NAMESPACE
 // If your code uses a lot of JUCE classes, then this will obviously save you
 // a lot of typing, but can be disabled by setting DONT_SET_USING_JUCE_NAMESPACE.
 using namespace juce;
#endif

#if ! JUCE_DONT_DECLARE_PROJECTINFO
namespace ProjectInfo
{
    const char* const  projectName    = "scout";
    const char* const  companyName    = "JRENG";
    const char* const  versionString  = "0.1.0";
    const int          versionNumber  =  0x100;
}
#endif

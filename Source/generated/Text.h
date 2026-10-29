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
 * @file Text.h
 * @brief scout message strings — one constant per failure, one for success.
 */

#pragma once

namespace text
{
/*_____________________________________________________________________________*/

/** @brief scout message texts, emitted verbatim when a run fails or completes. */
struct Diagnostics
{
    static constexpr const char* const failNotFound        { "not found"                     };///< Referenced table, column, symbol, or file does not exist.
    static constexpr const char* const failToolchain       { "toolchain command failed"      };///< A toolchain row's process could not start or exited nonzero.
    static constexpr const char* const failToolchainColumn { "toolchain column not declared" };///< A ## toolchain table's header row declares no command or flag column.
    static constexpr const char* const failSampleRange     { "sample value out of range"     };///< A ## sample table's intervalMs or stackDepth is outside its bounds.
    static constexpr const char* const failOutputWrite     { "cannot write output"           };///< An output file could not be written.
    static constexpr const char* const failCommand         { "command failed"                };///< The run command exited nonzero.
    static constexpr const char* const failFlagUnknown     { "unknown flag"                  };///< An argument after the manifest that is not --help or --version.
    static constexpr const char* const done                { "Fine."                         };///< Every step of the run succeeded.
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace text

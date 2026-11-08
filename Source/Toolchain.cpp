#include "Toolchain.h"

juce::StringArray Toolchain::getToolchainArguments (const juce::String& command, const juce::String& flag)
{
    juce::StringArray arguments { command };
    arguments.addTokens (flag, true);
    arguments.removeEmptyStrings();

    for (auto& argument : arguments)
        argument = argument.removeCharacters (juce::String::charToString (Chars::doubleQuote));

    return arguments;
}

juce::Result Toolchain::runProcess (const juce::StringArray& arguments, const juce::File& workingDirectory, const juce::String& diagnosticLine)
{
    juce::WaitableEvent finished;
    int exitCode { -1 };

    jam::Subprocess subprocess;
    subprocess.launch (arguments,
                       workingDirectory,
                       [&finished, &exitCode] (int processExitCode, const std::string&)
                       {
                           exitCode = processExitCode;
                           finished.signal();
                       },
                       [] (std::string_view chunk, bool)
                       {
                           fwrite (chunk.data(), 1, chunk.size(), stdout);
                           fflush (stdout);
                       });

    finished.wait();

    if (exitCode != 0)
        return juce::Result::fail (
            diagnosticLine + Id::diagnosticSeparator + text::Diagnostics::failToolchain);

    return juce::Result::ok();
}

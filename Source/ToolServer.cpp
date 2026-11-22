#include "ToolServer.h"

ToolServer::Run::Run (jam::Model& newModel, const juce::String& newRequestId)
    : numMessagesAttachment { numMessages, [this] (const juce::var& value) { row.setProperty (Id::numMessages, value, nullptr); } },
      isOpenAttachment { isOpen, [this] (const juce::var& value)
                         {
                             row.setProperty (Id::exitCode, exitCode.getValue(), nullptr);
                             row.setProperty (Id::isOpen, value, nullptr);
                         } }
{
    row.setProperty (Id::id, newRequestId, nullptr);
    row.setProperty (Id::output, juce::MemoryBlock {}, nullptr);
    row.setProperty (Id::numMessages, 0, nullptr);
    row.setProperty (Id::exitCode, 0, nullptr);
    row.setProperty (Id::isOpen, 1, nullptr);
    newModel.state.appendChild (row, nullptr);

    client.onBytesReady = [this] (int numBytes)
    {
        if (numBytes > 0)
            numMessages.setValue (numMessages.getValue() + 1);
    };

    client.onExit = [this] (int code)
    {
        exitCode.store (code);
        isOpen.setValue (0);
    };
}

ToolServer::ToolServer()
{
    server.addTool (jam::Strings::getView (Id::run.toString()),
                    [this] (const juce::Identifier&, const juce::String& requestId, const jam::JsonDocument& document, const jam::Document::Element& arguments)
                    {
                        runTool (requestId, document, arguments);
                    });

    server.onCancelled = [this] (const juce::String& requestId)
    {
        if (runs.contains (requestId))
        {
            model.state.removeChild (runs.at (requestId)->row, nullptr);
            runs.erase (requestId);
        }
    };

    server.onInputClosed = [this]
    {
        model.state.removeAllChildren (nullptr);
        runs.clear();
        quit();
    };

    model.state.addListener (this);
}

ToolServer::~ToolServer()
{
    model.state.removeListener (this);
}

const juce::String ToolServer::getApplicationName()
{
    return ProjectInfo::projectName;
}

const juce::String ToolServer::getApplicationVersion()
{
    return ProjectInfo::versionString;
}

bool ToolServer::moreThanOneInstanceAllowed()
{
    return true;
}

void ToolServer::initialise (const juce::String&)
{
    server.start();
}

void ToolServer::shutdown()
{
}

void ToolServer::anotherInstanceStarted (const juce::String&)
{
}

void ToolServer::systemRequestedQuit()
{
    quit();
}

void ToolServer::suspended()
{
}

void ToolServer::resumed()
{
}

void ToolServer::unhandledException (const std::exception*, const juce::String&, int)
{
}

void ToolServer::runTool (const juce::String& requestId, const jam::JsonDocument& document, const jam::Document::Element& arguments)
{
    const auto manifestPath { jam::Strings::fromView (document.getString (document.get (arguments, jam::Strings::getView (Id::manifest.toString())))) };

    if (juce::File::isAbsolutePath (manifestPath))
    {
        const juce::File manifestFile { manifestPath };

        if (manifestFile.existsAsFile())
        {
            const auto config { jam::ConfigDocument::parse (manifestFile.loadFileAsString(), manifestFile.getFullPathName()) };
            const bool hasSample { config != nullptr and config->getValueTree (Id::manifest).getChildWithName (jam::Format::toType (Id::sample)).isValid() };
            const auto childArguments { hasSample
                                            ? jam::Strings { "sudo", "-n", ProjectInfo::projectName, manifestFile.getFullPathName() }
                                            : jam::Strings { ProjectInfo::projectName, manifestFile.getFullPathName() } };
            auto run { std::make_unique<Run> (model, requestId) };

            if (run->client.open (childArguments, manifestFile.getParentDirectory(), jam::StdioClient::wantStdOut | jam::StdioClient::wantStdErr, nullptr, bufferSize, maximumPeriodMilliseconds))
            {
                runs.emplace (requestId, std::move (run));
            }
            else
            {
                model.state.removeChild (run->row, nullptr);
                server.sendToolError (requestId, "failed to launch scout");
            }
        }
        else
        {
            server.sendToolError (requestId, jam::Strings::getView (text::Diagnostics::failNotFound + Id::diagnosticSeparator + manifestFile.getFileName()));
        }
    }
    else
    {
        server.sendToolError (requestId, "manifest is not an absolute path");
    }
}

void ToolServer::appendOutput (Run& run)
{
    run.client.applyPendingData ([&run] (const char* data, int size)
    {
        if (size > 0)
        {
            const auto current { run.row[Id::output] };
            juce::MemoryBlock capture;

            if (const auto* existing { current.getBinaryData() }; existing != nullptr)
                capture = *existing;

            const auto cap { static_cast<size_t> (jam::Subprocess::byteCap) };

            if (capture.getSize() <= cap)
            {
                const auto remaining { cap - capture.getSize() };
                const auto incoming { static_cast<size_t> (size) };

                if (incoming <= remaining)
                {
                    capture.append (data, incoming);
                }
                else
                {
                    const std::string_view notice { jam::Subprocess::truncationNotice };

                    capture.append (data, remaining);
                    capture.append (notice.data(), notice.size());
                }

                run.row.setProperty (Id::output, capture, nullptr);
            }
        }

        return size;
    });
}

void ToolServer::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree.hasType (jam::Format::toType (Id::run)))
    {
        const auto requestId { tree[Id::id].toString() };

        if (runs.contains (requestId))
        {
            if (property == Id::numMessages)
            {
                appendOutput (*runs.at (requestId));
            }
            else if (property == Id::isOpen and static_cast<int> (tree[property]) == 0)
            {
                appendOutput (*runs.at (requestId));

                const int code { static_cast<int> (tree[Id::exitCode]) };
                const auto outputProperty { tree[Id::output] };
                const auto* capture { outputProperty.getBinaryData() };
                const std::string_view outputView { capture != nullptr ? std::string_view { static_cast<const char*> (capture->getData()), capture->getSize() } : std::string_view {} };

                if (code == 0)
                {
                    std::string json;

                    jam::JsonWriter::appendObject (json, [code, outputView] (std::string& members)
                    {
                        jam::JsonWriter::appendKey (members, Id::exitCode);
                        jam::Format::appendDecimal (members, code);
                        jam::JsonWriter::appendKey (members, Id::output);
                        jam::JsonWriter::appendString (members, outputView);
                    });

                    server.sendResponse (requestId, json);
                }
                else
                {
                    server.sendToolError (requestId, outputView);
                }

                model.state.removeChild (tree, nullptr);
                runs.erase (requestId);
            }
        }
    }
}

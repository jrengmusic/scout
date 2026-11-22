/**
 * @file ToolServer.h
 * @brief MCP server application that runs scout manifests in child processes over stdio.
 */

#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @class ToolServer
 * @brief Runs the JUCE application loop for the scout MCP stdio server.
 *
 * Registers the run tool callback and collects child process output on the message thread.
 */
class ToolServer : public juce::JUCEApplicationBase,
                   private juce::ValueTree::Listener
{
public:
    /**
     * @brief Registers the run tool, binds cancellation and input-close callbacks, and listens to model state.
     */
    ToolServer();

    /**
     * @brief Detaches this listener from model.state before destroying active runs and server.
     */
    ~ToolServer() override;

    /**
     * @brief Returns the application name from ProjectInfo.
     *
     * @return The project name string.
     */
    const juce::String getApplicationName() override;

    /**
     * @brief Returns the application version from ProjectInfo.
     *
     * @return The project version string.
     */
    const juce::String getApplicationVersion() override;

    /**
     * @brief Permits concurrent MCP server processes for separate clients.
     *
     * @return Always returns true.
     */
    bool moreThanOneInstanceAllowed() override;

    /**
     * @brief Starts the stdio MCP server loop on application startup.
     */
    void initialise (const juce::String&) override;

    /**
     * @brief Performs no extra shutdown work; input closure and destructors release state.
     */
    void shutdown() override;

    /**
     * @brief Ignores secondary launch notices because each stdio process serves one client.
     */
    void anotherInstanceStarted (const juce::String&) override;

    /**
     * @brief Quits the JUCE message loop when the host OS requests termination.
     */
    void systemRequestedQuit() override;

    /**
     * @brief Ignores application suspend events in headless stdio mode.
     */
    void suspended() override;

    /**
     * @brief Ignores application resume events in headless stdio mode.
     */
    void resumed() override;

    /**
     * @brief Leaves unhandled exceptions unintercepted; required JUCEApplicationBase override.
     */
    void unhandledException (const std::exception*, const juce::String&, int) override;

private:
    /**
     * @struct Run
     * @brief Owns the child scout process, stdio client, and Model state for one run call.
     */
    struct Run
    {
        /**
         * @brief Appends a run row to newModel.state and binds stdio read and exit callbacks.
         *
         * @param newModel      Shared Model whose state tree owns this run row.
         * @param newRequestId  MCP JSON-RPC request identifier for this manifest run.
         */
        Run (jam::Model& newModel, const juce::String& newRequestId);

        /** @brief ValueTree row storing request id, captured output, message count, exit code, and open status. */
        juce::ValueTree row { jam::Format::toType (Id::run) };

        /** @brief Atomic read-batch counter; client.onBytesReady increments it when bytes arrive. */
        jam::Parameter<int> numMessages { Id::numMessages, 0, Id::numMessages };

        /** @brief Atomic child exit code; client.onExit stores it before setting isOpen to zero. */
        jam::Parameter<int> exitCode { Id::exitCode, 0, Id::exitCode };

        /** @brief Atomic open flag; client.onExit sets it to zero when the child process exits. */
        jam::Parameter<int> isOpen { Id::isOpen, 1, Id::isOpen };

        /** @brief Message-thread attachment syncing numMessages onto row to wake output draining. */
        jam::Model::ParameterAttachment numMessagesAttachment;

        /** @brief Message-thread attachment writing exitCode and isOpen onto row when the child exits. */
        jam::Model::ParameterAttachment isOpenAttachment;

        /** @brief Stdio subprocess client running the child scout process with merged stdout and stderr. */
        jam::StdioClient client;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Run)
    };

    /**
     * @brief Validates the manifest path, prepends sudo for sample tables, and launches child scout.
     *
     * @param requestId  MCP JSON-RPC request identifier for the run tool call.
     * @param document   Parsed JSON document holding the tool call arguments.
     * @param arguments  JSON object element containing the absolute manifest path.
     */
    void runTool (const juce::String& requestId, const jam::JsonDocument& document, const jam::Document::Element& arguments);

    /**
     * @brief Drains pending output from run.client and appends it up to jam::Subprocess::byteCap.
     *
     * @param run  Active run instance whose buffered child output moves into run.row.
     */
    void appendOutput (Run& run);

    /**
     * @brief Drains child output on numMessages changes and emits the final response when isOpen reaches zero.
     *
     * @param tree      Modified ValueTree node inside model.state.
     * @param property  Identifier of the changed property on tree.
     */
    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override;

    /** @brief FIFO buffer capacity in bytes for reading merged child stdout and stderr. */
    static constexpr int bufferSize { 1 << 20 };

    /** @brief Polling period limit in milliseconds; zero dispatches reads without throttling. */
    static constexpr double maximumPeriodMilliseconds { 0.0 };

    /** @brief Shared Model owning active run ValueTree rows for message-thread dispatch. */
    jam::Model model { Id::run };

    /** @brief JSON-RPC stdio server exposing tools from embedded scoutTools metadata. */
    jam::ModelContextServer server { BinaryData::getString (files::scoutTools).toStdString(), ProjectInfo::projectName, ProjectInfo::versionString };

    /** @brief Active manifest runs keyed by MCP JSON-RPC request identifier. */
    jam::HashMap<juce::String, std::unique_ptr<Run>> runs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToolServer)
};

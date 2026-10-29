#pragma once
#include <JuceHeader.h>
#include "Symbolizer.h"
#include "generated/Generated.h"

#if JUCE_WINDOWS
#include <initguid.h>
#include <evntcons.h>
#endif

#if JUCE_MAC
/**
 * @brief One record of the kernel trace buffer, in the layout that the kernel writes.
 *
 * The size of the struct is 64 bytes. The kernel trace read fills an array of these records.
 */
struct kd_buf
{
    /** @brief Time of the event, in kernel timer ticks. */
    uint64_t timestamp;

    /** @brief First argument of the event. Its meaning depends on `debugid`. */
    uint64_t arg1;

    /** @brief Second argument of the event. Its meaning depends on `debugid`. */
    uint64_t arg2;

    /** @brief Third argument of the event. Its meaning depends on `debugid`. */
    uint64_t arg3;

    /** @brief Fourth argument of the event. Its meaning depends on `debugid`. */
    uint64_t arg4;

    /** @brief Fifth argument of the event. Its meaning depends on `debugid`. */
    uint64_t arg5;

    /** @brief Identifier of the event type. */
    uint32_t debugid;

    /** @brief Index of the CPU that recorded the event. */
    uint32_t cpuid;

    /** @brief Padding that the kernel reserves. */
    uint64_t unused;
};

static_assert (sizeof (kd_buf) == 64);
#endif

/**
 * @brief Samples the stacks of the processes that the scout process starts, while a command runs.
 *
 * The constructor starts a kernel sampling session and a thread that reads its records.
 * The sampler counts, for each table type, how many samples hold each frame.
 * The two table types are `sample` for the timer samples and `cswitch` for the context switches.
 * The `drain` function stops the session and the thread.
 * The `getSamples` function then returns the counts as a tree.
 * Only the descendants of the scout process contribute samples.
 */
class Sampler : private juce::Thread
{
public:
#if JUCE_MAC
    /** @brief Smallest stack depth that the macOS session accepts. */
    static constexpr int minimumStackDepth { 2 };

    /** @brief Largest stack depth that the macOS session accepts. */
    static constexpr int maximumStackDepth { 128 };
#endif

#if JUCE_LINUX
    /** @brief Smallest stack depth that the Linux session accepts. */
    static constexpr int minimumStackDepth { 1 };

    /** @brief Largest stack depth that the Linux session accepts. */
    static constexpr int maximumStackDepth { 127 };
#endif

#if JUCE_WINDOWS
    /** @brief Smallest stack depth that the Windows session accepts. */
    static constexpr int minimumStackDepth { 1 };

    /** @brief Largest stack depth that the Windows session accepts. */
    static constexpr int maximumStackDepth { 192 };
#endif

    /** @brief Smallest sampling interval, in milliseconds. */
    static constexpr int minimumIntervalMs { 1 };

    /**
     * @brief Starts the sampling session and the thread that reads its records.
     *
     * When the session does not start, the thread does not start.
     * The `drain` function returns the failure of the session.
     *
     * @param sampleTable  The `sample` table of the manifest. It holds the `intervalMs` and `stackDepth` properties.
     */
    explicit Sampler (const juce::ValueTree& sampleTable);

    /**
     * @brief Stops the thread and, when it is still active, the session.
     */
    ~Sampler() override;

    /**
     * @brief Stops the sampling and waits until the thread has read all records.
     *
     * Call it once, after the command ends.
     *
     * @return  Ok when the session ran and stopped without a failure. Otherwise the first failure of the session, of the stop or of the record read.
     */
    juce::Result drain();

    /**
     * @brief Returns the counts as a tree.
     *
     * The tree has one table for each table type. A table has one row for each frame key.
     * A row holds the `value` count, the `calls` per epoch and the `count` unit.
     * A `dropped` row holds the number of events that the kernel lost.
     *
     * @return  Tree of the `samples` type. Call it after `drain`.
     */
    juce::ValueTree getSamples() const;

private:
    /**
     * @brief Reads the records of the session until the thread must exit, then reads the rest.
     */
    void run() override;

    /**
     * @brief Adds one to the count of each frame key, for a table type.
     *
     * The function skips each key that is not valid.
     *
     * @param tableType   Table type that receives the counts.
     * @param sampleKeys  Frame keys of one sample. Each key counts one time.
     * @param weight      Value that each key adds to its count.
     */
    void addCounts (const juce::Identifier& tableType, const jam::HashSet<juce::Identifier>& sampleKeys, int weight);

    /** @brief Resolves the frame addresses to frame keys. */
    Symbolizer symbolizer;

#if JUCE_MAC or JUCE_WINDOWS
    /** @brief Identifiers of the processes that descend from the scout process. */
    jam::HashSet<int> descendants;
#endif

#if JUCE_MAC
    /**
     * @brief Passes each record to the member function that its event identifier selects.
     *
     * The function skips each record with an event that no member function handles.
     *
     * @param records      The records that the read returned.
     * @param recordCount  Number of records.
     */
    void addRecords (const kd_buf* records, int recordCount);

    /**
     * @brief Opens the bracket of the CPU of a record.
     *
     * The bracket holds the action and the thread of the sample start. It replaces an open bracket of the same CPU.
     *
     * @param record  Record of the sample start event.
     */
    void addSampleStart (const kd_buf& record);

    /**
     * @brief Sets the process of the open bracket of the CPU of a record.
     *
     * @param record  Record of the thread information event.
     */
    void addThreadInfo (const kd_buf& record);

    /**
     * @brief Sets the frame count and the kernel flag of the open bracket of the CPU of a record.
     *
     * @param record  Record of a kernel or user stack header event.
     */
    void addStackHeader (const kd_buf& record);

    /**
     * @brief Adds the frame addresses of a record to the open bracket of its CPU.
     *
     * The function adds frames until the bracket holds the frame count of the header.
     *
     * @param record  Record of a kernel or user stack frames event.
     */
    void addStackFrames (const kd_buf& record);

    /**
     * @brief Closes the bracket of the CPU of a record and counts its sample.
     *
     * A bracket counts only when its process is a descendant.
     * A kernel stack goes to `addKernelSamples`. A user stack goes to `addUserSamples`.
     *
     * @param record  Record of the sample end event.
     */
    void addSampleEnd (const kd_buf& record);

    /**
     * @brief Counts the kernel stack of a bracket.
     *
     * When the meta flags show that a user stack follows, the function also notes a pending user sample for the thread.
     *
     * @param bracket    Closed bracket with a kernel stack.
     * @param metaFlags  Flags of the sample end event.
     */
    void addKernelSamples (const juce::Array<juce::uint64>& bracket, juce::uint64 metaFlags);

    /**
     * @brief Counts the user stack of a bracket.
     *
     * The count goes to each table type that has a pending user sample for the thread. The weight is the pending count.
     * When the thread has no pending sample, the count goes to the table type of the bracket action.
     *
     * @param bracket  Closed bracket with a user stack.
     */
    void addUserSamples (const juce::Array<juce::uint64>& bracket);

    /**
     * @brief Reads records from the kernel trace buffer.
     *
     * When the read fails, the function stores the failure in `reading`.
     *
     * @param records   Array that receives the records.
     * @param capacity  Number of records that the array holds.
     *
     * @return  Number of records that the read returned. It is zero after a failure.
     */
    int readRecords (kd_buf* records, int capacity);

    /**
     * @brief Adds one to the `dropped` count of the `sample` table.
     *
     * @param record  Record of the lost events event.
     */
    void addLostEvents (const kd_buf& record);

    /**
     * @brief Replaces the descendants with the processes that now descend from the scout process.
     */
    void setDescendants();

    /** @brief The open bracket of each CPU, keyed by the identifier of the CPU. */
    jam::HashMap<juce::uint32, juce::Array<juce::uint64>> brackets;

    /** @brief Count of user samples that wait for a user stack, for each thread and each action. */
    jam::HashMap<juce::uint64, jam::HashMap<juce::uint64, int>> pendingUserSamples;

    /** @brief Result of the record reads. It holds the first read failure. */
    juce::Result reading { juce::Result::ok() };
#endif

#if JUCE_WINDOWS
    /**
     * @brief Receives each trace event record from the consumer and passes it to the member function that its event key selects.
     *
     * The function skips each record with an event that no member function handles.
     *
     * @param record  Event record. Its user context is the sampler.
     *
     * @return  Nothing.
     */
    static void WINAPI addRecord (PEVENT_RECORD record);

    /**
     * @brief Starts the trace session and its consumer.
     *
     * The function sets the privileges, loads the drivers, starts the trace, sets the interval and the stack walk, and opens the consumer.
     * When a step after the trace start fails, the function stops the trace.
     *
     * @param intervalMs  Sampling interval, in milliseconds.
     *
     * @return  Ok when all steps succeed. Otherwise the first failure.
     */
    juce::Result startSession (int intervalMs);

    /**
     * @brief Opens the real-time consumer of the trace session.
     *
     * @return  Ok when the consumer opens. Otherwise the failure of `OpenTrace`.
     */
    juce::Result openConsumer();

    /**
     * @brief Stores the time stamp and the event key of a sample or a context switch for its processor.
     *
     * The next stack event of the same processor uses the stored values.
     *
     * @param record  Record of a sampled profile or context switch event.
     */
    void addOrigin (const EVENT_RECORD& record);

    /**
     * @brief Counts the stack of a stack walk event.
     *
     * The function counts the stack when its process is a descendant and the stored origin of its processor has the same time stamp.
     * The table type comes from the event key of the origin.
     *
     * @param record  Record of the stack walk event.
     */
    void addStack (const EVENT_RECORD& record);

    /**
     * @brief Counts the frames of a stack.
     *
     * Kernel addresses resolve in the kernel process. Other addresses resolve in the process of the stack.
     *
     * @param tableType   Table type that receives the counts.
     * @param processId   Process of the stack.
     * @param stack       Start of the frame addresses.
     * @param frameCount  Number of frames.
     */
    void addFrames (const juce::Identifier& tableType, int processId, const void* stack, int frameCount);

    /**
     * @brief Adds the process of a process start event to the descendants when its parent is the scout process or a descendant.
     *
     * @param record  Record of the process start event.
     */
    void addDescendant (const EVENT_RECORD& record);

    /**
     * @brief Passes the image of an image load event to the symbolizer when its process is a descendant.
     *
     * @param record  Record of the image load event.
     */
    void addImage (const EVENT_RECORD& record);

    /** @brief Number of frames that the session keeps for each stack. */
    const int stackDepth;

    /** @brief Handle of the trace consumer. */
    PROCESSTRACE_HANDLE consumer { INVALID_PROCESSTRACE_HANDLE };

    /** @brief Time stamp and event key of the last sample or context switch, for each processor. */
    jam::HashMap<juce::uint16, juce::Array<juce::uint64>> origins;
#endif

#if JUCE_LINUX
    /**
     * @brief Opens the clock and context switch events and their rings on each CPU, then enables them.
     *
     * @param intervalMs  Sampling interval, in milliseconds.
     * @param stackDepth  Number of frames that each sample keeps.
     *
     * @return  Ok when all events and rings open. Otherwise the first failure, or a `not found` failure when the trace file system has no context switch event.
     */
    juce::Result startSession (int intervalMs, int stackDepth);

    /**
     * @brief Enables or disables all clock and context switch events.
     *
     * @param shouldEnable  True to enable the events. False to disable them.
     */
    void setEnabled (bool shouldEnable);

    /**
     * @brief Reads the new records of the rings of all CPUs.
     *
     * The clock rings fill the `sample` table type. The context switch rings fill the `cswitch` table type.
     */
    void addAllRecords();

    /**
     * @brief Passes each new record of a ring to the member function that its record type selects.
     *
     * The function then moves the tail of the ring to the head that it read.
     *
     * @param ring       Ring of one event on one CPU.
     * @param tableType  Table type that the ring fills.
     */
    void addRecords (void* ring, const juce::Identifier& tableType);

    /**
     * @brief Counts the frames of a sample record when its process is not the scout process.
     *
     * @param ring       Ring that holds the record.
     * @param position   Position of the record in the ring.
     * @param tableType  Table type that receives the counts.
     */
    void addSample (const void* ring, juce::uint64 position, const juce::Identifier& tableType);

    /**
     * @brief Adds the number of lost events of a record to the `dropped` count of a table type.
     *
     * @param ring       Ring that holds the record.
     * @param position   Position of the record in the ring.
     * @param tableType  Table type that receives the count.
     */
    void addLost (const void* ring, juce::uint64 position, const juce::Identifier& tableType);

    /**
     * @brief Counts the kernel and user frames of a sample record.
     *
     * The function splits the call chain at the user marker.
     * Kernel addresses resolve through the kernel symbols. User addresses resolve in the process of the sample.
     *
     * @param ring        Ring that holds the record.
     * @param position    Position of the record in the ring.
     * @param processId   Process of the sample.
     * @param tableType   Table type that receives the counts.
     */
    void addFrames (const void* ring, juce::uint64 position, int processId, const juce::Identifier& tableType);

    /** @brief File descriptor of the clock event of each CPU. */
    juce::Array<int> clockEvents;

    /** @brief File descriptor of the context switch event of each CPU. */
    juce::Array<int> switchEvents;

    /** @brief Ring of the clock event of each CPU. */
    std::vector<std::unique_ptr<void, void (*) (void*)>> clockRings;

    /** @brief Ring of the context switch event of each CPU. */
    std::vector<std::unique_ptr<void, void (*) (void*)>> switchRings;
#endif

    /** @brief Result of the session start. */
    const juce::Result session;

    /** @brief Count of each frame key, for each table type. */
    jam::HashMap<juce::Identifier, jam::HashMap<juce::Identifier, int>> counts { { Id::toType (Id::sample), {} }, { Id::toType (Id::cswitch), {} } };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sampler)
};

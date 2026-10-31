#include "Sampler.h"

static constexpr int stopTimeoutMilliseconds { 60000 };
static constexpr int unitWeight { 1 };
static constexpr std::string_view countUnit { "count" };
static constexpr std::string_view threadName { "scout::Sampler" };

#if JUCE_MAC or JUCE_LINUX
static double getIntervalNanoseconds (int intervalMs)
{
    return juce::RelativeTime::milliseconds (intervalMs).inSeconds() * jam::Profiler::nanosecondsPerSecond;
}
#endif

#if JUCE_MAC
#include <sys/sysctl.h>
#include <sys/kdebug.h>
#include <libproc.h>
#include <sys/proc_info.h>
#include <mach/mach_time.h>

static constexpr juce::uint64 timerAction { 1 };
static constexpr juce::uint64 switchAction { 2 };
static constexpr int recordCapacity { 1 << 20 };
static constexpr int readIntervalMs { 100 };
static constexpr int framesPerRecord { 4 };
static constexpr int kernelStackFlag { 1 << 3 };
static constexpr juce::uint64 userPendFlag { 1U << 1 };
static constexpr int timerCount { 1 };
static constexpr int on { 1 };
static constexpr int off { 0 };
static constexpr std::string_view kdebugName { "kdebug" };

static constexpr juce::uint32 sampleStartEvent { 0x25000001 };
static constexpr juce::uint32 sampleEndEvent { 0x25000002 };
static constexpr juce::uint32 threadInfoEvent { 0x25010004 };
static constexpr juce::uint32 kernelHeaderEvent { 0x25020014 };
static constexpr juce::uint32 kernelFramesEvent { 0x2502000c };
static constexpr juce::uint32 userHeaderEvent { 0x25020018 };
static constexpr juce::uint32 userFramesEvent { 0x25020010 };
static constexpr juce::uint32 schedulerEvent { 0x01400000 };
static constexpr juce::uint32 lostEventsEvent { TRACE_LOST_EVENTS };

static constexpr int actionSlot { 0 };
static constexpr int processSlot { 1 };
static constexpr int frameCountSlot { 2 };
static constexpr int kernelSlot { 3 };
static constexpr int threadSlot { 4 };
static constexpr int firstFrameSlot { 5 };

static constexpr juce::uint64 threadInfoSampler { 1U << 0 };
static constexpr juce::uint64 kernelStackSampler { 1U << 2 };
static constexpr juce::uint64 userStackSampler { 1U << 3 };
static constexpr int traceEnableFlag { 0x001 };
static constexpr juce::int64 debugidFilterType { 4 };

static juce::Result setKernelValue (const char* name, const void* value, size_t size)
{
    if (sysctlbyname (name, nullptr, nullptr, const_cast<void*> (value), size) == 0)
        return juce::Result::ok();

    return juce::Result::fail (juce::String (name) + Id::diagnosticSeparator + juce::String (std::strerror (errno)));
}

static juce::Result setKernelValue (const char* name, int value)
{
    return setKernelValue (name, &value, sizeof (value));
}

static juce::Result setKernelValue (const char* name, std::initializer_list<juce::uint64> values)
{
    return setKernelValue (name, values.begin(), values.size() * sizeof (juce::uint64));
}

static int getKernelValue (const char* name)
{
    int value { 0 };
    size_t size { sizeof (value) };
    [[maybe_unused]] const auto isRead { sysctlbyname (name, &value, &size, nullptr, 0) == 0 };
    jassert (isRead);

    return value;
}

static juce::Result setKdebugValue (int operation, int value)
{
    int mib[] { CTL_KERN, KERN_KDEBUG, operation, value };

    if (sysctl (mib, std::size (mib), nullptr, nullptr, nullptr, 0) == 0)
        return juce::Result::ok();

    return juce::Result::fail (jam::Strings::fromView (kdebugName) + juce::String (operation) + Id::diagnosticSeparator + juce::String (std::strerror (errno)));
}

static juce::Result setTypeFilter()
{
    juce::BigInteger bitmap;
    bitmap.setRange (KDBG_EXTRACT_CSC (KDBG_CLASS_ENCODE (DBG_PERF, 0)), KDBG_SUBCLASS_MAX + 1, true);
    bitmap.setBit (KDBG_EXTRACT_CSC (KDBG_CLASS_ENCODE (DBG_MACH, DBG_MACH_SCHED)));

    auto filter { bitmap.toMemoryBlock() };
    filter.setSize ((KDBG_CSC_MAX + 1) / CHAR_BIT, true);

    int mib[] { CTL_KERN, KERN_KDEBUG, KERN_KDSET_TYPEFILTER };
    size_t size { filter.getSize() };

    if (sysctl (mib, std::size (mib), filter.getData(), &size, nullptr, 0) == 0)
        return juce::Result::ok();

    return juce::Result::fail (jam::Strings::fromView (kdebugName) + juce::String (KERN_KDSET_TYPEFILTER) + Id::diagnosticSeparator + juce::String (std::strerror (errno)));
}

static juce::Result setSchedulerFilter()
{
    juce::MemoryOutputStream filter;
    filter.writeInt64 (debugidFilterType);
    filter.writeInt64 (0);
    filter.writeInt (schedulerEvent);

    return setKernelValue ("kperf.kdebug.filter", filter.getData(), filter.getDataSize());
}

static juce::Result setActionValues (int stackDepth)
{
    static constexpr juce::uint64 anyProcess { static_cast<juce::uint64> (-1) };
    static constexpr juce::uint64 allSamplers { threadInfoSampler | kernelStackSampler | userStackSampler };

    const auto depth { static_cast<juce::uint64> (stackDepth) };
    const std::array<std::pair<const char*, juce::uint64>, 4> actionValues
    {{
        { "kperf.action.samplers", allSamplers },
        { "kperf.action.filter_by_pid", anyProcess },
        { "kperf.action.kcallstack_depth", depth },
        { "kperf.action.ucallstack_depth", depth }
    }};

    for (const auto action : { timerAction, switchAction })
        for (const auto& [name, value] : actionValues)
            if (const auto result { setKernelValue (name, { action, value }) }; not result.wasOk())
                return result;

    return juce::Result::ok();
}

static juce::uint64 getTimerTicks (int intervalMs)
{
    mach_timebase_info_data_t timebase {};
    mach_timebase_info (&timebase);

    return static_cast<juce::uint64> (getIntervalNanoseconds (intervalMs) * timebase.denom / timebase.numer);
}

static juce::Result startSession (int intervalMs, int stackDepth)
{
    jassert (intervalMs >= Sampler::minimumIntervalMs);
    jassert (stackDepth >= Sampler::minimumStackDepth);
    jassert (stackDepth <= Sampler::maximumStackDepth);

    static constexpr juce::uint64 timerId { 0 };

    const std::vector<std::function<juce::Result()>> sessionSteps
    {
        [] { return setKernelValue ("kperf.reset", on); },
        [] { return setKernelValue ("kperf.action.count", getKernelValue ("kperf.limits.max_action_count")); },
        [stackDepth] { return setActionValues (stackDepth); },
        [] { return setKernelValue ("kperf.timer.count", timerCount); },
        [intervalMs] { return setKernelValue ("kperf.timer.period", { timerId, getTimerTicks (intervalMs) }); },
        [] { return setKernelValue ("kperf.timer.action", { timerId, timerAction }); },
        [] { return setKernelValue ("kperf.kdebug.action", static_cast<int> (switchAction)); },
        [] { return setSchedulerFilter(); },
        [] { return setKdebugValue (KERN_KDSETBUF, recordCapacity); },
        [] { return setKdebugValue (KERN_KDSETUP, 0); },
        [] { return setTypeFilter(); },
        [] { return setKdebugValue (KERN_KDENABLE, traceEnableFlag); },
        [] { return setKernelValue ("kperf.sampling", on); }
    };

    for (const auto& step : sessionSteps)
        if (const auto result { step() }; not result.wasOk())
            return result;

    return juce::Result::ok();
}

static juce::Result stopSession()
{
    const juce::Result results[]
    {
        setKernelValue ("kperf.sampling", off),
        setKdebugValue (KERN_KDENABLE, off),
        setKdebugValue (KERN_KDREMOVE, off),
        setKernelValue ("kperf.reset", on)
    };

    for (const auto& result : results)
        if (not result.wasOk())
            return result;

    return juce::Result::ok();
}

static bool isDescendant (int processId, int ancestorId)
{
    static constexpr int launchdProcessId { 1 };

    proc_bsdinfo info {};

    if (proc_pidinfo (processId, PROC_PIDTBSDINFO, 0, &info, sizeof (info)) == static_cast<int> (sizeof (info)))
    {
        const auto parentId { static_cast<int> (info.pbi_ppid) };

        if (parentId == ancestorId) return true;
        if (parentId > launchdProcessId) return isDescendant (parentId, ancestorId);
    }

    return false;
}

static const juce::Identifier& getActionTable (juce::uint64 action)
{
    static const jam::HashMap<juce::uint64, juce::Identifier> actionTables
    {
        { timerAction, Id::toType (Id::sample) },
        { switchAction, Id::toType (Id::cswitch) }
    };

    return actionTables.at (action);
}

Sampler::Sampler (const juce::ValueTree& sampleTable)
    : juce::Thread (jam::Strings::fromView (::threadName)),
      session { startSession (static_cast<int> (sampleTable[Id::intervalMs]), static_cast<int> (sampleTable[Id::stackDepth])) }
{
    if (session.wasOk())
        startThread();
}

Sampler::~Sampler()
{
    stopThread (stopTimeoutMilliseconds);

    [[maybe_unused]] const auto stopped { stopSession() };

    jassert (stopped.wasOk() or not session.wasOk());
}

juce::Result Sampler::drain()
{
    if (session.wasOk())
    {
        const auto sampling { setKernelValue ("kperf.sampling", off) };
        const auto tracing { setKdebugValue (KERN_KDENABLE, off) };

        signalThreadShouldExit();
        stopThread (stopTimeoutMilliseconds);

        if (not sampling.wasOk()) return sampling;
        if (not tracing.wasOk()) return tracing;

        return reading;
    }

    return session;
}

int Sampler::readRecords (kd_buf* records, int capacity)
{
    int mib[] { CTL_KERN, KERN_KDEBUG, KERN_KDREADTR };
    size_t size { static_cast<size_t> (capacity) * sizeof (kd_buf) };
    const auto status { sysctl (mib, std::size (mib), records, &size, nullptr, 0) };

    if (status != 0)
        reading = juce::Result::fail (jam::Strings::fromView (kdebugName) + juce::String (KERN_KDREADTR) + Id::diagnosticSeparator + juce::String (std::strerror (errno)));

    return status == 0 ? static_cast<int> (size) : 0;
}

void Sampler::run()
{
    juce::HeapBlock<kd_buf> records { recordCapacity };

    while (not threadShouldExit() and reading.wasOk())
    {
        setDescendants();
        addRecords (records, readRecords (records, recordCapacity));
        wait (readIntervalMs);
    }

    auto recordCount { readRecords (records, recordCapacity) };

    while (recordCount > 0)
    {
        addRecords (records, recordCount);
        recordCount = readRecords (records, recordCapacity);
    }
}

void Sampler::addRecords (const kd_buf* records, int recordCount)
{
    static const jam::HashMap<juce::uint32, void (Sampler::*) (const kd_buf&)> recordEvents
    {
        { kernelHeaderEvent, &Sampler::addStackHeader },
        { userHeaderEvent, &Sampler::addStackHeader },
        { kernelFramesEvent, &Sampler::addStackFrames },
        { userFramesEvent, &Sampler::addStackFrames },
        { sampleStartEvent, &Sampler::addSampleStart },
        { threadInfoEvent, &Sampler::addThreadInfo },
        { sampleEndEvent, &Sampler::addSampleEnd },
        { lostEventsEvent, &Sampler::addLostEvents }
    };

    for (const auto* record { records }; record != records + recordCount; ++record)
    {
        if (const auto event { recordEvents.find (record->debugid) }; event != recordEvents.end())
        {
            const auto& [eventId, recordMember] = *event;
            (this->*recordMember) (*record);
        }
    }
}

void Sampler::addSampleStart (const kd_buf& record)
{
    brackets[record.cpuid] = juce::Array<juce::uint64> { record.arg2, 0, 0, 0, record.arg5 };
}

void Sampler::addThreadInfo (const kd_buf& record)
{
    if (auto bracketEntry { brackets.find (record.cpuid) }; bracketEntry != brackets.end())
    {
        auto& [cpuId, bracket] = *bracketEntry;
        bracket.set (processSlot, record.arg1);
    }
}

void Sampler::addStackHeader (const kd_buf& record)
{
    if (auto bracketEntry { brackets.find (record.cpuid) }; bracketEntry != brackets.end())
    {
        auto& [cpuId, bracket] = *bracketEntry;

        bracket.set (frameCountSlot, record.arg2);
        bracket.set (kernelSlot, record.arg1 & kernelStackFlag);
    }
}

void Sampler::addStackFrames (const kd_buf& record)
{
    if (auto bracketEntry { brackets.find (record.cpuid) }; bracketEntry != brackets.end())
    {
        auto& [cpuId, bracket] = *bracketEntry;
        const std::array<juce::uint64, framesPerRecord> frames { record.arg1, record.arg2, record.arg3, record.arg4 };
        const auto frameCount { static_cast<int> (bracket.getReference (frameCountSlot)) };

        for (const auto address : frames)
            if (bracket.size() - firstFrameSlot < frameCount)
                bracket.add (address);
    }
}

void Sampler::addSampleEnd (const kd_buf& record)
{
    if (auto bracketEntry { brackets.find (record.cpuid) }; bracketEntry != brackets.end())
    {
        auto& [cpuId, bracket] = *bracketEntry;

        if (descendants.contains (static_cast<int> (bracket.getReference (processSlot))))
        {
            if (bracket.getReference (kernelSlot) != 0)
                addKernelSamples (bracket, record.arg2);
            else
                addUserSamples (bracket);
        }

        brackets.erase (record.cpuid);
    }
}

void Sampler::addKernelSamples (const juce::Array<juce::uint64>& bracket, juce::uint64 metaFlags)
{
    const auto action { bracket.getReference (actionSlot) };
    jam::HashSet<juce::Identifier> sampleKeys;

    for (int slot { firstFrameSlot }; slot < bracket.size() - 1; ++slot)
        sampleKeys.insert (symbolizer.getOrCreateKernelFrameKey (bracket.getReference (slot)));

    addCounts (getActionTable (action), sampleKeys, unitWeight);

    if ((metaFlags & userPendFlag) != 0)
        ++pendingUserSamples[bracket.getReference (threadSlot)][action];
}

void Sampler::addUserSamples (const juce::Array<juce::uint64>& bracket)
{
    const auto processId { static_cast<int> (bracket.getReference (processSlot)) };
    const auto threadId { bracket.getReference (threadSlot) };
    jam::HashSet<juce::Identifier> sampleKeys;

    for (int slot { firstFrameSlot }; slot < bracket.size() - 1; ++slot)
        sampleKeys.insert (symbolizer.getOrCreateUserFrameKey (processId, bracket.getReference (slot)));

    auto& pendingActions { pendingUserSamples[threadId] };

    if (pendingActions.empty())
        pendingActions.try_emplace (bracket.getReference (actionSlot), unitWeight);

    for (const auto& [action, pendingCount] : pendingActions)
        addCounts (getActionTable (action), sampleKeys, pendingCount);

    pendingUserSamples.erase (threadId);
}

void Sampler::addLostEvents (const kd_buf&)
{
    ++counts[Id::toType (Id::sample)][Id::dropped];
}

void Sampler::setDescendants()
{
    const auto capacity { proc_listallpids (nullptr, 0) };
    juce::HeapBlock<int> processIds { static_cast<size_t> (capacity) };
    const auto processCount { proc_listallpids (processIds, capacity * static_cast<int> (sizeof (int))) };

    descendants.clear();

    for (int index { 0 }; index < processCount; ++index)
        if (isDescendant (processIds[index], getpid()))
            descendants.insert (processIds[index]);
}
#endif

#if JUCE_LINUX
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

static_assert (JUCE_LITTLE_ENDIAN);

static constexpr int readIntervalMs { 10 };

static constexpr int processSlot { 1 };
static constexpr int frameCountSlot { 2 };
static constexpr int firstFrameSlot { 3 };
static constexpr int lostSlot { 2 };
static constexpr size_t wordBytes { sizeof (juce::uint64) };

static constexpr std::array<std::string_view, 2> tracefsRoots { "/sys/kernel/tracing", "/sys/kernel/debug/tracing" };
static constexpr std::string_view switchIdPath { "events/sched/sched_switch/id" };
static constexpr int noGroup { -1 };
static constexpr size_t ringDataBytes { 1 << 20 };

static perf_event_attr getEventAttributes (juce::uint32 type, juce::uint64 config, juce::uint64 period, int stackDepth)
{
    perf_event_attr attributes {};

    attributes.type = type;
    attributes.size = sizeof (perf_event_attr);
    attributes.config = config;
    attributes.sample_period = period;
    attributes.sample_type = PERF_SAMPLE_TID | PERF_SAMPLE_CALLCHAIN;
    attributes.sample_max_stack = static_cast<juce::uint16> (stackDepth);
    attributes.disabled = 1;
    attributes.inherit = 1;

    return attributes;
}

static perf_event_attr getClockAttributes (int intervalMs, int stackDepth)
{
    jassert (intervalMs >= Sampler::minimumIntervalMs);
    jassert (stackDepth >= Sampler::minimumStackDepth);
    jassert (stackDepth <= Sampler::maximumStackDepth);

    return getEventAttributes (PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_CLOCK, static_cast<juce::uint64> (getIntervalNanoseconds (intervalMs)), stackDepth);
}

static juce::File getSwitchIdFile()
{
    for (const auto root : tracefsRoots)
        if (const auto idFile { juce::File (jam::Strings::fromView (root)).getChildFile (jam::Strings::fromView (switchIdPath)) }; idFile.existsAsFile())
            return idFile;

    return juce::File (jam::Strings::fromView (tracefsRoots.at (0))).getChildFile (jam::Strings::fromView (switchIdPath));
}

static perf_event_attr getSwitchAttributes (const juce::File& switchIdFile, int stackDepth)
{
    static constexpr juce::uint64 everyEvent { 1 };

    return getEventAttributes (PERF_TYPE_TRACEPOINT, static_cast<juce::uint64> (switchIdFile.loadFileAsString().getIntValue()), everyEvent, stackDepth);
}

static int openEvent (const perf_event_attr& attributes, pid_t threadId, int cpu)
{
    return static_cast<int> (syscall (SYS_perf_event_open, &attributes, threadId, cpu, noGroup, PERF_FLAG_FD_CLOEXEC));
}

static void unmapRing (void* ring)
{
    munmap (ring, ringDataBytes + static_cast<size_t> (juce::SystemStats::getPageSize()));
}

static std::unique_ptr<void, void (*) (void*)> getRing (int event)
{
    void* mapping { MAP_FAILED };

    if (event >= 0)
        mapping = mmap (nullptr, ringDataBytes + static_cast<size_t> (juce::SystemStats::getPageSize()), PROT_READ | PROT_WRITE, MAP_SHARED, event, 0);

    return std::unique_ptr<void, void (*) (void*)> { mapping == MAP_FAILED ? nullptr : mapping, unmapRing };
}

static juce::uint64 getHead (const void* ring)
{
    return __atomic_load_n (&static_cast<const perf_event_mmap_page*> (ring)->data_head, __ATOMIC_ACQUIRE);
}

static juce::uint64 getTail (const void* ring)
{
    return static_cast<const perf_event_mmap_page*> (ring)->data_tail;
}

static void setTail (void* ring, juce::uint64 position)
{
    __atomic_store_n (&static_cast<perf_event_mmap_page*> (ring)->data_tail, position, __ATOMIC_RELEASE);
}

static perf_event_header getHeader (const void* ring, juce::uint64 position)
{
    const auto* data { static_cast<const char*> (ring) + juce::SystemStats::getPageSize() };

    return juce::readUnaligned<perf_event_header> (data + (position & (ringDataBytes - 1)));
}

static juce::uint64 getWord (const void* ring, juce::uint64 position)
{
    const auto* data { static_cast<const char*> (ring) + juce::SystemStats::getPageSize() };

    return juce::readUnaligned<juce::uint64> (data + (position & (ringDataBytes - 1)));
}

Sampler::Sampler (const juce::ValueTree& sampleTable)
    : juce::Thread (jam::Strings::fromView (::threadName)),
      session { startSession (static_cast<int> (sampleTable[Id::intervalMs]), static_cast<int> (sampleTable[Id::stackDepth])) }
{
    if (session.wasOk())
        startThread();
}

Sampler::~Sampler()
{
    stopThread (stopTimeoutMilliseconds);

    for (const auto* events : { &clockEvents, &switchEvents })
        for (const auto event : *events)
            if (event >= 0)
                close (event);
}

juce::Result Sampler::drain()
{
    if (session.wasOk())
    {
        setEnabled (false);
        signalThreadShouldExit();
        stopThread (stopTimeoutMilliseconds);
    }

    return session;
}

juce::Result Sampler::startSession (int intervalMs, int stackDepth)
{
    const auto switchIdFile { getSwitchIdFile() };

    if (switchIdFile.existsAsFile())
    {
        const auto clockAttributes { getClockAttributes (intervalMs, stackDepth) };
        const auto switchAttributes { getSwitchAttributes (switchIdFile, stackDepth) };

        for (int cpu { 0 }; cpu < juce::SystemStats::getNumCpus(); ++cpu)
        {
            clockEvents.add (openEvent (clockAttributes, gettid(), cpu));
            clockRings.push_back (getRing (clockEvents.getLast()));
            switchEvents.add (openEvent (switchAttributes, gettid(), cpu));
            switchRings.push_back (getRing (switchEvents.getLast()));

            if (clockEvents.getLast() < 0 or switchEvents.getLast() < 0)
                return juce::Result::fail (juce::String ("perf_event_open") + Id::diagnosticSeparator + juce::String (std::strerror (errno)));

            if (clockRings.back() == nullptr or switchRings.back() == nullptr)
                return juce::Result::fail (juce::String ("mmap") + Id::diagnosticSeparator + juce::String (std::strerror (errno)));
        }

        setEnabled (true);

        return juce::Result::ok();
    }

    return juce::Result::fail (juce::String (text::Diagnostics::failNotFound) + Id::diagnosticSeparator + switchIdFile.getFullPathName());
}

void Sampler::setEnabled (bool shouldEnable)
{
    for (const auto* events : { &clockEvents, &switchEvents })
    {
        for (const auto event : *events)
        {
            [[maybe_unused]] const auto status { ioctl (event, shouldEnable ? PERF_EVENT_IOC_ENABLE : PERF_EVENT_IOC_DISABLE, 0) };

            jassert (status == 0);
        }
    }
}

void Sampler::run()
{
    while (not threadShouldExit())
    {
        addAllRecords();
        wait (readIntervalMs);
    }

    addAllRecords();
}

void Sampler::addAllRecords()
{
    for (const auto& ring : clockRings)
        addRecords (ring.get(), Id::toType (Id::sample));

    for (const auto& ring : switchRings)
        addRecords (ring.get(), Id::toType (Id::cswitch));
}

void Sampler::addRecords (void* ring, const juce::Identifier& tableType)
{
    static const jam::HashMap<juce::uint32, void (Sampler::*) (const void*, juce::uint64, const juce::Identifier&)> recordTypes
    {
        { PERF_RECORD_SAMPLE, &Sampler::addSample },
        { PERF_RECORD_LOST, &Sampler::addLost }
    };

    const auto head { getHead (ring) };
    auto position { getTail (ring) };

    while (position < head)
    {
        const auto header { getHeader (ring, position) };

        if (const auto record { recordTypes.find (header.type) }; record != recordTypes.end())
        {
            const auto& [recordType, recordMember] = *record;
            (this->*recordMember) (ring, position, tableType);
        }

        position += header.size;
    }

    setTail (ring, position);
}

void Sampler::addSample (const void* ring, juce::uint64 position, const juce::Identifier& tableType)
{
    const auto processId { static_cast<int> (static_cast<juce::uint32> (getWord (ring, position + processSlot * wordBytes))) };

    if (processId != getpid())
        addFrames (ring, position, processId, tableType);
}

void Sampler::addLost (const void* ring, juce::uint64 position, const juce::Identifier& tableType)
{
    counts[tableType][Id::dropped] += static_cast<int> (getWord (ring, position + lostSlot * wordBytes));
}

void Sampler::addFrames (const void* ring, juce::uint64 position, int processId, const juce::Identifier& tableType)
{
    const auto frameCount { getWord (ring, position + frameCountSlot * wordBytes) };
    const auto getFrame = [ring, position] (juce::uint64 frame) { return getWord (ring, position + (firstFrameSlot + frame) * wordBytes); };
    juce::uint64 userMarker { 0 };

    while (userMarker < frameCount and getFrame (userMarker) != PERF_CONTEXT_USER)
        ++userMarker;

    jam::HashSet<juce::Identifier> sampleKeys;

    for (juce::uint64 frame { 0 }; frame < userMarker; ++frame)
    {
        const auto address { getFrame (frame) };

        if (address < PERF_CONTEXT_MAX)
            sampleKeys.insert (symbolizer.getOrCreateKernelFrameKey (address));
    }

    for (auto frame { userMarker + 1 }; frame < frameCount; ++frame)
    {
        const auto address { getFrame (frame) };

        if (address < PERF_CONTEXT_MAX)
            sampleKeys.insert (symbolizer.getOrCreateUserFrameKey (processId, address));
    }

    addCounts (tableType, sampleKeys, unitWeight);
}
#endif
#if JUCE_WINDOWS
#include <tdh.h>

#pragma comment (lib, "tdh.lib")

static_assert (JUCE_LITTLE_ENDIAN);

static constexpr int timestampSlot { 0 };
static constexpr int eventSlot { 1 };
static constexpr int stackHeaderBytes { 16 };
static constexpr int kernelAddressBit { 63 };
static constexpr int bufferSizeKilobytes { 1024 };
static constexpr int minimumBuffersPerCpu { 4 };
static constexpr int maximumBuffersPerCpu { 6 };
static constexpr int flushTimerSeconds { 1 };
static constexpr int hundredNanosecondsPerMillisecond { 10000 };
static constexpr const wchar_t* processIdName { L"ProcessId" };

static constexpr GUID threadGuid { 0x3d6fa8d1, 0xfe05, 0x11d0, { 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c } };
static constexpr GUID perfInfoGuid { 0xce1dbfb4, 0x137e, 0x4da6, { 0x87, 0xb0, 0x3f, 0x59, 0xaa, 0x10, 0x2c, 0xbc } };

static constexpr juce::uint32 threadProvider { threadGuid.Data1 };
static constexpr juce::uint32 perfInfoProvider { perfInfoGuid.Data1 };
static constexpr juce::uint32 stackWalkProvider { 0xdef2fe46 };
static constexpr juce::uint32 processProvider { 0x3d6fa8d0 };
static constexpr juce::uint32 imageProvider { 0x2cb15d1d };

static constexpr juce::uint8 cswitchOpcode { 36 };
static constexpr juce::uint8 sampledProfileOpcode { 46 };
static constexpr juce::uint8 stackWalkOpcode { 32 };
static constexpr juce::uint8 processStartOpcode { 1 };
static constexpr juce::uint8 imageLoadOpcode { 10 };

static constexpr juce::uint64 getEventKey (juce::uint32 providerData, juce::uint8 opcode) noexcept
{
    return (static_cast<juce::uint64> (providerData) << CHAR_BIT) | opcode;
}

static constexpr juce::uint64 cswitchEvent { getEventKey (threadProvider, cswitchOpcode) };
static constexpr juce::uint64 sampledProfileEvent { getEventKey (perfInfoProvider, sampledProfileOpcode) };
static constexpr juce::uint64 stackWalkEvent { getEventKey (stackWalkProvider, stackWalkOpcode) };
static constexpr juce::uint64 processStartEvent { getEventKey (processProvider, processStartOpcode) };
static constexpr juce::uint64 imageLoadEvent { getEventKey (imageProvider, imageLoadOpcode) };

static juce::uint64 getPropertyValue (const EVENT_RECORD& record, const wchar_t* propertyName)
{
    PROPERTY_DATA_DESCRIPTOR descriptor {};
    descriptor.PropertyName = reinterpret_cast<ULONGLONG> (propertyName);
    descriptor.ArrayIndex = ULONG_MAX;

    auto* event { const_cast<PEVENT_RECORD> (&record) };
    juce::uint64 value {};
    ULONG size {};

    [[maybe_unused]] const auto sizeStatus { TdhGetPropertySize (event, 0, nullptr, 1, &descriptor, &size) };

    jassert (sizeStatus == ERROR_SUCCESS);

    juce::HeapBlock<BYTE> data { size };
    [[maybe_unused]] const auto propertyStatus { TdhGetProperty (event, 0, nullptr, 1, &descriptor, size, data) };

    jassert (propertyStatus == ERROR_SUCCESS);

    std::memcpy (&value, data.get(), juce::jmin (static_cast<size_t> (size), sizeof (value)));

    return value;
}

static juce::String getPropertyText (const EVENT_RECORD& record, const wchar_t* propertyName)
{
    PROPERTY_DATA_DESCRIPTOR descriptor {};
    descriptor.PropertyName = reinterpret_cast<ULONGLONG> (propertyName);
    descriptor.ArrayIndex = ULONG_MAX;

    auto* event { const_cast<PEVENT_RECORD> (&record) };
    ULONG size {};

    [[maybe_unused]] const auto sizeStatus { TdhGetPropertySize (event, 0, nullptr, 1, &descriptor, &size) };

    jassert (sizeStatus == ERROR_SUCCESS);

    juce::HeapBlock<wchar_t> text { size / sizeof (wchar_t) + 1, true };
    [[maybe_unused]] const auto propertyStatus { TdhGetProperty (event, 0, nullptr, 1, &descriptor, size, reinterpret_cast<PBYTE> (text.get())) };

    jassert (propertyStatus == ERROR_SUCCESS);

    return juce::String (text.get());
}

static juce::Result setPrivilege (const wchar_t* privilegeName)
{
    TOKEN_PRIVILEGES privileges {};
    HANDLE token {};

    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (LookupPrivilegeValueW (nullptr, privilegeName, &privileges.Privileges[0].Luid))
    {
        if (OpenProcessToken (GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token))
        {
            const juce::ScopeGuard closeToken { [token] { CloseHandle (token); } };

            SetLastError (ERROR_SUCCESS);
            AdjustTokenPrivileges (token, FALSE, &privileges, 0, nullptr, nullptr);

            return getResult ("AdjustTokenPrivileges", GetLastError());
        }

        return getResult ("OpenProcessToken", GetLastError());
    }

    return getResult ("LookupPrivilegeValue", GetLastError());
}

static juce::MemoryBlock getSessionProperties()
{
    static constexpr ULONG enableFlags { EVENT_TRACE_FLAG_PROFILE | EVENT_TRACE_FLAG_CSWITCH | EVENT_TRACE_FLAG_PROCESS | EVENT_TRACE_FLAG_IMAGE_LOAD };

    juce::MemoryBlock block { sizeof (EVENT_TRACE_PROPERTIES) + sizeof (KERNEL_LOGGER_NAMEW), true };
    auto* properties { static_cast<EVENT_TRACE_PROPERTIES*> (block.getData()) };
    const auto cpuCount { static_cast<ULONG> (juce::SystemStats::getNumCpus()) };

    properties->Wnode.BufferSize = static_cast<ULONG> (block.getSize());
    properties->Wnode.Guid = SystemTraceControlGuid;
    properties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    properties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    properties->LoggerNameOffset = sizeof (EVENT_TRACE_PROPERTIES);
    properties->EnableFlags = enableFlags;
    properties->BufferSize = bufferSizeKilobytes;
    properties->MinimumBuffers = cpuCount * minimumBuffersPerCpu;
    properties->MaximumBuffers = cpuCount * maximumBuffersPerCpu;
    properties->FlushTimer = flushTimerSeconds;

    return block;
}

static juce::Result stopSession()
{
    auto properties { getSessionProperties() };

    return getResult ("ControlTrace", ControlTraceW (0, KERNEL_LOGGER_NAMEW, static_cast<EVENT_TRACE_PROPERTIES*> (properties.getData()), EVENT_TRACE_CONTROL_STOP));
}

static juce::Result setProfileInterval (int intervalMs)
{
    TRACE_PROFILE_INTERVAL profile {};
    profile.Interval = static_cast<ULONG> (intervalMs * hundredNanosecondsPerMillisecond);

    return getResult ("TraceSetInformation", TraceSetInformation (0, TraceSampledProfileIntervalInfo, &profile, sizeof (profile)));
}

static juce::Result setStackWalk (TRACEHANDLE sessionHandle)
{
    CLASSIC_EVENT_ID events[] { { perfInfoGuid, sampledProfileOpcode }, { threadGuid, cswitchOpcode } };

    return getResult ("TraceSetInformation", TraceSetInformation (sessionHandle, TraceStackTracingInfo, events, sizeof (events)));
}

Sampler::Sampler (const juce::ValueTree& sampleTable)
    : juce::Thread (jam::Strings::fromView (::threadName)),
      stackDepth { static_cast<int> (sampleTable[Id::stackDepth]) },
      session { startSession (static_cast<int> (sampleTable[Id::intervalMs])) }
{
    if (session.wasOk())
        startThread();
}

Sampler::~Sampler()
{
    if (session.wasOk() and isThreadRunning())
    {
        [[maybe_unused]] const auto stopped { stopSession() };

        jassert (stopped.wasOk());
    }

    stopThread (stopTimeoutMilliseconds);
}

juce::Result Sampler::drain()
{
    if (session.wasOk())
    {
        auto block { getSessionProperties() };
        auto* properties { static_cast<EVENT_TRACE_PROPERTIES*> (block.getData()) };
        const auto queried { getResult ("ControlTrace", ControlTraceW (0, KERNEL_LOGGER_NAMEW, properties, EVENT_TRACE_CONTROL_QUERY)) };
        const auto eventsLost { static_cast<int> (properties->EventsLost) };
        const auto stopped { stopSession() };

        stopThread (stopTimeoutMilliseconds);

        if (eventsLost > 0)
            counts[Id::toType (Id::sample)][Id::dropped] += eventsLost;

        return queried.wasOk() ? stopped : queried;
    }

    return session;
}

void Sampler::run()
{
    static constexpr ULONG consumerCount { 1 };

    const juce::ScopeGuard closeConsumer { [this] { CloseTrace (consumer); } };
    [[maybe_unused]] const auto status { ProcessTrace (&consumer, consumerCount, nullptr, nullptr) };

    jassert (status == ERROR_SUCCESS or status == ERROR_CANCELLED);
}

juce::Result Sampler::startSession (int intervalMs)
{
    jassert (intervalMs >= minimumIntervalMs);
    jassert (stackDepth >= minimumStackDepth);
    jassert (stackDepth <= maximumStackDepth);

    TRACEHANDLE sessionHandle {};
    auto properties { getSessionProperties() };

    const std::vector<std::function<juce::Result()>> startSteps
    {
        [] { return setPrivilege (SE_SYSTEM_PROFILE_NAME); },
        [] { return setPrivilege (SE_DEBUG_NAME); },
        [this] { return symbolizer.addDrivers(); },
        [&sessionHandle, &properties] { return getResult ("StartTrace", StartTraceW (&sessionHandle, KERNEL_LOGGER_NAMEW, static_cast<EVENT_TRACE_PROPERTIES*> (properties.getData()))); }
    };

    const std::vector<std::function<juce::Result()>> sessionSteps
    {
        [intervalMs] { return setProfileInterval (intervalMs); },
        [&sessionHandle] { return setStackWalk (sessionHandle); },
        [this] { return openConsumer(); }
    };

    for (const auto& step : startSteps)
        if (const auto result { step() }; not result.wasOk())
            return result;

    for (const auto& step : sessionSteps)
    {
        if (const auto result { step() }; not result.wasOk())
        {
            [[maybe_unused]] const auto stopped { stopSession() };

            jassert (stopped.wasOk());

            return result;
        }
    }

    return juce::Result::ok();
}

juce::Result Sampler::openConsumer()
{
    EVENT_TRACE_LOGFILEW logfile {};

    logfile.LoggerName = const_cast<LPWSTR> (KERNEL_LOGGER_NAMEW);
    logfile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
    logfile.EventRecordCallback = &Sampler::addRecord;
    logfile.Context = this;

    consumer = OpenTraceW (&logfile);

    return getResult ("OpenTrace", consumer == INVALID_PROCESSTRACE_HANDLE ? GetLastError() : ERROR_SUCCESS);
}

void WINAPI Sampler::addRecord (PEVENT_RECORD record)
{
    static const jam::HashMap<juce::uint64, void (Sampler::*) (const EVENT_RECORD&)> recordEvents
    {
        { cswitchEvent, &Sampler::addOrigin },
        { sampledProfileEvent, &Sampler::addOrigin },
        { stackWalkEvent, &Sampler::addStack },
        { processStartEvent, &Sampler::addDescendant },
        { imageLoadEvent, &Sampler::addImage }
    };

    const auto& header { record->EventHeader };

    if (const auto event { recordEvents.find (getEventKey (header.ProviderId.Data1, header.EventDescriptor.Opcode)) }; event != recordEvents.end())
    {
        const auto& [eventKey, recordMember] = *event;
        (static_cast<Sampler*> (record->UserContext)->*recordMember) (*record);
    }
}

void Sampler::addOrigin (const EVENT_RECORD& record)
{
    const auto& header { record.EventHeader };

    origins[record.BufferContext.ProcessorIndex] = juce::Array<juce::uint64> { static_cast<juce::uint64> (header.TimeStamp.QuadPart), getEventKey (header.ProviderId.Data1, header.EventDescriptor.Opcode) };
}

void Sampler::addStack (const EVENT_RECORD& record)
{
    static const jam::HashMap<juce::uint64, juce::Identifier> stackTables
    {
        { sampledProfileEvent, Id::toType (Id::sample) },
        { cswitchEvent, Id::toType (Id::cswitch) }
    };

    juce::MemoryInputStream payload { record.UserData, record.UserDataLength, false };
    const auto eventTimeStamp { static_cast<juce::uint64> (payload.readInt64()) };
    const auto stackProcess { payload.readInt() };
    const auto origin { origins.find (record.BufferContext.ProcessorIndex) };

    payload.skipNextBytes (sizeof (juce::uint32));

    if (descendants.contains (stackProcess) and origin != origins.end())
    {
        const auto& [processorIndex, originSlots] = *origin;
        const auto table { stackTables.find (originSlots.getReference (eventSlot)) };

        if (originSlots.getReference (timestampSlot) == eventTimeStamp and table != stackTables.end())
        {
            const auto& [eventKey, tableType] = *table;
            const auto frameCount { juce::jmin ((record.UserDataLength - stackHeaderBytes) / static_cast<int> (sizeof (juce::uint64)), stackDepth) };
            const auto* stack { static_cast<const char*> (payload.getData()) + payload.getPosition() };

            addFrames (tableType, stackProcess, stack, frameCount);
        }
    }
}

void Sampler::addFrames (const juce::Identifier& tableType, int processId, const void* stack, int frameCount)
{
    jam::HashSet<juce::Identifier> stackKeys;

    for (int frame { 0 }; frame < frameCount; ++frame)
    {
        const auto address { juce::readUnaligned<juce::uint64> (static_cast<const char*> (stack) + frame * sizeof (juce::uint64)) };
        const auto isKernel { (address >> kernelAddressBit) != 0 };

        stackKeys.insert (symbolizer.getOrCreateFrameKey (isKernel ? Symbolizer::kernelProcessId : processId, address));
    }

    addCounts (tableType, stackKeys, unitWeight);
}

void Sampler::addDescendant (const EVENT_RECORD& record)
{
    const auto processId { static_cast<int> (getPropertyValue (record, processIdName)) };
    const auto parentId { static_cast<int> (getPropertyValue (record, L"ParentId")) };

    if (parentId == static_cast<int> (GetCurrentProcessId()) or descendants.contains (parentId))
        descendants.insert (processId);
}

void Sampler::addImage (const EVENT_RECORD& record)
{
    const auto processId { static_cast<int> (getPropertyValue (record, processIdName)) };

    if (descendants.contains (processId))
        symbolizer.addImage (processId, getPropertyValue (record, L"ImageBase"), getPropertyValue (record, L"ImageSize"), getPropertyText (record, L"FileName"));
}
#endif

void Sampler::addCounts (const juce::Identifier& tableType, const jam::HashSet<juce::Identifier>& sampleKeys, int weight)
{
    auto& sampleCounts { counts[tableType] };

    for (const auto& sampleKey : sampleKeys)
        if (sampleKey.isValid())
            sampleCounts[sampleKey] += weight;
}

juce::ValueTree Sampler::getSamples() const
{
    juce::ValueTree samples { Id::samples };

    for (const auto& [tableType, sampleCounts] : counts)
    {
        juce::ValueTree table { tableType };

        for (const auto& [sampleKey, count] : sampleCounts)
        {
            juce::ValueTree row { sampleKey };
            row.setProperty (Id::value, count, nullptr);
            row.setProperty (Id::calls, jam::Profiler::callsPerEpoch, nullptr);
            row.setProperty (Id::unit, jam::Strings::fromView (countUnit), nullptr);
            table.appendChild (row, nullptr);
        }

        samples.appendChild (table, nullptr);
    }

    return samples;
}

#pragma once
#include <JuceHeader.h>

#if JUCE_WINDOWS
/**
 * @brief Returns the result of a Windows status code.
 *
 * @param name    Name of the call that returned the status. A failure starts with it.
 * @param status  Status code of the call.
 *
 * @return  Ok when the status is `ERROR_SUCCESS`. Otherwise a failure with the name and the system message of the status.
 */
juce::Result getResult (const char* name, ULONG status);
#endif

/**
 * @brief Resolves stack frame addresses to frame keys.
 *
 * A frame key is an identifier from the name of the symbol of the address.
 * When the image has no symbol for the address, the key comes from the name of the image.
 * When the address has no image, the key is not valid.
 * The object keeps each key that it made, so each address resolves one time.
 */
class Symbolizer
{
public:
    /** @brief Makes an empty symbolizer. */
    Symbolizer();

    /** @brief Releases the symbol state. */
    ~Symbolizer();

#if JUCE_MAC or JUCE_LINUX
    /**
     * @brief Returns the frame key of a kernel address.
     *
     * The function reads the kernel symbols one time. It makes the key when the address is new.
     *
     * @param address  Address in the kernel.
     *
     * @return  Frame key of the kernel symbol, or of the kernel image name when no symbol precedes the address.
     */
    juce::Identifier getOrCreateKernelFrameKey (juce::uint64 address);

    /**
     * @brief Returns the frame key of a user address.
     *
     * The function finds the image that holds the address in the process. It reads the symbols of the image one time.
     * It makes the key when the address is new.
     *
     * @param processId  Process that holds the address.
     * @param address    Address in the process.
     *
     * @return  Frame key of the symbol, or of the image name when no symbol precedes the address.
     *          The key is not valid when no image holds the address.
     */
    juce::Identifier getOrCreateUserFrameKey (int processId, juce::uint64 address);
#endif

#if JUCE_WINDOWS
    /** @brief Process identifier that the system uses for the kernel. */
    static constexpr int kernelProcessId { 4 };

    /**
     * @brief Returns the frame key of an address.
     *
     * The function makes the key when the address is new.
     * The key comes from the symbol, or from the module name when no symbol exists.
     *
     * @param processId  Process that holds the address. Use `kernelProcessId` for a kernel address.
     * @param address    Address to resolve.
     *
     * @return  Frame key. It is not valid when the address has no symbol and no module.
     */
    juce::Identifier getOrCreateFrameKey (int processId, juce::uint64 address);

    /**
     * @brief Loads the symbols of an image into a process.
     *
     * @param processId   Process that loads the image.
     * @param imageBase   Address where the image loads.
     * @param imageSize   Size of the image, in bytes.
     * @param imagePath   Path of the image in the NT form.
     */
    void addImage (int processId, juce::uint64 imageBase, juce::uint64 imageSize, const juce::String& imagePath);

    /**
     * @brief Loads the symbols of all device drivers into the kernel process.
     *
     * @return  Ok when the system lists the drivers. Otherwise the failure of `EnumDeviceDrivers`.
     */
    juce::Result addDrivers();
#endif

private:
    /** @brief Frame key of each address, for each process. */
    jam::HashMap<int, jam::HashMap<juce::uint64, juce::Identifier>> frameKeys;

#if JUCE_MAC or JUCE_LINUX
    /** @brief Symbol name of each image offset, for each image path. */
    jam::HashMap<juce::String, std::map<juce::uint64, juce::String>> imageSymbols;
#endif

#if JUCE_LINUX
    /**
     * @brief Returns the frame key of an address in a mapped region.
     *
     * @param regionText   Line of the process maps that describes the region.
     * @param regionStart  First address of the region.
     * @param address      Address to resolve.
     *
     * @return  Frame key of the symbol, or of the image name when no symbol precedes the address.
     *          The key is not valid when the region is not a file image or does not hold the address.
     */
    juce::Identifier getOrCreateImageFrameKey (const juce::String& regionText, juce::uint64 regionStart, juce::uint64 address);

    /** @brief Mapped regions of each process, keyed by the start address of the region. Each value is the maps line. */
    jam::HashMap<int, std::map<juce::uint64, juce::String>> regions;
#endif

#if JUCE_WINDOWS
    /**
     * @brief Returns the symbol handle of a process.
     *
     * The function initializes the symbol handler of the process when the handle is new.
     *
     * @param processId  Process of the handle.
     *
     * @return  Symbol handle of the process.
     */
    HANDLE getOrCreateSymbolHandle (int processId);

    /** @brief Guard of the symbol handler of each process. The guard cleans the handler up. */
    jam::HashMap<int, juce::ErasedScopeGuard> symbolHandles;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Symbolizer)
};

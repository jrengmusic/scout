#include "Symbolizer.h"

static juce::Identifier getFrameKey (const juce::String& frameName)
{
    static constexpr std::string_view allowedCharacters { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789:-" };
    static constexpr std::string_view argumentsOpening { "(" };
    static constexpr std::string_view disallowedReplacement { "-" };

    const auto name { frameName.upToLastOccurrenceOf (jam::Strings::fromView (argumentsOpening), false, false) };
    const auto disallowed { name.removeCharacters (jam::Strings::fromView (allowedCharacters)) };

    return juce::Identifier (name.replaceCharacters (disallowed, juce::String::repeatedString (jam::Strings::fromView (disallowedReplacement), disallowed.length())));
}

#if JUCE_MAC or JUCE_LINUX
#include <cxxabi.h>

static constexpr int kernelProcessId { 0 };

static juce::String getDemangledName (const juce::String& symbolName)
{
    int status {};
    const std::unique_ptr<char, void (*) (void*)> demangled { abi::__cxa_demangle (symbolName.toRawUTF8(), nullptr, nullptr, &status), std::free };

    return status == 0 ? juce::String::fromUTF8 (demangled.get()) : symbolName;
}

static juce::Identifier getFrameKey (const std::map<juce::uint64, juce::String>& symbols, juce::uint64 offset, const juce::String& imageName)
{
    const auto symbol { symbols.upper_bound (offset) };

    if (symbol != symbols.begin())
    {
        const auto& [symbolOffset, symbolName] = *std::prev (symbol);
        return getFrameKey (getDemangledName (symbolName));
    }

    return getFrameKey (imageName);
}
#endif

#if JUCE_MAC
#include <sys/sysctl.h>
#include <libproc.h>
#include <sys/proc_info.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>

static juce::File getKernelFile()
{
    static constexpr const char* versionName { "kern.version" };
    static constexpr std::string_view kernelsFolder { "/System/Library/Kernels" };
    static constexpr std::string_view armReleaseToken { "RELEASE_ARM64_" };
    static constexpr std::string_view releaseKernelPrefix { "kernel.release." };
    static constexpr std::string_view defaultKernelName { "kernel" };

    size_t size {};
    [[maybe_unused]] const auto sizeStatus { sysctlbyname (versionName, nullptr, &size, nullptr, 0) };

    jassert (sizeStatus == 0);

    juce::HeapBlock<char> version { size };
    [[maybe_unused]] const auto versionStatus { sysctlbyname (versionName, version.get(), &size, nullptr, 0) };

    jassert (versionStatus == 0);

    const auto kernelsDirectory { juce::File (jam::Strings::fromView (kernelsFolder)) };
    const auto versionText { juce::String (version.get()) };
    const auto armToken { jam::Strings::fromView (armReleaseToken) };
    const auto kernelName { versionText.contains (armToken)
                            ? jam::Strings::fromView (releaseKernelPrefix) + versionText.fromFirstOccurrenceOf (armToken, false, false).toLowerCase()
                            : jam::Strings::fromView (defaultKernelName) };
    const auto kernelFile { kernelsDirectory.getChildFile (kernelName) };

    jassert (kernelFile.existsAsFile());

    return kernelFile;
}

static const juce::String& getKernelImageName()
{
    static const auto imageName { getKernelFile().getFileName() };

    return imageName;
}

static const load_command* getCommand (const void* data, size_t size, const std::function<bool (const load_command*)>& isMatch)
{
    const auto* base { static_cast<const char*> (data) };
    const auto* header { static_cast<const mach_header_64*> (data) };

    if (size > sizeof (mach_header_64) and header->magic == MH_MAGIC_64)
    {
        auto offset { sizeof (mach_header_64) };

        for (uint32_t index { 0 }; index < header->ncmds; ++index)
        {
            const void* location { base + offset };
            const auto* command { static_cast<const load_command*> (location) };

            if (isMatch (command)) return command;

            offset += command->cmdsize;
        }
    }

    return nullptr;
}

static juce::uint64 getTextAddress (const load_command* command)
{
    const void* location { command };
    const auto* segment { static_cast<const segment_command_64*> (location) };

    if (command->cmd == LC_SEGMENT_64 and juce::String (segment->segname, sizeof (segment->segname)).compare (SEG_TEXT) == 0)
        return segment->vmaddr;

    return 0;
}

static juce::String getSymbolName (const char* text)
{
    const auto symbolName { juce::String::fromUTF8 (text) };

    return symbolName.startsWithChar ('_') ? symbolName.substring (1) : symbolName;
}

static std::map<juce::uint64, juce::String> getTableSymbols (const char* base, const symtab_command* table, juce::uint64 textAddress)
{
    std::map<juce::uint64, juce::String> symbols;
    const void* entriesLocation { base + table->symoff };
    const auto* entries { static_cast<const nlist_64*> (entriesLocation) };

    for (const auto* entry { entries }; entry != entries + table->nsyms; ++entry)
        if ((entry->n_type & (N_STAB | N_TYPE)) == N_SECT)
            symbols.try_emplace (entry->n_value - textAddress, getSymbolName (base + table->stroff + entry->n_un.n_strx));

    return symbols;
}

static std::map<juce::uint64, juce::String> getSymbols (const juce::MemoryBlock& image)
{
    const auto* tableCommand { getCommand (image.getData(), image.getSize(), [] (const load_command* command) { return command->cmd == LC_SYMTAB; }) };
    const auto* textCommand { getCommand (image.getData(), image.getSize(), [] (const load_command* command) { return getTextAddress (command) != 0; }) };

    if (tableCommand != nullptr)
    {
        const void* tableLocation { tableCommand };
        const juce::uint64 textAddress { textCommand != nullptr ? getTextAddress (textCommand) : 0 };

        return getTableSymbols (static_cast<const char*> (image.getData()), static_cast<const symtab_command*> (tableLocation), textAddress);
    }

    return {};
}

static bool isSymbolImage (const juce::File& imageFile)
{
    juce::FileInputStream stream { imageFile };

    return static_cast<juce::uint32> (stream.readInt()) == MH_MAGIC_64;
}

static const std::map<juce::uint64, juce::String>& getKernelSymbols()
{
    static const auto symbols ([]
    {
        juce::MemoryBlock image;
        getKernelFile().loadFileAsData (image);

        return getSymbols (image);
    }());

    return symbols;
}

static juce::uint64 getKernelTextAddress()
{
    static const auto textAddress { []
    {
        const juce::MemoryMappedFile image { getKernelFile(), juce::MemoryMappedFile::readOnly };
        const auto* textCommand { getCommand (image.getData(), image.getSize(), [] (const load_command* command) { return getTextAddress (command) != 0; }) };

        return textCommand != nullptr ? getTextAddress (textCommand) : juce::uint64 { 0 };
    }() };

    return textAddress;
}

juce::Identifier Symbolizer::getOrCreateUserFrameKey (int processId, juce::uint64 address)
{
    auto [keyEntry, isCreated] = frameKeys[processId].try_emplace (address);
    auto& [keyAddress, frameKey] = *keyEntry;
    proc_regionwithpathinfo region {};

    if (isCreated and proc_pidinfo (processId, PROC_PIDREGIONPATHINFO, address, &region, sizeof (region)) == static_cast<int> (sizeof (region)) and region.prp_vip.vip_path[0] != '\0')
    {
        const juce::File imageFile { juce::String::fromUTF8 (region.prp_vip.vip_path) };
        auto [symbolEntry, isSymbolsCreated] = imageSymbols.try_emplace (imageFile.getFullPathName());
        auto& [imagePath, symbols] = *symbolEntry;
        juce::MemoryBlock image;

        if (isSymbolsCreated and isSymbolImage (imageFile) and imageFile.loadFileAsData (image))
            symbols = getSymbols (image);

        frameKey = getFrameKey (symbols, address - region.prp_prinfo.pri_address, imageFile.getFileName());
    }

    return frameKey;
}
#endif

#if JUCE_LINUX
#include <elf.h>

static constexpr std::string_view rangeSeparator { "-" };

static juce::uint64 getFileOffset (const juce::MemoryBlock& image, juce::uint64 address)
{
    const auto* base { static_cast<const char*> (image.getData()) };
    const auto* header { static_cast<const Elf64_Ehdr*> (image.getData()) };

    for (Elf64_Half index { 0 }; index < header->e_phnum; ++index)
    {
        const void* location { base + header->e_phoff + index * header->e_phentsize };
        const auto* segment { static_cast<const Elf64_Phdr*> (location) };

        if (segment->p_type == PT_LOAD and address >= segment->p_vaddr and address < segment->p_vaddr + segment->p_memsz)
            return address - segment->p_vaddr + segment->p_offset;
    }

    return address;
}

static std::map<juce::uint64, juce::String> getSymbols (const juce::MemoryBlock& image)
{
    std::map<juce::uint64, juce::String> symbols;
    const auto* base { static_cast<const char*> (image.getData()) };
    const auto* header { static_cast<const Elf64_Ehdr*> (image.getData()) };

    if (image.getSize() > sizeof (Elf64_Ehdr) and std::memcmp (header->e_ident, ELFMAG, SELFMAG) == 0 and header->e_ident[EI_CLASS] == ELFCLASS64)
    {
        for (Elf64_Half index { 0 }; index < header->e_shnum; ++index)
        {
            const void* location { base + header->e_shoff + index * header->e_shentsize };
            const auto* section { static_cast<const Elf64_Shdr*> (location) };

            if (section->sh_type == SHT_SYMTAB or section->sh_type == SHT_DYNSYM)
            {
                const void* namesLocation { base + header->e_shoff + section->sh_link * header->e_shentsize };
                const void* entriesLocation { base + section->sh_offset };
                const auto* names { static_cast<const Elf64_Shdr*> (namesLocation) };
                const auto* entries { static_cast<const Elf64_Sym*> (entriesLocation) };

                for (const auto* entry { entries }; entry != entries + section->sh_size / sizeof (Elf64_Sym); ++entry)
                    if (ELF64_ST_TYPE (entry->st_info) == STT_FUNC and entry->st_shndx != SHN_UNDEF)
                        symbols.try_emplace (getFileOffset (image, entry->st_value), juce::String::fromUTF8 (base + names->sh_offset + entry->st_name));
            }
        }
    }

    return symbols;
}

static bool isSymbolImage (const juce::File& imageFile)
{
    juce::FileInputStream stream { imageFile };
    char magic[SELFMAG] {};

    stream.read (magic, SELFMAG);

    return std::memcmp (magic, ELFMAG, SELFMAG) == 0;
}

static const std::map<juce::uint64, juce::String>& getKernelSymbols()
{
    static const auto symbols ([]
    {
        static constexpr std::string_view kernelSymbolsPath { "/proc/kallsyms" };
        static constexpr std::string_view textTypes { "Tt" };
        static constexpr int addressField { 0 };
        static constexpr int typeField { 1 };
        static constexpr int nameField { 2 };

        std::map<juce::uint64, juce::String> kernelSymbols;

        for (const auto& line : juce::StringArray::fromLines (juce::File (jam::Strings::fromView (kernelSymbolsPath)).loadFileAsString()))
        {
            const auto fields (juce::StringArray::fromTokens (line, false));
            const auto address { static_cast<juce::uint64> (fields[addressField].getHexValue64()) };

            if (address != 0 and fields[typeField].containsAnyOf (jam::Strings::fromView (textTypes)))
                kernelSymbols.try_emplace (address, fields[nameField]);
        }

        return kernelSymbols;
    }());

    return symbols;
}

static std::map<juce::uint64, juce::String> getRegions (int processId)
{
    static constexpr std::string_view procPath { "/proc" };
    static constexpr std::string_view mapsName { "maps" };

    std::map<juce::uint64, juce::String> regions;
    const auto mapsFile { juce::File (jam::Strings::fromView (procPath)).getChildFile (juce::String (processId)).getChildFile (jam::Strings::fromView (mapsName)) };

    for (const auto& line : juce::StringArray::fromLines (mapsFile.loadFileAsString()))
        regions.try_emplace (static_cast<juce::uint64> (line.upToFirstOccurrenceOf (jam::Strings::fromView (rangeSeparator), false, false).getHexValue64()), line);

    return regions;
}

static const juce::String& getKernelImageName()
{
    static constexpr std::string_view kernelImageName { "vmlinux" };
    static const auto imageName { jam::Strings::fromView (kernelImageName) };

    return imageName;
}

juce::Identifier Symbolizer::getOrCreateUserFrameKey (int processId, juce::uint64 address)
{
    auto [keyEntry, isCreated] = frameKeys[processId].try_emplace (address);
    auto& [keyAddress, frameKey] = *keyEntry;

    if (isCreated)
    {
        auto [regionEntry, isRegionsCreated] = regions.try_emplace (processId);
        auto& [regionProcessId, processRegions] = *regionEntry;

        if (isRegionsCreated)
            processRegions = getRegions (processId);

        const auto region { processRegions.upper_bound (address) };

        if (region != processRegions.begin())
        {
            const auto& [regionStart, regionText] = *std::prev (region);
            frameKey = getOrCreateImageFrameKey (regionText, regionStart, address);
        }
    }

    return frameKey;
}

juce::Identifier Symbolizer::getOrCreateImageFrameKey (const juce::String& regionText, juce::uint64 regionStart, juce::uint64 address)
{
    static constexpr int rangeField { 0 };
    static constexpr int offsetField { 2 };
    static constexpr int imagePathField { 5 };
    static constexpr juce::juce_wchar rootCharacter { '/' };
    static constexpr std::string_view pathSeparator { " " };

    const auto fields (juce::StringArray::fromTokens (regionText, false));
    const auto regionEnd { static_cast<juce::uint64> (fields[rangeField].fromFirstOccurrenceOf (jam::Strings::fromView (rangeSeparator), false, false).getHexValue64()) };
    const auto regionOffset { static_cast<juce::uint64> (fields[offsetField].getHexValue64()) };
    juce::Identifier frameKey;

    if (address < regionEnd and fields[imagePathField].startsWithChar (rootCharacter))
    {
        const juce::File imageFile { fields.joinIntoString (jam::Strings::fromView (pathSeparator), imagePathField) };
        auto [symbolEntry, isSymbolsCreated] = imageSymbols.try_emplace (imageFile.getFullPathName());
        auto& [imagePath, symbols] = *symbolEntry;
        juce::MemoryBlock image;

        if (isSymbolsCreated and isSymbolImage (imageFile) and imageFile.loadFileAsData (image))
            symbols = getSymbols (image);

        frameKey = getFrameKey (symbols, address - regionStart + regionOffset, imageFile.getFileName());
    }

    return frameKey;
}
#endif

#if JUCE_MAC or JUCE_LINUX
juce::Identifier Symbolizer::getOrCreateKernelFrameKey (juce::uint64 address)
{
    auto [keyEntry, isCreated] = frameKeys[kernelProcessId].try_emplace (address);
    auto& [keyAddress, frameKey] = *keyEntry;

    if (isCreated)
    {
#if JUCE_MAC
        frameKey = getFrameKey (getKernelSymbols(), address - getKernelTextAddress(), getKernelImageName());
#else
        frameKey = getFrameKey (getKernelSymbols(), address, getKernelImageName());
#endif
    }

    return frameKey;
}
#endif

#if JUCE_WINDOWS
#include "generated/Generated.h"
#include <dbghelp.h>
#include <psapi.h>

#pragma comment (lib, "psapi.lib")

juce::Result getResult (const char* name, ULONG status)
{
    if (status == ERROR_SUCCESS)
        return juce::Result::ok();

    return juce::Result::fail (juce::String (name) + Id::diagnosticSeparator + juce::String (std::system_category().message (static_cast<int> (status))));
}

static juce::String getDosPath (const juce::String& ntPath)
{
    static const auto prefixes ([]
    {
        static constexpr std::string_view deviceNamespace { "??" };
        static constexpr std::string_view systemRootName { "SystemRoot" };

        const auto separator { juce::File::getSeparatorString() };
        juce::Array<juce::File> roots;
        juce::StringPairArray drivePrefixes;

        juce::File::findFileSystemRoots (roots);
        drivePrefixes.set (separator + jam::Strings::fromView (deviceNamespace) + separator, juce::String());
        drivePrefixes.set (separator + jam::Strings::fromView (systemRootName) + separator, juce::File::getSpecialLocation (juce::File::windowsSystemDirectory).getParentDirectory().getFullPathName() + separator);

        for (const auto& root : roots)
        {
            wchar_t target[MAX_PATH] {};

            if (QueryDosDeviceW (root.getFullPathName().upToFirstOccurrenceOf (separator, false, false).toWideCharPointer(), target, MAX_PATH) > 0)
                drivePrefixes.set (juce::String (target) + separator, root.getFullPathName());
        }

        return drivePrefixes;
    }());

    for (const auto& prefix : prefixes.getAllKeys())
        if (ntPath.startsWith (prefix)) return prefixes[prefix] + ntPath.substring (prefix.length());

    return ntPath;
}

static juce::Identifier getFrameKey (HANDLE symbolHandle, juce::uint64 address)
{
    juce::MemoryBlock symbolBlock { sizeof (SYMBOL_INFOW) + MAX_SYM_NAME * sizeof (wchar_t), true };
    auto* symbol { static_cast<SYMBOL_INFOW*> (symbolBlock.getData()) };
    IMAGEHLP_MODULEW64 moduleInfo {};
    DWORD64 displacement {};

    symbol->SizeOfStruct = sizeof (SYMBOL_INFOW);
    symbol->MaxNameLen = MAX_SYM_NAME;
    moduleInfo.SizeOfStruct = sizeof (moduleInfo);

    if (SymFromAddrW (symbolHandle, address, &displacement, symbol))
        return getFrameKey (juce::String (symbol->Name));

    if (SymGetModuleInfoW64 (symbolHandle, address, &moduleInfo))
        return getFrameKey (juce::File (juce::String (moduleInfo.ImageName)).getFileName());

    return juce::Identifier();
}

juce::Result Symbolizer::addDrivers()
{
    DWORD neededBytes {};
    EnumDeviceDrivers (nullptr, 0, &neededBytes);

    juce::HeapBlock<LPVOID> bases { neededBytes / sizeof (LPVOID) };
    DWORD returnedBytes {};
    const auto isEnumerated { EnumDeviceDrivers (bases.get(), neededBytes, &returnedBytes) };
    const auto kernelHandle { getOrCreateSymbolHandle (kernelProcessId) };

    if (isEnumerated)
    {
        for (DWORD index { 0 }; index < returnedBytes / sizeof (LPVOID); ++index)
        {
            wchar_t path[MAX_PATH] {};

            [[maybe_unused]] const auto pathLength { GetDeviceDriverFileNameW (bases[index], path, MAX_PATH) };

            jassert (pathLength > 0);

            [[maybe_unused]] const auto loadedBase { SymLoadModuleExW (kernelHandle, nullptr, getDosPath (juce::String (path)).toWideCharPointer(), nullptr, reinterpret_cast<DWORD64> (bases[index]), 0, nullptr, 0) };

            jassert (loadedBase != 0 or GetLastError() == ERROR_SUCCESS);
        }
    }

    return getResult ("EnumDeviceDrivers", isEnumerated ? ERROR_SUCCESS : GetLastError());
}

void Symbolizer::addImage (int processId, juce::uint64 imageBase, juce::uint64 imageSize, const juce::String& imagePath)
{
    const auto dosPath { getDosPath (imagePath) };
    [[maybe_unused]] const auto loadedBase { SymLoadModuleExW (getOrCreateSymbolHandle (processId), nullptr, dosPath.toWideCharPointer(), nullptr, imageBase, static_cast<DWORD> (imageSize), nullptr, 0) };

    jassert (loadedBase != 0 or GetLastError() == ERROR_SUCCESS);
}

HANDLE Symbolizer::getOrCreateSymbolHandle (int processId)
{
    const auto symbolHandle { reinterpret_cast<HANDLE> (static_cast<uintptr_t> (processId)) };
    const auto [guardEntry, isCreated] = symbolHandles.try_emplace (processId, [symbolHandle] { SymCleanup (symbolHandle); });

    if (isCreated)
    {
        SymSetOptions (SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

        [[maybe_unused]] const auto isInitialized { SymInitializeW (symbolHandle, nullptr, FALSE) };

        jassert (isInitialized);
    }

    return symbolHandle;
}

juce::Identifier Symbolizer::getOrCreateFrameKey (int processId, juce::uint64 address)
{
    auto [keyEntry, isCreated] = frameKeys[processId].try_emplace (address);
    auto& [keyAddress, frameKey] = *keyEntry;

    if (isCreated)
        frameKey = getFrameKey (getOrCreateSymbolHandle (processId), address);

    return frameKey;
}
#endif

Symbolizer::Symbolizer() = default;

Symbolizer::~Symbolizer() = default;

#include <windows.h>
#include <psapi.h>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <fstream>

struct ExportInfo
{
    std::string moduleName;
    std::string funcName;
};

std::string GetProcessName()
{
    char path[MAX_PATH] = { 0 };
    GetModuleBaseNameA(GetCurrentProcess(), NULL, path, MAX_PATH);
    return std::string(path);
}

std::string GetModuleName(HMODULE hMod)
{
    char path[MAX_PATH] = { 0 };
    GetModuleBaseNameA(GetCurrentProcess(), hMod, path, MAX_PATH);
    return std::string(path);
}

void AddExportsFromModule(HMODULE hMod, std::unordered_map<uintptr_t, ExportInfo>& addrToInfo)
{
    MODULEINFO mi = { 0 };
    if (!GetModuleInformation(GetCurrentProcess(), hMod, &mi, sizeof(mi)))
        return;

    BYTE* base = reinterpret_cast<BYTE*>(mi.lpBaseOfDll);
    std::string moduleName = GetModuleName(hMod);

    IMAGE_DOS_HEADER* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE)
        return;

    IMAGE_NT_HEADERS* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (!nt || nt->Signature != IMAGE_NT_SIGNATURE)
        return;

    const auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!dir.VirtualAddress || !dir.Size)
        return;

    IMAGE_EXPORT_DIRECTORY* exp = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(base + dir.VirtualAddress);
    if (!exp)
        return;

    DWORD* funcs = reinterpret_cast<DWORD*>(base + exp->AddressOfFunctions);
    DWORD* names = reinterpret_cast<DWORD*>(base + exp->AddressOfNames);
    WORD* ords = reinterpret_cast<WORD*>(base + exp->AddressOfNameOrdinals);

    std::vector<std::string> perIndex(exp->NumberOfFunctions);

    for (DWORD i = 0; i < exp->NumberOfNames; ++i)
    {
        DWORD idx = ords[i];
        if (idx < exp->NumberOfFunctions)
        {
            const char* nm = reinterpret_cast<const char*>(base + names[i]);
            if (nm && *nm)
                perIndex[idx] = nm;
        }
    }

    DWORD expStart = dir.VirtualAddress;
    DWORD expEnd = dir.VirtualAddress + dir.Size;

    for (DWORD i = 0; i < exp->NumberOfFunctions; ++i)
    {
        DWORD rva = funcs[i];
        if (!rva)
            continue;

        if (rva >= expStart && rva < expEnd)
            continue;

        uintptr_t addr = reinterpret_cast<uintptr_t>(base + rva);

        std::string funcName = perIndex[i];
        if (funcName.empty())
        {
            char tmp[64];
            sprintf_s(tmp, "ord%u", exp->Base + i);
            funcName = tmp;
        }

        if (addrToInfo.find(addr) == addrToInfo.end())
        {
            addrToInfo[addr] = { moduleName, funcName };
        }
    }
}

void CollectAllExports(std::unordered_map<uintptr_t, ExportInfo>& addrToInfo)
{
    HMODULE mods[1024] = { 0 };
    DWORD needed = 0;

    if (!EnumProcessModules(GetCurrentProcess(), mods, sizeof(mods), &needed))
        return;

    size_t count = needed / sizeof(HMODULE);
    for (size_t i = 0; i < count; ++i)
        AddExportsFromModule(mods[i], addrToInfo);
}

void DumpToFile(const std::unordered_map<uintptr_t, ExportInfo>& addrToInfo)
{
    std::string filename = GetProcessName() + "_funcs.txt";
    std::ofstream file(filename);

    if (!file.is_open())
        return;

    for (const auto& pair : addrToInfo)
    {
        char buffer[512];
        sprintf_s(buffer, "0x%llx\t%s\t%s\n",
            (unsigned long long)pair.first,
            pair.second.moduleName.c_str(),
            pair.second.funcName.c_str());
        file << buffer;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        std::unordered_map<uintptr_t, ExportInfo> exports;
        CollectAllExports(exports);
        DumpToFile(exports);
    }
    return TRUE;
}
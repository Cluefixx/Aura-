#pragma once

#include <Windows.h>
#include <winternl.h>

typedef NTSTATUS(NTAPI* NtQueryVirtualMemory_t)(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    DWORD MemoryInformationClass,
    PVOID MemoryInformation,
    SIZE_T MemoryInformationLength,
    PSIZE_T ReturnLength);

typedef NTSTATUS(NTAPI* NtProtectVirtualMemory_t)(
    HANDLE ProcessHandle,
    PVOID* BaseAddress,
    PSIZE_T RegionSize,
    ULONG NewProtect,
    PULONG OldProtect);

namespace HyperionBypass {

    inline uintptr_t g_moduleBase = 0;
    inline SIZE_T    g_moduleSize = 0;

    inline NtQueryVirtualMemory_t  g_originalNtQVM = nullptr;
    inline BYTE                    g_originalQvmBytes[32] = {};
    inline uintptr_t               g_ntQvmAddr = 0;
    inline size_t                  g_qvmHookLen = 0;

    inline NtProtectVirtualMemory_t g_originalNtPVM = nullptr;
    inline BYTE                     g_originalPvmBytes[32] = {};
    inline uintptr_t                g_ntPvmAddr = 0;
    inline size_t                   g_pvmHookLen = 0;

    inline BYTE* g_qvmTrampoline = nullptr;
    inline BYTE* g_pvmTrampoline = nullptr;

    inline bool IsAddressInModule(PVOID addr)
    {
        uintptr_t a = (uintptr_t)addr;
        return a >= g_moduleBase && a < (g_moduleBase + g_moduleSize);
    }

    inline size_t CalcSyscallStubLen(uintptr_t funcAddr)
    {
        BYTE* p = (BYTE*)funcAddr;
        size_t len = 0;
        for (int i = 0; i < 32 && len < 14; i++)
        {
            BYTE b = p[len];
            if ((b == 0x48 || b == 0x49) && (p[len + 1] >= 0xB8 && p[len + 1] <= 0xBF)) { len += 10; continue; }
            if (b == 0x4C && p[len + 1] == 0x8B && p[len + 2] == 0xD1) { len += 3;  continue; }
            if (b == 0xB8) { len += 5;  continue; }
            if (b == 0xF6 && p[len + 1] == 0x04 && p[len + 2] == 0x25) { len += 8;  continue; }
            if (b == 0x75) { len += 2;  continue; }
            if (b == 0xCD) { len += 2;  continue; }
            if (b == 0x0F && p[len + 1] == 0x05) { len += 2;  continue; }
            if (b == 0xC3) { len += 1;  break; }
            if (b == 0x90) { len += 1;  continue; }
            len += 1;
        }
        return len >= 14 ? len : 14;
    }

    NTSTATUS NTAPI HookedNtQueryVirtualMemory(
        HANDLE ProcessHandle,
        PVOID BaseAddress,
        DWORD MemoryInformationClass,
        PVOID MemoryInformation,
        SIZE_T MemoryInformationLength,
        PSIZE_T ReturnLength);

    NTSTATUS NTAPI HookedNtProtectVirtualMemory(
        HANDLE ProcessHandle,
        PVOID* BaseAddress,
        PSIZE_T RegionSize,
        ULONG NewProtect,
        PULONG OldProtect);

    bool InstallHook();
    void RemoveHook();

    LONG WINAPI CFGHook(PEXCEPTION_POINTERS ExceptionInfo);

}

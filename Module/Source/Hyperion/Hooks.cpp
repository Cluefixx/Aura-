#include <Hyperion/Hooks.hpp>
#include <Roblox/Offsets.hpp>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#ifndef MemoryBasicInformation
#define MemoryBasicInformation 0
#endif

namespace HyperionBypass {

    static BYTE* BuildTrampoline(uintptr_t originalAddr, BYTE* savedBytes, size_t hookLen)
    {
        BYTE* trampoline = static_cast<BYTE*>(VirtualAlloc(nullptr, 4096,
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
        if (!trampoline)
            return nullptr;

        memcpy(trampoline, savedBytes, hookLen);

        size_t off = hookLen;
        trampoline[off] = 0xFF;
        trampoline[off + 1] = 0x25;
        *reinterpret_cast<DWORD*>(trampoline + off + 2) = 0;
        uintptr_t returnAddr = originalAddr + hookLen;
        memcpy(trampoline + off + 6, &returnAddr, 8);

        return trampoline;
    }

    static bool WriteHook(uintptr_t target, uintptr_t hookFn, size_t hookLen)
    {
        BYTE jmp[14];
        jmp[0] = 0xFF;
        jmp[1] = 0x25;
        *reinterpret_cast<DWORD*>(jmp + 2) = 0;
        memcpy(jmp + 6, &hookFn, 8);

        BYTE patch[32] = {};
        memcpy(patch, jmp, 14);
        for (size_t i = 14; i < hookLen; i++)
            patch[i] = 0x90;

        DWORD oldProt = 0;
        VirtualProtect(reinterpret_cast<PVOID>(target), hookLen, PAGE_EXECUTE_READWRITE, &oldProt);
        memcpy(reinterpret_cast<void*>(target), patch, hookLen);
        VirtualProtect(reinterpret_cast<PVOID>(target), hookLen, oldProt, &oldProt);
        return true;
    }

    NTSTATUS NTAPI HookedNtQueryVirtualMemory(
        HANDLE ProcessHandle,
        PVOID BaseAddress,
        DWORD MemoryInformationClass,
        PVOID MemoryInformation,
        SIZE_T MemoryInformationLength,
        PSIZE_T ReturnLength)
    {
        NTSTATUS result = g_originalNtQVM(ProcessHandle, BaseAddress,
            MemoryInformationClass, MemoryInformation, MemoryInformationLength, ReturnLength);

        if (NT_SUCCESS(result) && MemoryInformationClass == MemoryBasicInformation && MemoryInformation)
        {
            if (ProcessHandle == GetCurrentProcess() || ProcessHandle == reinterpret_cast<HANDLE>(-1))
            {
                auto* mbi = static_cast<PMEMORY_BASIC_INFORMATION>(MemoryInformation);
                bool hideBase = IsAddressInModule(mbi->BaseAddress);
                bool hideAlloc = mbi->AllocationBase && IsAddressInModule(mbi->AllocationBase);
                if (hideBase || hideAlloc)
                {
                    mbi->State = MEM_FREE;
                    mbi->Protect = 0;
                    mbi->AllocationProtect = 0;
                    mbi->Type = 0;
                    mbi->AllocationBase = nullptr;
                    mbi->RegionSize = 0x1000;
                }
            }
        }
        return result;
    }

    NTSTATUS NTAPI HookedNtProtectVirtualMemory(
        HANDLE ProcessHandle,
        PVOID* BaseAddress,
        PSIZE_T RegionSize,
        ULONG NewProtect,
        PULONG OldProtect)
    {
        if (BaseAddress && *BaseAddress &&
            (ProcessHandle == GetCurrentProcess() || ProcessHandle == reinterpret_cast<HANDLE>(-1)))
        {
            if (IsAddressInModule(*BaseAddress))
            {
                if (OldProtect) *OldProtect = PAGE_EXECUTE_READWRITE;
                return 0;
            }
        }
        return g_originalNtPVM(ProcessHandle, BaseAddress, RegionSize, NewProtect, OldProtect);
    }

    bool InstallHook()
    {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (!ntdll)
            return false;

        auto ntQvm = reinterpret_cast<NtQueryVirtualMemory_t>(GetProcAddress(ntdll, "NtQueryVirtualMemory"));
        if (!ntQvm)
            return false;

        g_ntQvmAddr = reinterpret_cast<uintptr_t>(ntQvm);
        g_qvmHookLen = CalcSyscallStubLen(g_ntQvmAddr);
        if (g_qvmHookLen < 14) g_qvmHookLen = 14;
        if (g_qvmHookLen > 32) g_qvmHookLen = 32;

        memcpy(g_originalQvmBytes, reinterpret_cast<void*>(g_ntQvmAddr), g_qvmHookLen);
        g_qvmTrampoline = BuildTrampoline(g_ntQvmAddr, g_originalQvmBytes, g_qvmHookLen);
        if (!g_qvmTrampoline)
            return false;

        g_originalNtQVM = reinterpret_cast<NtQueryVirtualMemory_t>(g_qvmTrampoline);
        WriteHook(g_ntQvmAddr, reinterpret_cast<uintptr_t>(&HookedNtQueryVirtualMemory), g_qvmHookLen);

        auto ntPvm = reinterpret_cast<NtProtectVirtualMemory_t>(GetProcAddress(ntdll, "NtProtectVirtualMemory"));
        if (!ntPvm)
            return false;

        g_ntPvmAddr = reinterpret_cast<uintptr_t>(ntPvm);
        g_pvmHookLen = CalcSyscallStubLen(g_ntPvmAddr);
        if (g_pvmHookLen < 14) g_pvmHookLen = 14;
        if (g_pvmHookLen > 32) g_pvmHookLen = 32;

        memcpy(g_originalPvmBytes, reinterpret_cast<void*>(g_ntPvmAddr), g_pvmHookLen);
        g_pvmTrampoline = BuildTrampoline(g_ntPvmAddr, g_originalPvmBytes, g_pvmHookLen);
        if (!g_pvmTrampoline)
            return false;

        g_originalNtPVM = reinterpret_cast<NtProtectVirtualMemory_t>(g_pvmTrampoline);
        WriteHook(g_ntPvmAddr, reinterpret_cast<uintptr_t>(&HookedNtProtectVirtualMemory), g_pvmHookLen);

        return true;
    }

    void RemoveHook()
    {
        if (g_ntQvmAddr && g_originalQvmBytes[0] != 0)
        {
            DWORD oldProt = 0;
            VirtualProtect(reinterpret_cast<PVOID>(g_ntQvmAddr), g_qvmHookLen, PAGE_EXECUTE_READWRITE, &oldProt);
            memcpy(reinterpret_cast<void*>(g_ntQvmAddr), g_originalQvmBytes, g_qvmHookLen);
            VirtualProtect(reinterpret_cast<PVOID>(g_ntQvmAddr), g_qvmHookLen, oldProt, &oldProt);
        }
        if (g_qvmTrampoline) { VirtualFree(g_qvmTrampoline, 0, MEM_RELEASE); g_qvmTrampoline = nullptr; }

        if (g_ntPvmAddr && g_originalPvmBytes[0] != 0)
        {
            DWORD oldProt = 0;
            VirtualProtect(reinterpret_cast<PVOID>(g_ntPvmAddr), g_pvmHookLen, PAGE_EXECUTE_READWRITE, &oldProt);
            memcpy(reinterpret_cast<void*>(g_ntPvmAddr), g_originalPvmBytes, g_pvmHookLen);
            VirtualProtect(reinterpret_cast<PVOID>(g_ntPvmAddr), g_pvmHookLen, oldProt, &oldProt);
        }
        if (g_pvmTrampoline) { VirtualFree(g_pvmTrampoline, 0, MEM_RELEASE); g_pvmTrampoline = nullptr; }
    }

    LONG WINAPI CFGHook(PEXCEPTION_POINTERS ExceptionInfo)
    {
        const uintptr_t exception_address =
            reinterpret_cast<uintptr_t>(ExceptionInfo->ExceptionRecord->ExceptionAddress);
        if (exception_address >= Offsets::Hyperion::ControlFlowGuardian &&
            exception_address < Offsets::Hyperion::ControlFlowGuardian + 0x100)
        {
            ExceptionInfo->ContextRecord->Rip = Offsets::Hyperion::ValidateCandidate;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }

}

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <KtmW32.h>
#include <winternl.h>

#include <cstdint>
#include <vector>
#include <string>

#pragma comment(lib, "KtmW32.lib")

extern HMODULE g_ntdll;

using fn_ntcreatesection = NTSTATUS(NTAPI*)(
    PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES,
    PLARGE_INTEGER, ULONG, ULONG, HANDLE);

using fn_ntmapviewofsection = NTSTATUS(NTAPI*)(
    HANDLE, HANDLE, PVOID*, ULONG_PTR, SIZE_T,
    PLARGE_INTEGER, PSIZE_T, DWORD, ULONG, ULONG);

using fn_ntunmapviewofsection = NTSTATUS(NTAPI*)(HANDLE, PVOID);

using fn_ntwritevirtualmemory = NTSTATUS(NTAPI*)(
    HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);

using fn_ntquerysysteminformation = NTSTATUS(NTAPI*)(
    ULONG, PVOID, ULONG, PULONG);

using fn_ntqueryobject = NTSTATUS(NTAPI*)(
    HANDLE, ULONG, PVOID, ULONG, PULONG);

using fn_zwsetiocomplete = NTSTATUS(NTAPI*)(
    HANDLE, PVOID, PVOID, NTSTATUS, ULONG_PTR);

struct rbxctx {
    HWND   hwnd;
    DWORD  pid;
    HANDLE process;
    DWORD  thread_id;
};


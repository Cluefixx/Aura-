#include "rbx.h"
#include "config.h"

HMODULE g_ntdll = nullptr;

rbxctx* rbxget() {
    auto ctx = new rbxctx{};

    g_ntdll = GetModuleHandleA("ntdll.dll");

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return ctx;

    PROCESSENTRY32 pe{};
    pe.dwSize = sizeof(pe);
    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, "RobloxPlayerBeta.exe") == 0) {
                ctx->pid = pe.th32ProcessID;
                break;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);

    if (ctx->pid == 0)
        return ctx;

    ctx->hwnd      = FindWindowA(nullptr, nullptr);
    ctx->process   = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ctx->pid);
    if (ctx->process)
        ctx->thread_id = GetThreadId(ctx->process);

    return ctx;
}

static void iterthreads(const rbxctx* ctx, bool resume) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return;

    THREADENTRY32 te{};
    te.dwSize = sizeof(te);
    if (!Thread32First(snap, &te)) {
        CloseHandle(snap);
        return;
    }
    do {
        if (te.th32OwnerProcessID != ctx->pid) continue;
        HANDLE t = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
        if (!t) continue;
        if (resume) ResumeThread(t);
        else        SuspendThread(t);
        CloseHandle(t);
    } while (Thread32Next(snap, &te));
    CloseHandle(snap);
}

void susthread(const rbxctx* ctx)   { iterthreads(ctx, false); }
void resumethread(const rbxctx* ctx) { iterthreads(ctx, true);  }

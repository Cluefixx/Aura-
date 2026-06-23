#include "inject.h"
#include "loader.h"

#pragma pack(push, 1)
struct ldr_param {
    uintptr_t image_base;
    void*     load_library;
    void*     get_proc_address;
    uint64_t  status;
};

struct trampoline {
    uint8_t   mov_rax[2];
    uintptr_t loader_addr;
    uint8_t   mov_rcx[2];
    uintptr_t param_addr;
    uint8_t   jmp_rax[2];
};
#pragma pack(pop)
static_assert(sizeof(trampoline) == 22, "trampoline size mismatch");

void poolparty(HANDLE proc, const void* tramp, SIZE_T tramp_size) {
    auto ntqsi = (fn_ntquerysysteminformation) GetProcAddress(g_ntdll, "NtQuerySystemInformation");
    auto ntqo  = (fn_ntqueryobject)            GetProcAddress(g_ntdll, "NtQueryObject");
    auto zwsic = (fn_zwsetiocomplete)          GetProcAddress(g_ntdll, "ZwSetIoCompletion");
    if (!ntqsi || !ntqo || !zwsic) return;

    auto handle_buf = (uint8_t*)operator new(100000);
    NTSTATUS st = ntqsi(51, handle_buf, 100000, nullptr);
    if (st < 0) { operator delete(handle_buf); return; }

    uint64_t count  = *(uint64_t*)handle_buf;
    auto name_buf   = (uint8_t*)operator new(10000);
    ZeroMemory(name_buf, 10000);

    HANDLE stolen = nullptr;
    for (uint64_t i = 0; i < count && !stolen; ++i) {
        HANDLE dup = nullptr;
        if (!DuplicateHandle(proc, (HANDLE)(ULONG_PTR)i, GetCurrentProcess(),
                &dup, 0, FALSE, DUPLICATE_SAME_ACCESS))
            continue;
        if (ntqo(dup, 2, name_buf, 10000, nullptr) >= 0) {
            auto buf = *(const wchar_t**)(name_buf + 8);
            if (buf && wcscmp(buf, L"IoCompletion") == 0) {
                stolen = dup;
                break;
            }
        }
        CloseHandle(dup);
    }
    operator delete(name_buf);
    operator delete(handle_buf);
    if (!stolen) return;

    LPVOID tramp_addr = VirtualAllocEx(proc, nullptr, tramp_size,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!tramp_addr) { CloseHandle(stolen); return; }

    SIZE_T wrote = 0;
    if (!WriteProcessMemory(proc, tramp_addr, tramp, tramp_size, &wrote) || wrote != tramp_size) {
        VirtualFreeEx(proc, tramp_addr, 0, MEM_RELEASE);
        CloseHandle(stolen);
        return;
    }

    PVOID landing = nullptr;
    MEMORY_BASIC_INFORMATION mbi{};
    uint8_t scratch[0x1000];
    SIZE_T q = VirtualQueryEx(proc, nullptr, &mbi, sizeof(mbi));
    while (q && !landing) {
        if (mbi.State   == MEM_COMMIT   &&
            mbi.Protect == PAGE_READWRITE &&
            mbi.RegionSize >= 0x48) {
            SIZE_T off = 0;
            while (off + 0x48 <= mbi.RegionSize) {
                SIZE_T cap    = mbi.RegionSize - off - 0x47;
                SIZE_T toread = (cap > 0x1000) ? 0x1000 : cap;
                SIZE_T nread  = 0;
                if (!ReadProcessMemory(proc,
                        (LPCVOID)((uintptr_t)mbi.BaseAddress + off),
                        scratch, toread, &nread)) break;
                if (nread < 0x48) break;
                for (SIZE_T j = 0; j + 0x48 <= nread; ++j) {
                    bool allzero = true;
                    for (SIZE_T k = 0; k < 0x48; ++k) {
                        if (scratch[j + k]) { allzero = false; break; }
                    }
                    if (allzero) {
                        landing = (PVOID)((uintptr_t)mbi.BaseAddress + off + j);
                        goto found;
                    }
                }
                off += 0x1000;
            }
        }
        q = VirtualQueryEx(proc,
            (LPCVOID)((uintptr_t)mbi.BaseAddress + mbi.RegionSize),
            &mbi, sizeof(mbi));
    }
found:
    if (!landing) { CloseHandle(stolen); return; }

    uint64_t packet[9] = {};
    packet[7] = (uint64_t)(uintptr_t)tramp_addr;
    WriteProcessMemory(proc, landing, packet, sizeof(packet), nullptr);
    zwsic(stolen, landing, nullptr, 0, 0);
    CloseHandle(stolen);
}

void trIGGER(const rbxctx* ctx, uintptr_t image_base) {
    auto ntcs  = (fn_ntcreatesection)    GetProcAddress(g_ntdll, "NtCreateSection");
    auto ntmvs = (fn_ntmapviewofsection) GetProcAddress(g_ntdll, "NtMapViewOfSection");
    if (!ntcs || !ntmvs) return;

    HANDLE sec = nullptr;
    LARGE_INTEGER max_size{};
    max_size.QuadPart = 0x1020;
    NTSTATUS st = ntcs(&sec, 0xE, nullptr,
        &max_size, PAGE_EXECUTE_READWRITE, SEC_COMMIT, nullptr);
    if (st < 0) return;

    PVOID view   = nullptr;
    SIZE_T viewsz = 0x1020;
    st = ntmvs(sec, ctx->process, &view, 0, 0, nullptr,
        &viewsz, 2, 0, PAGE_EXECUTE_READWRITE);
    if (st < 0) { CloseHandle(sec); return; }

    PVOID param_addr = (PVOID)((uintptr_t)view + 0x1000);

    WriteProcessMemory(ctx->process, view, loader, 0x1000, nullptr);

    HMODULE k32   = GetModuleHandleA("kernel32.dll");
    ldr_param param{};
    param.image_base      = image_base;
    param.load_library    = GetProcAddress(k32, "LoadLibraryA");
    param.get_proc_address = GetProcAddress(k32, "GetProcAddress");
    param.status          = 0;
    WriteProcessMemory(ctx->process, param_addr, &param, 0x20, nullptr);

    trampoline tr{};
    tr.mov_rax[0]  = 0x48; tr.mov_rax[1]  = 0xB8;
    tr.loader_addr = (uintptr_t)view;
    tr.mov_rcx[0]  = 0x48; tr.mov_rcx[1]  = 0xB9;
    tr.param_addr  = (uintptr_t)param_addr;
    tr.jmp_rax[0]  = 0xFF; tr.jmp_rax[1]  = 0xE0;

    poolparty(ctx->process, &tr, sizeof(tr));

    uint64_t readback[4] = {};
    while (true) {
        Sleep(500);
        ReadProcessMemory(ctx->process, param_addr, readback, 0x20, nullptr);
        if ((uint32_t)readback[3] == 5) break;
    }

    CloseHandle(sec);
}

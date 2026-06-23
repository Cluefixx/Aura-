#include "mapper.h"
#include "rbx.h"

#include <cstdlib>
#include <map>

namespace {

struct chunk {
    uintptr_t base;
    SIZE_T    size;
};

using chunk_map = std::map<int, chunk>;

static std::string getcarrier() { // carrier is the dll if you wonder
    WIN32_FIND_DATAA fd{};
    HANDLE h = FindFirstFileA("C:\\Windows\\System32\\*.dll", &fd);
    if (h == INVALID_HANDLE_VALUE) return {};

    auto ntcs   = (fn_ntcreatesection)      GetProcAddress(g_ntdll, "NtCreateSection");
    auto ntmvs  = (fn_ntmapviewofsection)   GetProcAddress(g_ntdll, "NtMapViewOfSection");
    auto ntumvs = (fn_ntunmapviewofsection) GetProcAddress(g_ntdll, "NtUnmapViewOfSection");

    std::vector<std::string> candidates;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        ULONGLONG sz = ((ULONGLONG)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
        if (sz == 0 || sz > 0x10000) continue;

        std::string path = std::string("C:\\Windows\\System32\\") + fd.cFileName;
        HANDLE file = CreateFileA(path.c_str(),
            GENERIC_READ | GENERIC_EXECUTE, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) continue;

        HANDLE sec = nullptr;
        NTSTATUS st = ntcs(&sec, SECTION_ALL_ACCESS, nullptr,
            nullptr, PAGE_READONLY, SEC_IMAGE, file);
        CloseHandle(file);
        if (st < 0) continue;

        PVOID  view     = nullptr;
        SIZE_T viewsize = 0;
        st = ntmvs(sec, GetCurrentProcess(), &view, 0, 0,
            nullptr, &viewsize, 2, 0, PAGE_READONLY);
        CloseHandle(sec);
        if (st < 0) continue;

        bool keep = (viewsize == 0x10000);
        ntumvs(GetCurrentProcess(), view);
        if (keep) candidates.push_back(path);

    } while (FindNextFileA(h, &fd));
    FindClose(h);

    if (candidates.empty()) return {};
    return candidates[(size_t)rand() % candidates.size()];
}

static uintptr_t buildcarrier(HANDLE proc, 
                                      const std::string& carrier_path,
                                      SIZE_T payload_size,
                                      chunk_map& chunks) { // carrier is a dll if you wonder
    char tmp_dir[MAX_PATH]  = {};
    char tmp_file[MAX_PATH] = {};
    GetTempPathA(MAX_PATH, tmp_dir);
    GetTempFileNameA(tmp_dir, "BYF", 0, tmp_file);

    if (!CopyFileA(carrier_path.c_str(), tmp_file, FALSE)) {
        DeleteFileA(tmp_file);
        return 0;
    }

    HANDLE file = CreateFileA(tmp_file,
        GENERIC_READ | GENERIC_EXECUTE,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tmp_file);
        return 0;
    }

    int chunk_count = (int)(((payload_size + 0xFFFF) >> 16) + 1);
    const SIZE_T kchunk = 0x10000;

    MEMORY_BASIC_INFORMATION mbi{};
    uintptr_t carrier_base = 0;
    SIZE_T q = VirtualQueryEx(proc, (LPCVOID)0x10000, &mbi, sizeof(mbi));
    while (q) {
        if (mbi.State == MEM_FREE) {
            uintptr_t aligned = ((uintptr_t)mbi.BaseAddress + 0xFFFF) & ~(uintptr_t)0xFFFF;
            SIZE_T need = (SIZE_T)chunk_count * kchunk;
            if (mbi.RegionSize >= need &&
                aligned + need <= (uintptr_t)mbi.BaseAddress + mbi.RegionSize) {
                carrier_base = aligned;
                break;
            }
        }
        if ((uintptr_t)mbi.BaseAddress + mbi.RegionSize >= 0x7FFFFFFFFFFEULL) break;
        q = VirtualQueryEx(proc,
            (LPCVOID)((uintptr_t)mbi.BaseAddress + mbi.RegionSize), &mbi, sizeof(mbi));
    }

    if (!carrier_base) {
        CloseHandle(file);
        DeleteFileA(tmp_file);
        return 0;
    }

    auto ntcs  = (fn_ntcreatesection)      GetProcAddress(g_ntdll, "NtCreateSection");
    auto ntmvs = (fn_ntmapviewofsection)   GetProcAddress(g_ntdll, "NtMapViewOfSection");
    auto ntwvm = (fn_ntwritevirtualmemory) GetProcAddress(g_ntdll, "NtWriteVirtualMemory");

    int mapped = 0;
    for (int i = 0; i < chunk_count; ++i) {
        HANDLE sec = nullptr;
        NTSTATUS st = ntcs(&sec, SECTION_ALL_ACCESS,
            nullptr, nullptr, PAGE_READONLY, SEC_IMAGE, file);
        if (st < 0) continue;

        PVOID  view   = (PVOID)(carrier_base + (uintptr_t)i * kchunk);
        SIZE_T viewsz = 0;
        st = ntmvs(sec, proc, &view, 0, 0, nullptr,
            &viewsz, 2, 0, PAGE_EXECUTE_READ);
        CloseHandle(sec);
        if (st < 0) continue;

        DWORD old = 0;
        VirtualProtectEx(proc, view, kchunk, PAGE_EXECUTE_READWRITE, &old);

        std::vector<uint8_t> zeros(kchunk, 0);
        SIZE_T wrote = 0;
        ntwvm(proc, view, zeros.data(), kchunk, &wrote);

        chunks[i] = chunk{ (uintptr_t)view, kchunk };
        ++mapped;
    }
    CloseHandle(file);

    if (mapped == 0) return 0;

    HANDLE tx = CreateTransaction(nullptr, nullptr, 0, 0, 0, 0, nullptr);
    if ((uintptr_t)tx - 1 <= 0xFFFFFFFFFFFFFFFDULL) {
        DeleteFileTransactedA(tmp_file, tx);
        CommitTransaction(tx);
        CloseHandle(tx);
    } else {
        DeleteFileA(tmp_file);
    }

    return carrier_base;
}

static void writechunk(HANDLE proc, const chunk_map& chunks,
                                uintptr_t dst, const void* src, SIZE_T size) {
    auto ntwvm = (fn_ntwritevirtualmemory)GetProcAddress(g_ntdll, "NtWriteVirtualMemory");

    const uint8_t* p = (const uint8_t*)src;
    while (size > 0) {
        const chunk* hit = nullptr;
        for (auto& kv : chunks) {
            uintptr_t end = kv.second.base + kv.second.size;
            if (dst >= kv.second.base && dst < end) { hit = &kv.second; break; }
        }
        if (!hit) return;
        SIZE_T avail = hit->size - (dst - hit->base);
        SIZE_T n     = (size <= avail) ? size : avail;
        SIZE_T wrote = 0;
        ntwvm(proc, (PVOID)dst, (PVOID)p, n, &wrote);
        dst  += n;
        p    += n;
        size -= n;
    }
}

}

uintptr_t oraclemap(const rbxctx* ctx,
                     const std::vector<char>& dll,
                     uintptr_t& out_base) {
    out_base = 0;

    std::string carrier = getcarrier();
    if (carrier.empty()) return 0;

    susthread(ctx);

    auto dos = (PIMAGE_DOS_HEADER)dll.data();
    if (dll.empty() || dos->e_magic != IMAGE_DOS_SIGNATURE) {
        resumethread(ctx);
        return 0;
    }
    auto nt = (PIMAGE_NT_HEADERS64)(dll.data() + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        resumethread(ctx);
        return 0;
    }

    chunk_map chunks;
    uintptr_t base = buildcarrier(ctx->process, carrier,
                                          nt->OptionalHeader.SizeOfImage, chunks);
    if (!base) {
        resumethread(ctx);
        return 0;
    }

    writechunk(ctx->process, chunks, base,
        dll.data(), nt->OptionalHeader.SizeOfHeaders);

    auto sec = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec) {
        if (!sec->SizeOfRawData) continue;
        writechunk(ctx->process, chunks,
            base + sec->VirtualAddress,
            dll.data() + sec->PointerToRawData,
            sec->SizeOfRawData);
    }

    out_base = base;
    resumethread(ctx);
    return base + nt->OptionalHeader.AddressOfEntryPoint;
}

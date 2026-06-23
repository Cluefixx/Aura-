#include "common.h"
#include "config.h"
#include "rbx.h"
#include "mapper.h"
#include "inject.h"

#include <fstream>

static void con_print(const char* msg) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD w = 0;
    WriteConsoleA(h, msg, (DWORD)strlen(msg), &w, nullptr);
    WriteConsoleA(h, "\n", 1, &w, nullptr);
}

static std::vector<char> readfile(const char* path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    auto sz = (size_t)f.tellg();
    if (sz == 0) return {};
    f.seekg(0, std::ios::beg);
    std::vector<char> buf(sz);
    f.read(buf.data(), (std::streamsize)sz);
    if (!f) return {};
    return buf;
}

int main() {
    AllocConsole();

    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hin  = GetStdHandle(STD_INPUT_HANDLE);
    SetStdHandle(STD_OUTPUT_HANDLE, hout);
    SetStdHandle(STD_ERROR_HANDLE,  hout);
    SetStdHandle(STD_INPUT_HANDLE,  hin);
    SetConsoleTitleA("injector");

    DWORD con_mode = 0;
    if (hout != INVALID_HANDLE_VALUE && GetConsoleMode(hout, &con_mode))
        SetConsoleMode(hout, con_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    rbxctx* rbx = rbxget();

    if (!g_ntdll || !rbx->process) {
        con_print("failed to inject");
        Sleep(800);
        delete rbx;
        ExitProcess(0);
    }

    auto dll = readfile(module);
    if (dll.empty()) {
        con_print("failed to inject");
        Sleep(1000);
        delete rbx;
        ExitProcess(0);
    }

    uintptr_t image_base = 0;
    uintptr_t ep = oraclemap(rbx, dll, image_base);
    if (!ep) {
        con_print("failed to inject");
        Sleep(1000);
        if (rbx->process) CloseHandle(rbx->process);
        delete rbx;
        ExitProcess(0);
    }

    trIGGER(rbx, image_base);

    con_print("injected");

    if (rbx->process) CloseHandle(rbx->process);
    delete rbx;
    ExitProcess(0);
}

#include <Communication/Communication.hpp>
#include <Exploit/TeleportHandler/TeleportHandler.hpp>
#include <Roblox/Offsets.hpp>
#include <Exploit/Common/Log.hpp>
#include <Hyperion/Hooks.hpp>
#include <Exploit/Render/Render.hpp>

#include <Windows.h>
#include <thread>

static void ModuleMain(HMODULE)
{
    LogInit();

    //Render::Initialize();

    Communication::Initialize();
    TeleportHandler::Initialize();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        AddVectoredExceptionHandler(1, HyperionBypass::CFGHook);

        std::thread(ModuleMain, hModule).detach();
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        RemoveVectoredExceptionHandler(HyperionBypass::CFGHook);
        //Render::Shutdown();
    }

    return TRUE;
}

#include <windows.h>
#include "hook_render/hook_render.h"
#include "modules/console/console.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// DLL Entry Point
BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        {
            // Create thread to avoid blocking DllMain - minimal init
            HANDLE hThread = CreateThread(nullptr, 0, InitHookThread, nullptr, 0, nullptr);
            if (hThread)
            {
                CloseHandle(hThread);
            }
        }
        break;
    case DLL_PROCESS_DETACH:
        // Cleanup console
        Console::Cleanup();
        // Cleanup render
        CleanupRender();
        break;
    }
    return TRUE;
}

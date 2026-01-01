#include <windows.h>
#include "hook_render/hook_render.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// DLL Entry Point
BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // Create thread to avoid blocking DllMain
        CreateThread(nullptr, 0, InitHookThread, nullptr, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        // Cleanup
        CleanupRender();
        break;
    }
    return TRUE;
}

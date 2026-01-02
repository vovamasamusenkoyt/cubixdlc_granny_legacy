#include <windows.h>
#include "hook_render/hook_render.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// DLL Entry Point
BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
{
    __try
    {
        switch (ul_reason_for_call)
        {
        case DLL_PROCESS_ATTACH:
            // Create thread to avoid blocking DllMain
            // Don't do anything heavy here - DllMain should be fast
            CreateThread(nullptr, 0, InitHookThread, nullptr, 0, nullptr);
            break;
        case DLL_PROCESS_DETACH:
            // Cleanup
            CleanupRender();
            break;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // Exception in DllMain - can't do much here
        // Just return FALSE to indicate failure
        return FALSE;
    }
    return TRUE;
}

#include "hook_render.h"
#include "../deps/minhook/include/MinHook.h"
#include "../deps/imgui/imgui.h"
#include "../deps/imgui/backends/imgui_impl_dx11.h"
#include "../deps/imgui/backends/imgui_impl_win32.h"
#include "../modules/watermark/watermark.h"
#include "../hud/hud.h"

// Forward declare
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Globals
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
bool g_ImGuiInitialized = false;
HWND g_hWnd = nullptr;
WNDPROC g_OriginalWndProc = nullptr;
bool g_ShowMenu = false;

// Function pointers
typedef HRESULT(__stdcall* PresentFn)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall* ResizeBuffersFn)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

static PresentFn oPresent = nullptr;
static ResizeBuffersFn oResizeBuffers = nullptr;

// WndProc hook
LRESULT WINAPI HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (g_ImGuiInitialized)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;
    }
    return CallWindowProc(g_OriginalWndProc, hWnd, msg, wParam, lParam);
}

// Hook implementations
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (!g_ImGuiInitialized)
    {
        // Get device from swapchain
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice)))
        {
            g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);
            
            // Get window handle from swapchain
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            g_hWnd = sd.OutputWindow;
            g_pSwapChain = pSwapChain;
            
            // Initialize ImGui
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            
            ImGui::StyleColorsDark();
            
            ImGui_ImplWin32_Init(g_hWnd);
            ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
            
            // Hook WndProc to handle window messages (only once)
            if (!g_OriginalWndProc)
            {
                g_OriginalWndProc = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);
            }
            
            // Create render target
            ID3D11Texture2D* pBackBuffer = nullptr;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
            if (pBackBuffer)
            {
                g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
                pBackBuffer->Release();
            }
            
            g_ImGuiInitialized = true;
        }
    }
    
    if (g_ImGuiInitialized && g_pd3dDeviceContext && g_mainRenderTargetView)
    {
        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        // Toggle menu with "Del" key (VK_DELETE)
        static bool prevKeyState = false;
        bool currentKeyState = (GetAsyncKeyState(VK_DELETE) & 0x8000) != 0;
        if (currentKeyState && !prevKeyState)
        {
            bool wasOpen = g_ShowMenu;
            g_ShowMenu = !g_ShowMenu;
            
            // Play sound when opening/closing HUD
            if (g_ShowMenu && !wasOpen)
            {
                // HUD opened - play ON sound
                PlaySoundResource(IDR_SOUND_ON);
            }
            else if (!g_ShowMenu && wasOpen)
            {
                // HUD closed - play OFF sound
                PlaySoundResource(IDR_SOUND_OFF);
            }
        }
        prevKeyState = currentKeyState;
        
        // Render watermark (always visible)
        RenderWatermark();
        
        // Render HUD menu
        RenderHUD();
        
        // Render ImGui
        ImGui::Render();
        
        // Save current render targets
        ID3D11RenderTargetView* pOldRTV = nullptr;
        ID3D11DepthStencilView* pOldDSV = nullptr;
        g_pd3dDeviceContext->OMGetRenderTargets(1, &pOldRTV, &pOldDSV);
        
        // Set our render target
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        
        // Restore render targets
        g_pd3dDeviceContext->OMSetRenderTargets(1, &pOldRTV, pOldDSV);
        if (pOldRTV) pOldRTV->Release();
        if (pOldDSV) pOldDSV->Release();
    }
    
    return oPresent(pSwapChain, SyncInterval, Flags);
}

HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    // Cleanup render target before resize
    if (g_mainRenderTargetView)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
    
    // Call original ResizeBuffers
    HRESULT hr = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    
    // Recreate render target after resize
    if (g_ImGuiInitialized && g_pd3dDevice && SUCCEEDED(hr))
    {
        ID3D11Texture2D* pBackBuffer = nullptr;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
        if (pBackBuffer)
        {
            g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
            pBackBuffer->Release();
        }
    }
    
    return hr;
}

// Thread function to initialize hooks
DWORD WINAPI InitHookThread(LPVOID)
{
    // Wait a bit for the game to initialize
    Sleep(1000);
    
    // Initialize MinHook
    if (MH_Initialize() != MH_OK)
        return 1;
    
    // Create dummy device and swapchain to get vtable
    // Create a temporary window for the dummy swapchain
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"TempWindowClass";
    
    if (!RegisterClassExW(&wc))
    {
        MH_Uninitialize();
        return 1;
    }
    
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"TempWindow", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, wc.hInstance, NULL);
    if (!hwnd)
    {
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        MH_Uninitialize();
        return 1;
    }
    
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    
    ID3D11Device* pDummyDevice = nullptr;
    ID3D11DeviceContext* pDummyContext = nullptr;
    IDXGISwapChain* pDummySwapChain = nullptr;
    
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        &featureLevel,
        1,
        D3D11_SDK_VERSION,
        &sd,
        &pDummySwapChain,
        &pDummyDevice,
        nullptr,
        &pDummyContext);
    
    if (FAILED(hr))
    {
        DestroyWindow(hwnd);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        MH_Uninitialize();
        return 1;
    }
    
    // Get vtable
    void** pVTable = *(void***)pDummySwapChain;
    
    // Get function pointers from vtable
    void* pPresent = pVTable[8];      // Present index
    void* pResizeBuffers = pVTable[13]; // ResizeBuffers index
    
    // Cleanup dummy objects
    pDummyContext->Release();
    pDummyDevice->Release();
    pDummySwapChain->Release();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    
    // Create hooks
    if (MH_CreateHook(pPresent, &hkPresent, (LPVOID*)&oPresent) != MH_OK)
    {
        MH_Uninitialize();
        return 1;
    }
    
    if (MH_CreateHook(pResizeBuffers, &hkResizeBuffers, (LPVOID*)&oResizeBuffers) != MH_OK)
    {
        MH_Uninitialize();
        return 1;
    }
    
    // Enable hooks
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
    {
        MH_Uninitialize();
        return 1;
    }
    
    return 0;
}

// Cleanup
void CleanupRender()
{
    if (g_ImGuiInitialized)
    {
        // Restore original WndProc
        if (g_hWnd && g_OriginalWndProc)
            SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)g_OriginalWndProc);
        
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        
        if (g_mainRenderTargetView)
            g_mainRenderTargetView->Release();
        if (g_pd3dDeviceContext)
            g_pd3dDeviceContext->Release();
        if (g_pd3dDevice)
            g_pd3dDevice->Release();
    }
    
    // Disable and remove hooks
    if (oPresent)
    {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_RemoveHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }
}


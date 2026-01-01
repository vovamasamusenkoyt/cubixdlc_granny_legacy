#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

// Forward declarations
extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;
extern IDXGISwapChain* g_pSwapChain;
extern ID3D11RenderTargetView* g_mainRenderTargetView;
extern bool g_ImGuiInitialized;
extern HWND g_hWnd;
extern WNDPROC g_OriginalWndProc;
extern bool g_ShowMenu;

// Hook initialization
DWORD WINAPI InitHookThread(LPVOID);

// Hook functions
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

// WndProc hook
LRESULT WINAPI HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Cleanup
void CleanupRender();


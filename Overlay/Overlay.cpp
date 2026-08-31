#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"

#include "../Popup/Popup.h"
#include "Overlay.h"

#include <d3d11.h>
#include <dxgi.h>
#include <dwmapi.h>
#include <dxgiformat.h>
#include <d3dcommon.h>
#include <Uxtheme.h>
#include <Windows.h>
#include <string>
#include "../Cheat/Cheat.h"
#include "../Offsets/Offsets.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

static UINT WindowPosX = 0;
static UINT WindowPosY = 0;

static UINT WindowSizeX = 0;
static UINT WindowSizeY = 0;

static HWND OverlayWindow = 0;
static HWND TargetWindow = 0;

static LPCSTR TargetClass = "";
static LPCSTR TargetName = "";

static bool TargetInForeground = false;

static void CreateRenderTarget()
{
        ID3D11Texture2D* pBackBuffer = nullptr;
        if (SUCCEEDED(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer))))
        {
                g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
                pBackBuffer->Release();
        }
}

static void CleanupRenderTarget()
{
        if (g_mainRenderTargetView)
        {
                g_mainRenderTargetView->Release();
                g_mainRenderTargetView = nullptr;
        }
}

static bool CreateDeviceD3D(HWND hWnd)
{
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
        D3D_FEATURE_LEVEL featureLevel;

        HRESULT res = D3D11CreateDeviceAndSwapChain(
                nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_SINGLETHREADED,
                featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
                &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext
        );

        if (FAILED(res))
                return false;

        CreateRenderTarget();
        return true;
}

static void CleanupDeviceD3D()
{
        CleanupRenderTarget();

        if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
        if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
        if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
                return true;

        switch (msg)
        {
        case WM_SYSCOMMAND:
                if ((wParam & 0xfff0) == SC_KEYMENU)
                        return 0;
                break;
        case WM_DESTROY:
                ::PostQuitMessage(0);
                return 0;
        }
        return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

static void ResizeOverlay()
{
        if (g_pd3dDeviceContext != nullptr)
        {
                CleanupRenderTarget();
                g_pd3dDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
                g_pSwapChain->ResizeBuffers(0, WindowSizeX, WindowSizeY, DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
        }

        ::SetWindowPos(OverlayWindow, HWND_TOPMOST, WindowPosX, WindowPosY, WindowSizeX, WindowSizeY, SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

static void UpdateTargetSize()
{
        POINT Point{};
        ClientToScreen(TargetWindow, &Point);

        RECT Rect{};
        GetClientRect(TargetWindow, &Rect);

        if (WindowPosX == Point.x && WindowPosY == Point.y && WindowSizeX == Rect.right && WindowSizeY == Rect.bottom)
                return;

        WindowPosX = Point.x;
        WindowPosY = Point.y;
        WindowSizeX = Rect.right;
        WindowSizeY = Rect.bottom;

        ResizeOverlay();
}

static HWND GetTarget()
{
        if (IsWindow(TargetWindow))
                return TargetWindow;
        
        TargetWindow = FindWindowA(TargetClass, TargetName);

        return TargetWindow;
}

static bool AttachToTarget(const LPCSTR& TargetClassParam, const LPCSTR& TargetNameParam)
{
        TargetClass = TargetClassParam;
        TargetName = TargetNameParam;

        TargetWindow = GetTarget();

        if (TargetWindow == NULL)
                return false;

        UpdateTargetSize();

        return true;
}

static bool PeekMessageQuit()
{
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                        return true;
        }
        return false;
}

static bool UpdateTargetNow()
{
        if (GetTarget() == NULL)
                return false;

        UpdateTargetSize();

        return true;
}

static bool UpdateTargetPeriodically()
{
        static ULONGLONG LastUpdate = 0;
        ULONGLONG CurrentTick = GetTickCount64();

        if (CurrentTick - LastUpdate > 200)
        {
                LastUpdate = CurrentTick;

                if (not UpdateTargetNow())
                        return false;

                bool TargetWasInForeground = TargetInForeground;
                TargetInForeground = GetForegroundWindow() == TargetWindow;

                if (not TargetWasInForeground and TargetInForeground)
                        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        }

        return true;
}

static bool HandleInits()
{
        bool Success;

        Success = Offsets::Init();
        if (!Success)
        {
                Popup::Error(Offsets::GetError());
                return false;
        }

        return true;
}

void Overlay::Run()
{
        ImGui_ImplWin32_EnableDpiAwareness();

        WNDCLASSEXW wc = { 
                sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, 
                GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, 
                L"Overlay Class", nullptr 
        };
        ::RegisterClassExW(&wc);

        if (!AttachToTarget("SDL_app", "Counter-Strike 2"))
        {
                Popup::Error("Couldn't attach overlay to target");
                return;
        }

        OverlayWindow = ::CreateWindowEx(
                WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
                wc.lpszClassName, L"Dear ImGui DirectX11 Overlay", WS_POPUP,
                WindowPosX, WindowPosY, WindowSizeX, WindowSizeY,
                nullptr, nullptr, wc.hInstance, nullptr
        );

        if (OverlayWindow == NULL)
        {
                Popup::Error("Window couldn't be created");
                return;
        }

        LONG_PTR exStyle = GetWindowLongPtr(OverlayWindow, GWL_EXSTYLE);
        ::SetWindowLongPtr(OverlayWindow, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);

        ::SetLayeredWindowAttributes(OverlayWindow, RGB(0, 0, 0), 255, LWA_ALPHA);

        MARGINS margins = { -1 };
        ::DwmExtendFrameIntoClientArea(OverlayWindow, &margins);

        if (!CreateDeviceD3D(OverlayWindow))
        {
                CleanupDeviceD3D();
                ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
                Popup::Error("Device couldn't be created");
                return;
        }

        ::ShowWindow(OverlayWindow, SW_SHOWDEFAULT);
        ::UpdateWindow(OverlayWindow);

        ImGui::CreateContext();

        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(OverlayWindow);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);

        bool SuccessfullyInited = HandleInits();
        if (!SuccessfullyInited)
                return;

        while (true)
        {
                if (PeekMessageQuit())
                        break;

                if (not UpdateTargetPeriodically())
                        break;

                if (not TargetInForeground)
                {
                        ::Sleep(20);
                        continue;
                }

                ImGui_ImplDX11_NewFrame();
                ImGui_ImplWin32_NewFrame();
                ImGui::NewFrame();

                Cheat::Run();

                ImGui::Render();

                const float clear_color_transparent[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
                g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_transparent);

                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

                g_pSwapChain->Present(1, 0);
        }

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceD3D();
        ::DestroyWindow(OverlayWindow);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

        return;
}
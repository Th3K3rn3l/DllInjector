// Dear ImGui: standalone example application for Windows API + DirectX 11

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <windowsx.h>
#include <string>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#include "ImageBytes.h"

#include "processManager.h"
#include <codecvt>

#include "Injector.h";


// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


std::wstring OpenFileDialogW() {
    wchar_t szFile[260] = { 0 }; // Используем wchar_t вместо char
    OPENFILENAMEW ofn;           // Используем структуру W
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);

    // Строки фильтра тоже должны быть широкими (L"...")
    ofn.lpstrFilter = L"DLL Files (*.dll)\0*.dll\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn)) {
        return std::wstring(szFile); // Возвращаем wstring
    }
    return L"";
}

bool LoadTextureFromMemory(const unsigned char* data, size_t data_size, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    int image_width = 0;
    int image_height = 0;
    int channels = 0;
    unsigned char* rgba_data = stbi_load_from_memory(data, (int)data_size, &image_width, &image_height, &channels, 4);

    if (rgba_data == nullptr) return false;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = rgba_data;
    subResource.SysMemPitch = image_width * 4;

    ID3D11Texture2D* pTexture = nullptr;
    HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
    if (SUCCEEDED(hr))
    {
        g_pd3dDevice->CreateShaderResourceView(pTexture, nullptr, out_srv);
        pTexture->Release();
        *out_width = image_width;
        *out_height = image_height;
    }

    stbi_image_free(rgba_data);
    return SUCCEEDED(hr);
}

// Main code
int main(int, char**)
{
    setlocale(LC_ALL, "Russian");
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"DLL INJECTOR", WS_POPUP, 100, 100, (int)(854 * main_scale), (int)(480 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    ImFont* fontTitle = io.Fonts->AddFontFromFileTTF("./customFonts/Orbitron-ExtraBold.ttf", 32.0f); // для названия окна
    ImFont* fontBold = io.Fonts->AddFontFromFileTTF("./customFonts/Orbitron-Bold.ttf", 20.0f); // Для заголовков
    ImFont* fontRegular = io.Fonts->AddFontFromFileTTF("./customFonts/Orbitron-Regular.ttf", 18.0f); // обычный

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Переменные
    std::string current_process = "Notepad.exe";
    std::wstring current_dll = L"C://Injected.dll";
    static int active_tab = 0;
    ProcessManager pm;
    std::vector<Process> processes =  pm.getProcesses();
    int current_pid = 0;
    std::string current_method = "standard injection";
    std::vector<std::string> INJECTION_METHODS = {
    "standard injection",
    "thread hijacking"
    }
    ;

    // СТИЛИ
    // Устанавливаем цвет фона окна (очень темный сине-черный)
    style.Colors[ImGuiCol_WindowBg] = ImVec4(222.0f, 215.0f, 215.0f, 1.00f);
    style.WindowRounding = 10.0f; // Скругление основного окна
    style.WindowBorderSize = 1.0f;
    style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.FrameRounding = 5.0f;      // Скругление кнопок, инпутов и комбо-боксов
    style.GrabRounding = 12.0f;      // Скругление ползунков (слайдеров)
    style.PopupRounding = 8.0f;      // Скругление выпадающих списков (твой Combo)
    // Цвет активного (нажатого) раздела Selectable
    style.Colors[ImGuiCol_Header] = ImVec4(0, 0, 0, 0); // Обычное выделение
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0, 0, 0, 0); // При наведении
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0, 0, 0, 0); // При клике
    // Фон самого выпадающего списка
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);

    // Текст в списке (сделаем его черным)
    style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);

    // Цвет текста внутри чекбоксов и инпутов (если они тоже белые)
    style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f); // Белый фон полей
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.20f, 0.45f, 0.88f, 1.00f); // Синяя галочка
    // Картинки
    ID3D11ShaderResourceView* logoTexture = nullptr;
    int logo_w, logo_h;

    // Загружаем лого
    LoadTextureFromMemory(logo, sizeof(logo), &logoTexture, &logo_w, &logo_h);
    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        // Растянуть окно ImGUI на всю рабочую поверхность
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

        // Убираем все стандартные декорации окна ImGui
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar;

        ImGui::Begin("MainLayout", nullptr, flags);
        {
            // Лого
            ImGui::SetCursorPos(ImVec2(30, 60));
            ImGui::Image((void*)logoTexture, ImVec2(140,140));
            // Колонка кнопок разделов слева
            ImGui::SetCursorPos({ 50, 230 });
            ImGui::PushFont(fontRegular);
            ImGui::BeginChild("TabsList", ImVec2(150, 0), false);
            {
                // Определяем три цвета
                ImVec4 col_black = ImVec4(0.00f, 0.00f, 0.00f, 1.00f); // Обычный
                ImVec4 col_blue = ImVec4(0.20f, 0.45f, 0.88f, 1.00f); // Активный (синий)

                // Кнопка 1: Injection
                bool active0 = (active_tab == 0);
                ImGui::PushStyleColor(ImGuiCol_Text, active0 ? col_blue : col_black);
                if (ImGui::Selectable("Injection", active0, 0, ImVec2(0, 30))) { active_tab = 0; }
                ImGui::PopStyleColor();
                ImGui::Dummy(ImVec2(0, 10.0f)); // Добавляет 10 пикселей пустого пространства вниз
                // Кнопка 2: Settings
                bool active1 = (active_tab == 1);
                ImGui::PushStyleColor(ImGuiCol_Text, active1 ? col_blue : col_black);
                if (ImGui::Selectable("Settings", active1, 0, ImVec2(0, 30))) { active_tab = 1; }
                ImGui::PopStyleColor();
                ImGui::Dummy(ImVec2(0, 10.0f)); // Добавляет 10 пикселей пустого пространства вниз
                // Кнопка 3: About me
                bool active2 = (active_tab == 2);
                ImGui::PushStyleColor(ImGuiCol_Text, active2 ? col_blue : col_black);
                if (ImGui::Selectable("About me", active2, 0, ImVec2(0, 30))) { active_tab = 2; }
                ImGui::PopStyleColor();
            }
            ImGui::EndChild();
            ImGui::PopFont();
            // Контент раздела (который справа)
            ImGui::SetCursorPos({450, 30});
            switch (active_tab)
            {
                case 0: // Страница Injection
                    {
                        // Заголовок
                        ImGui::PushFont(fontTitle);
                        ImGui::Text("INJECTION");
                        ImGui::SetCursorPos({300, 100});
                        ImGui::PopFont();
                        // список процессов
                        ImGui::PushFont(fontRegular);
                        ImGui::SetNextItemWidth(500.0f);
                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.45f, 0.88f, 0.20f));
                        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.20f, 0.45f, 0.88f, 0.40f));
                        ImGui::SetNextWindowSize(ImVec2(500, 0));
                        if (ImGui::BeginCombo("##process_combo", current_process.c_str()))
                        {
                            for (Process process : processes)
                            {
                                std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                                std::string processName = converter.to_bytes(process.name);

                                // Используем PID для уникальности
                                std::string displayName = processName + " (PID: " + std::to_string(process.ID) + ")";
                                std::string selectableId = displayName + "##" + std::to_string(process.ID);
                                if (ImGui::Selectable(selectableId.c_str()))
                                {
                                    current_process = processName;
                                    current_pid = process.ID;
                                }
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::PopStyleColor(2);
                        ImGui::SameLine();
                        if (ImGui::Button("Refresh", { 100, 30 }))
                        {
                            processes = pm.getProcesses();
                        }
                        ImGui::Dummy(ImVec2(0, 10));         // вертикальный отступ
                        ImGui::SetCursorPosX(300);
                        ImGui::SetNextWindowSize(ImVec2(500, 0));
                        ImGui::SetNextItemWidth(500);
                        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                        std::string currentDll = converter.to_bytes(current_dll);
                        if (ImGui::BeginCombo("##dll_combo", currentDll.c_str()))
                        {
                            ImGui::EndCombo();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Add dll", { 100, 30 }))
                        {
                            current_dll = OpenFileDialogW();
                        }
                        ImGui::Dummy(ImVec2(0, 10));         // вертикальный отступ
                        ImGui::SetCursorPosX(300);
                        ImGui::SetNextWindowSize(ImVec2(500, 0));
                        ImGui::SetNextItemWidth(500);
                        if (ImGui::BeginCombo("##method_combo", current_method.c_str()))
                        {
                            for (std::string method : INJECTION_METHODS)
                            {
                                if (ImGui::Selectable(method.c_str()))
                                {
                                    current_method = method;
                                }
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Inject", { 100, 30 }))
                        {
                            Injector injector;
                            injector.setDllPath(current_dll);
                            injector.setPID(current_pid);
                            injector.setThreadId(19320);
                            injector.standardInject();
                            if (current_method == "standard injection") injector.standardInject();
                            else injector.threadHijacking();
                        }
                        ImGui::PopFont();
                        break;
                    }
                case 1: // Страница Settings
                    {
                        ImGui::PushFont(fontTitle);
                        ImGui::Text("SETTINGS");
                        ImGui::PopFont();
                        break;
                    }
                case 2: // Страница About me
                    {
                        ImGui::PushFont(fontTitle);
                        ImGui::Text("ABOUT ME");
                        ImGui::PopFont();
                        break;
                    }
            }
            
            
        }
        ImGui::End();
        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    // This is a basic setup. Optimally could use e.g. DXGI_SWAP_EFFECT_FLIP_DISCARD and handle fullscreen mode differently. See #8979 for suggestions.
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
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

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_NCHITTEST:
    {
        // Определяем зону, за которую можно таскать окно
        const int TOP_BAR_HEIGHT = 70; // Высота твоего кастомного тайтл-бара
        POINT cursor = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ::ScreenToClient(hWnd, &cursor);

        if (cursor.y < TOP_BAR_HEIGHT and cursor.x < ImGui::GetIO().DisplaySize.x - 85)
            return HTCAPTION; // Говорим винде, что тут "заголовок"

        return HTCLIENT;
    }
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

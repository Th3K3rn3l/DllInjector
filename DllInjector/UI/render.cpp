#include "render.h"
#include "imgui.h"
#include "../processManager.h"
#include "../Injector.h"
#include "../assets/ImageBytes.h"
#include <codecvt>
#include "../helperFuncs/helperFuncs.h"
#include <cpr/cpr.h>
#include <crow/json.h>   // для crow::json::load

extern ID3D11Device* g_pd3dDevice;


Renderer::Renderer(bool* doneOrNot, ImGuiIO& io, AuthState* auth_state)
{
    int logo_w, logo_h;
    LoadTextureFromMemory(logo, sizeof(logo), &logoTexture, &logo_w, &logo_h);
    fontTitle = io.Fonts->AddFontFromFileTTF("./customFonts/Orbitron-ExtraBold.ttf", 32.0f);
    fontBold = io.Fonts->AddFontFromFileTTF("./customFonts/Orbitron-Bold.ttf", 20.0f);
    fontRegular = io.Fonts->AddFontFromFileTTF("./customFonts/Orbitron-Regular.ttf", 18.0f);
    processes = pm.getProcesses();
    current_auth_state = auth_state;
    done = doneOrNot;
}

#include <vector>
#include <cstdint>

#include <vector>
#include <cstdint>

ID3D11ShaderResourceView* Renderer::generateQRTexture(ID3D11Device* device, const std::string& text, int pixels_per_module) {
    using namespace qrcodegen;
    QrCode qr = QrCode::encodeText(text.c_str(), QrCode::Ecc::MEDIUM);
    int size = qr.getSize() * pixels_per_module;
    std::vector<uint32_t> pixels(size * size, 0xFFFFFFFF); // белый фон

    for (int y = 0; y < qr.getSize(); ++y) {
        for (int x = 0; x < qr.getSize(); ++x) {
            if (qr.getModule(x, y)) {
                for (int py = 0; py < pixels_per_module; ++py) {
                    for (int px = 0; px < pixels_per_module; ++px) {
                        int idx = (y * pixels_per_module + py) * size + (x * pixels_per_module + px);
                        pixels[idx] = 0xFF000000; // чёрный
                    }
                }
            }
        }
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = size;
    desc.Height = size;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pixels.data();
    initData.SysMemPitch = size * 4;

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, &initData, &texture);
    if (FAILED(hr)) return nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    device->CreateShaderResourceView(texture, nullptr, &srv);
    texture->Release();
    return srv;
}

void Renderer::drawLogin()
{
    // Кнопка закрытия
    float windowWidth = ImGui::GetIO().DisplaySize.x;
    ImGui::SetCursorPos(ImVec2(windowWidth - 60, 25));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 0.2f, 0.2f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 0, 0, 0.6f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    if (ImGui::Button("X", ImVec2(50, 50))) *done = true;
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    ImGui::SetCursorPos(ImVec2(windowSize.x * 0.5f - 150, 80));
    ImGui::BeginChild("LoginFrame", ImVec2(500, 400), true, ImGuiWindowFlags_NoBackground);
    {
        ImGui::PushFont(fontTitle);
        ImGui::Text("WELCOME BACK");
        ImGui::PopFont();
        ImGui::Dummy(ImVec2(0, 20));

        static char user_name[64] = "";
        static char user_pass[64] = "";

        ImGui::PushFont(fontRegular);
        ImGui::Text("Username");
        ImGui::InputText("##login", user_name, IM_ARRAYSIZE(user_name));
        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Text("Password");
        ImGui::InputText("##pass", user_pass, IM_ARRAYSIZE(user_pass), ImGuiInputTextFlags_Password);
        ImGui::Dummy(ImVec2(0, 20));

        if (ImGui::Button("LOGIN", ImVec2(300, 40))) {
            std::string json_payload = "{\"username\": \"" + std::string(user_name) + "\", "
                "\"password\": \"" + std::string(user_pass) + "\"}";
            cpr::Response r = cpr::Post(cpr::Url{ "http://localhost:8000/login" },
                cpr::Header{ {"Content-Type", "application/json"} },
                cpr::Body{ json_payload });
            if (r.status_code == 200) {
                auto data = crow::json::load(r.text);
                if (data.has("requires_2fa") && data["requires_2fa"].b()) {
                    // Требуется 2FA
                    temp_2fa_token = data["temp_token"].s();
                    twofa_code.clear();
                    twofa_status.clear();
                    *current_auth_state = AUTH_2FA;
                }
                else if (data.has("token")) {
                    // Успешный вход без 2FA
                    jwt_token = data["token"].s();
                    *current_auth_state = AUTH_SUCCESS;
                }
                else {
                    twofa_status = "Unknown response";
                }
            }
            else {
                twofa_status = "Login failed";
            }
        }

        ImGui::SetCursorPosX(40);
        if (ImGui::Selectable("Don't have an account?", false, 0, ImVec2(250, 50))) {
            *current_auth_state = AUTH_REGISTER;
        }
        ImGui::PopFont();
    }
    ImGui::EndChild();
}

void Renderer::drawRegister()
{
    float windowWidth = ImGui::GetIO().DisplaySize.x;
    ImGui::SetCursorPos(ImVec2(windowWidth - 60, 25));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 0.2f, 0.2f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 0, 0, 0.6f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    if (ImGui::Button("X", ImVec2(50, 50))) *done = true;
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    ImGui::SetCursorPos(ImVec2(windowSize.x * 0.5f - 150, 80));
    ImGui::BeginChild("RegisterFrame", ImVec2(500, 400), true, ImGuiWindowFlags_NoBackground);
    {
        ImGui::PushFont(fontTitle);
        ImGui::Text("CREATE ACCOUNT");
        ImGui::PopFont();
        ImGui::Dummy(ImVec2(0, 20));

        static char reg_name[64] = "";
        static char reg_pass[64] = "";

        ImGui::PushFont(fontRegular);
        ImGui::Text("Username");
        ImGui::InputText("##reg_user", reg_name, IM_ARRAYSIZE(reg_name));
        ImGui::Dummy({ 0,10 });
        ImGui::Text("Password");
        ImGui::InputText("##reg_pass", reg_pass, IM_ARRAYSIZE(reg_pass), ImGuiInputTextFlags_Password);
        ImGui::Dummy(ImVec2(0, 20));

        if (ImGui::Button("REGISTER", ImVec2(300, 40))) {
            std::string json_payload = "{\"username\": \"" + std::string(reg_name) + "\", "
                "\"password\": \"" + std::string(reg_pass) + "\"}";
            cpr::Response r = cpr::Post(cpr::Url{ "http://localhost:8000/register" },
                cpr::Header{ {"Content-Type", "application/json"} },
                cpr::Body{ json_payload });
            if (r.status_code == 200) *current_auth_state = AUTH_LOGIN;
        }

        ImGui::SetCursorPosX(90);
        if (ImGui::Selectable("Back to login", false, 0, ImVec2(160, 20))) {
            *current_auth_state = AUTH_LOGIN;
        }
        ImGui::PopFont();
    }
    ImGui::EndChild();
}

void Renderer::draw2FA()
{
    // Кнопка закрытия
    float windowWidth = ImGui::GetIO().DisplaySize.x;
    ImGui::SetCursorPos(ImVec2(windowWidth - 60, 25));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 0.2f, 0.2f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 0, 0, 0.6f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5);
    if (ImGui::Button("X", ImVec2(50, 50))) *done = true;
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    ImGui::SetCursorPos(ImVec2(windowSize.x * 0.5f - 200, 80));
    ImGui::BeginChild("2FAFrame", ImVec2(400, 250), true, ImGuiWindowFlags_NoBackground);
    ImGui::PushFont(fontTitle);
    ImGui::Text("TWO-FACTOR AUTH");
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0, 20));

    ImGui::PushFont(fontRegular);
    ImGui::Text("Enter code from Google Authenticator");
    static char code[7] = "";
    ImGui::InputText("##2fa_code", code, 7, ImGuiInputTextFlags_CharsDecimal);
    twofa_code = code;
    ImGui::Dummy(ImVec2(0, 20));
    if (ImGui::Button("VERIFY", ImVec2(300, 40))) {
        crow::json::wvalue req;
        req["temp_token"] = temp_2fa_token;
        req["code"] = twofa_code;
        cpr::Response r = cpr::Post(cpr::Url{ "http://localhost:8000/verify-2fa-login" },
            cpr::Header{ {"Content-Type", "application/json"} },
            cpr::Body{ req.dump() });
        if (r.status_code == 200) {
            auto data = crow::json::load(r.text);
            if (data.has("token")) {
                jwt_token = data["token"].s();
                *current_auth_state = AUTH_SUCCESS;
            }
            else {
                twofa_status = "Invalid response";
            }
        }
        else {
            twofa_status = "Invalid code";
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("BACK", ImVec2(100, 40))) {
        *current_auth_state = AUTH_LOGIN;
    }
    if (!twofa_status.empty()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", twofa_status.c_str());
    }
    ImGui::PopFont();
    ImGui::EndChild();
}

// ... (другие методы)

void Renderer::drawSettingsContent()
{
    // Заголовок и разделитель
    ImGui::PushFont(fontTitle);
    ImGui::Text("SETTINGS");
    ImGui::PopFont();
    ImGui::Separator();

    ImGui::PushFont(fontRegular);
    ImGui::Text("Two-Factor Authentication (2FA)");
    ImGui::Dummy(ImVec2(0, 10));

    // Сценарий 1: Кнопка для включения 2FA
    if (!showing_2fa_setup) {
        if (ImGui::Button("Enable 2FA", ImVec2(150, 0))) {
            if (jwt_token.empty()) {
                twofa_status = "Not authenticated";
            }
            else {
                // Отправляем запрос на сервер для генерации секрета
                cpr::Header headers;
                headers["Authorization"] = "Bearer " + jwt_token;
                headers["Content-Type"] = "application/json";
                cpr::Response r = cpr::Post(cpr::Url{ "http://localhost:8000/enable2fa" }, headers);
                if (r.status_code == 200) {
                    auto data = crow::json::load(r.text);
                    pending_secret = data["secret"].s();
                    pending_uri = data["uri"].s();
                    verify_2fa_code.clear();

                    // 🔄 **ВОТ ЗДЕСЬ МЫ ГЕНЕРИРУЕМ QR-КОД**
                    if (qrTexture) { qrTexture->Release(); qrTexture = nullptr; }
                    qrTexture = generateQRTexture(g_pd3dDevice, pending_uri, 4);
                    showing_2fa_setup = true;
                }
                else { twofa_status = "Failed to generate 2FA secret"; }
            }
        }
    }
    // Сценарий 2: Отображение QR-кода и поля для подтверждения
    else {
        // 🖼️ **ПОКАЗЫВАЕМ QR-КОД**
        if (qrTexture) {
            ImGui::Image((void*)qrTexture, ImVec2(200, 200));
        }
        else {
            ImGui::Text("QR code not available");
        }

        // А если QR-код не сработал, показываем секрет текстом
        ImGui::TextWrapped("Or enter this secret manually in Google Authenticator:");
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 1, 1));
        ImGui::TextWrapped("%s", pending_secret.c_str());
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 10));

        // Поле для ввода кода из приложения
        ImGui::Text("Enter verification code:");
        static char vcode[7] = "";
        ImGui::InputText("##verify2fa", vcode, 7, ImGuiInputTextFlags_CharsDecimal);
        verify_2fa_code = vcode;

        // Кнопка подтверждения
        if (ImGui::Button("Confirm 2FA", ImVec2(150, 0))) {
            crow::json::wvalue req;
            req["secret"] = pending_secret;
            req["code"] = verify_2fa_code;

            cpr::Header headers;
            headers["Authorization"] = "Bearer " + jwt_token;
            headers["Content-Type"] = "application/json";
            cpr::Response r = cpr::Post(cpr::Url{ "http://localhost:8000/confirm2fa" }, headers, cpr::Body{ req.dump() });
            if (r.status_code == 200) {
                showing_2fa_setup = false;
                twofa_status = "2FA enabled successfully!";

                // 🧹 Освобождаем текстуру, она больше не нужна
                if (qrTexture) { qrTexture->Release(); qrTexture = nullptr; }
            }
            else { twofa_status = "Invalid code, try again"; }
        }
        ImGui::SameLine();

        // Кнопка отмены
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            showing_2fa_setup = false;
            // 🧹 Освобождаем текстуру, если нажали "Отмена"
            if (qrTexture) { qrTexture->Release(); qrTexture = nullptr; }
        }
    }

    // Отображение статуса-сообщения (успех/ошибка)
    if (!twofa_status.empty()) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", twofa_status.c_str());
    }
    ImGui::PopFont();
}

// ... (остальные методы, включая drawInjectionContent и drawAboutContent)

void Renderer::drawInjectionContent()
{
    ImGui::PushFont(fontTitle);
    ImGui::Text("INJECTION");
    ImGui::PopFont();

    ImGui::PushFont(fontRegular);
    ImGui::SetNextItemWidth(500);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.45f, 0.88f, 0.20f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.20f, 0.45f, 0.88f, 0.40f));

    if (ImGui::BeginCombo("##process_combo", current_process.c_str())) {
        for (Process process : processes) {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            std::string processName = converter.to_bytes(process.name);
            std::string displayName = processName + " (PID: " + std::to_string(process.ID) + ")";
            std::string selectableId = displayName + "##" + std::to_string(process.ID);
            if (ImGui::Selectable(selectableId.c_str())) {
                current_process = processName;
                current_pid = process.ID;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopStyleColor(2);
    ImGui::SameLine();
    if (ImGui::Button("Refresh", { 100, 30 })) processes = pm.getProcesses();

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::SetNextItemWidth(500);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string currentDll = converter.to_bytes(current_dll);
    if (ImGui::BeginCombo("##dll_combo", currentDll.c_str())) ImGui::EndCombo();
    ImGui::SameLine();
    if (ImGui::Button("Add dll", { 100, 30 })) current_dll = OpenFileDialogW();

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::SetNextItemWidth(500);
    if (ImGui::BeginCombo("##method_combo", current_method.c_str())) {
        for (std::string method : injector.INJECTION_METHODS) {
            if (ImGui::Selectable(method.c_str())) current_method = method;
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Inject", { 100, 30 })) {
        injector.setDllPath(current_dll);
        injector.setPID(current_pid);
        injector.setThreadId(18888);
        if (current_method == "standard injection")
            injector.standardInject();
        else
            injector.threadHijacking();
    }
    ImGui::PopFont();
}
void Renderer::drawAboutContent()
{
    ImGui::PushFont(fontTitle);
    ImGui::Text("ABOUT ME");
    ImGui::PopFont();
    ImGui::PushFont(fontRegular);
    ImGui::TextWrapped("DLL Injector with 2FA support\nVersion 1.0\n\nPowered by ImGui + Crow + libsodium");
    ImGui::PopFont();
}



void Renderer::drawMain()
{
    // Логотип
    ImGui::SetCursorPos(ImVec2(30, 60));
    ImGui::Image((void*)logoTexture, ImVec2(140, 140));

    // Кнопка закрытия (без изменений)
    float windowWidth = ImGui::GetIO().DisplaySize.x;
    ImGui::SetCursorPos(ImVec2(windowWidth - 60, 25));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 0.2f, 0.2f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 0, 0, 0.6f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    if (ImGui::Button("X", ImVec2(50, 50))) *done = true;
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    // Левая колонка вкладок (фиксированная ширина 200)
    const float leftPanelWidth = 200.0f;
    ImGui::SetCursorPos({ 20, 230 });
    ImGui::PushFont(fontRegular);
    ImGui::BeginChild("TabsList", ImVec2(leftPanelWidth, ImGui::GetIO().DisplaySize.y - 280), false);
    {
        ImVec4 col_black = ImVec4(0, 0, 0, 1);
        ImVec4 col_blue = ImVec4(0.20f, 0.45f, 0.88f, 1);
        bool active0 = (active_tab == 0);
        ImGui::PushStyleColor(ImGuiCol_Text, active0 ? col_blue : col_black);
        if (ImGui::Selectable("Injection", active0, 0, ImVec2(0, 30))) active_tab = 0;
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 10));
        bool active1 = (active_tab == 1);
        ImGui::PushStyleColor(ImGuiCol_Text, active1 ? col_blue : col_black);
        if (ImGui::Selectable("Settings", active1, 0, ImVec2(0, 30))) active_tab = 1;
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 10));
        bool active2 = (active_tab == 2);
        ImGui::PushStyleColor(ImGuiCol_Text, active2 ? col_blue : col_black);
        if (ImGui::Selectable("About me", active2, 0, ImVec2(0, 30))) active_tab = 2;
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopFont();

    // Правая область контента (всё оставшееся место)
    float contentX = leftPanelWidth + 40;       // отступ от левой панели
    float contentWidth = ImGui::GetIO().DisplaySize.x - contentX - 30; // отступ справа
    float contentHeight = ImGui::GetIO().DisplaySize.y - 80;
    ImGui::SetCursorPos({ contentX, 30 });
    ImGui::BeginChild("ContentArea", ImVec2(contentWidth, contentHeight), false);
    {
        // Принудительно чёрный текст (чтобы было читаемо)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
        switch (active_tab)
        {
        case 0: drawInjectionContent(); break;
        case 1: drawSettingsContent(); break;
        case 2: drawAboutContent(); break;
        }
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
}
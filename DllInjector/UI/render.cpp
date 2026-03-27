#include "render.h"
#include "imgui.h"
#include "../processManager.h"
#include "../Injector.h"
#include "../assets/ImageBytes.h"
#include <codecvt>
#include "../helperFuncs/helperFuncs.h"
#include <cpr/cpr.h>

Renderer::Renderer(bool* doneOrNot, ImGuiIO& io, AuthState* auth_state)
{
    // Загружаем logo и сохраняем в logoTexture
    int logo_w, logo_h;
    LoadTextureFromMemory(logo, sizeof(logo), &logoTexture, &logo_w, &logo_h);
    // загружаем шрифты
    fontTitle = io.Fonts->AddFontFromFileTTF("./customFonts/Orbitron-ExtraBold.ttf", 32.0f); // для названия окна
    fontBold = io.Fonts->AddFontFromFileTTF("./customFonts/Orbitron-Bold.ttf", 20.0f); // Для заголовков
    fontRegular = io.Fonts->AddFontFromFileTTF("./customFonts/Orbitron-Regular.ttf", 18.0f); // обычный
    // загружаем процессы
    processes = pm.getProcesses();
    current_auth_state = auth_state;
    // Загружаем статус программы
    done = doneOrNot;
}


void Renderer::drawLogin()
{
    // Кнопка закрытия окна
    float windowWidth = ImGui::GetIO().DisplaySize.x;
    ImGui::SetCursorPos(ImVec2(windowWidth - 60, 25));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));          // Прозрачный фон
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 0.4f)); // Красный отсвет при наведении
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.6f));  // Ярко-красный при клике
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

    if (ImGui::Button("X", ImVec2(50, 50)))
    {
        *done = true; // Закрываем цикл приложения
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    // Центрируем форму
    ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    ImGui::SetCursorPos(ImVec2(windowSize.x * 0.5f - 150, 80));
    // Блок входа
    ImGui::BeginChild("LoginFrame", ImVec2(500, 400), true, ImGuiWindowFlags_NoBackground);
    {
        // Заголовок
        ImGui::PushFont(fontTitle);
        ImGui::Text("WELCOME BACK");
        ImGui::PopFont();
        ImGui::Dummy(ImVec2(0, 20));

        // Поля ввода
        static char user_name[64] = "";
        static char user_pass[64] = "";

        ImGui::PushFont(fontRegular);
        ImGui::Text("Username");
        ImGui::InputText("##login", user_name, IM_ARRAYSIZE(user_name));

        ImGui::Dummy(ImVec2(0, 10));

        ImGui::Text("Password");
        ImGui::InputText("##pass", user_pass, IM_ARRAYSIZE(user_pass), ImGuiInputTextFlags_Password);

        ImGui::Dummy(ImVec2(0, 20));

        // Кнопка входа
        if (ImGui::Button("LOGIN", ImVec2(300, 40))) {
            // Здесь логика авторизации
            std::string my_hwid = GetMachineHWID();
            std::string json_payload = "{\"username\": \"" + std::string(user_name) + "\", "
                "\"password\": \"" + std::string(user_pass) + "\", "
                "\"hwid\": \"" + my_hwid + "\"}";
            cpr::Response r = cpr::Post(cpr::Url{ "http://localhost:8000/login" },
                cpr::Header{ { "Content-Type", "application/json" } }, // Указываем, что это JSON
                cpr::Body{ json_payload }                             // Передаем строку
            );
            if (r.status_code == 200) *current_auth_state = AUTH_SUCCESS;
        }

        // Переход на регистрацию
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
    // Кнопка закрытия окна
    float windowWidth = ImGui::GetIO().DisplaySize.x;
    ImGui::SetCursorPos(ImVec2(windowWidth - 60, 25));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));          // Прозрачный фон
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 0.4f)); // Красный отсвет при наведении
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.6f));  // Ярко-красный при клике
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

    if (ImGui::Button("X", ImVec2(50, 50)))
    {
        *done = true; // Закрываем цикл приложения
    }

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
        static char reg_key[64] = ""; // Лицензионный ключ

        ImGui::PushFont(fontRegular);
        ImGui::Text("Username");
        ImGui::InputText("##reg_user", reg_name, IM_ARRAYSIZE(reg_name));

        ImGui::Dummy({ 0, 10 });

        ImGui::Text("Password");
        ImGui::InputText("##reg_pass", reg_pass, IM_ARRAYSIZE(reg_pass), ImGuiInputTextFlags_Password);

        ImGui::Dummy(ImVec2(0, 20));

        if (ImGui::Button("REGISTER", ImVec2(300, 40))) {
            // Здесь логика авторизации
            std::string json_payload = "{\"username\": \"" + std::string(reg_name) + "\", "
                "\"password\": \"" + std::string(reg_pass) + "\"}";
            cpr::Response r = cpr::Post(cpr::Url{ "http://localhost:8000/register" },
                cpr::Header{ { "Content-Type", "application/json" } }, // Указываем, что это JSON
                cpr::Body{ json_payload }                             // Передаем строку
            );
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
void Renderer::drawMain()
{
    // Лого
    ImGui::SetCursorPos(ImVec2(30, 60));
    ImGui::Image((void*)logoTexture, ImVec2(140, 140));
    // Кнопка закрытия окна
    float windowWidth = ImGui::GetIO().DisplaySize.x;
    ImGui::SetCursorPos(ImVec2(windowWidth - 60, 25));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));          // Прозрачный фон
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 0.4f)); // Красный отсвет при наведении
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.6f));  // Ярко-красный при клике
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

    if (ImGui::Button("X", ImVec2(50, 50)))
    {
        *done = true; // Закрываем цикл приложения
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
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
    ImGui::SetCursorPos({ 450, 30 });
    switch (active_tab)
    {
    case 0: // Страница Injection
    {
        // Заголовок
        ImGui::PushFont(fontTitle);
        ImGui::Text("INJECTION");
        ImGui::SetCursorPos({ 300, 100 });
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
            for (std::string method : injector.INJECTION_METHODS)
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
            injector.setDllPath(current_dll);
            injector.setPID(current_pid);
            injector.setThreadId(18888);
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
}
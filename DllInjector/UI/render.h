#pragma once
#include <d3d11.h>
#include "../processManager.h"
#include "../Injector.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "qrcodegen.hpp"


enum AuthState { AUTH_LOGIN, AUTH_REGISTER, AUTH_2FA, AUTH_SUCCESS };

class Renderer
{
public:
	Renderer(bool* doneOrNot, ImGuiIO& io, AuthState* auth_state);
	~Renderer() {};

	void drawLogin();
	void drawRegister();
	void draw2FA();
	void drawMain();

	void drawInjectionContent();
	void drawSettingsContent();
	void drawAboutContent();
	
	bool* done;
private:
	ProcessManager pm;
	std::vector<Process> processes;
	ID3D11ShaderResourceView* logoTexture = nullptr;
	ImFont* fontTitle;
	ImFont* fontBold;
	ImFont* fontRegular;
	std::string current_process;
	int active_tab = 0;
	int current_pid = 0;
	std::wstring current_dll = L"C://Injected.dll";
	std::string current_method = "standard injection";
	Injector injector;
	AuthState* current_auth_state;
	// Для 2FA
	std::string temp_2fa_token;   // временный токен после логина
	std::string twofa_code;       // вводимый код
	std::string twofa_status;     // сообщения об ошибках

	// Для включения 2FA (в настройках)
	bool showing_2fa_setup = false;
	std::string pending_secret;
	std::string pending_uri;
	std::string verify_2fa_code;
	std::string jwt_token;   // храним JWT после успешного входа
	ID3D11ShaderResourceView* qrTexture = nullptr; // Добавьте эту строку
	static ID3D11ShaderResourceView* generateQRTexture(ID3D11Device* device, const std::string& text, int pixels_per_module = 4);
};
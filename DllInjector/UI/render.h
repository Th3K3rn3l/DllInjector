#pragma once
#include <d3d11.h>
#include "../processManager.h"
#include "../Injector.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

enum AuthState { AUTH_LOGIN, AUTH_REGISTER, AUTH_SUCCESS };

class Renderer
{
public:
	Renderer(bool* doneOrNot, ImGuiIO& io, AuthState* auth_state);
	~Renderer() {};

	void drawLogin();
	void drawRegister();
	void drawMain();
	
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
};
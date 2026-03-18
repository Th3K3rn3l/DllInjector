#include <iostream> // для работы с консолью
#define NOMINMAX
#include <Windows.h> // для работы с winapi
#include "logo.h" // для ASCII ART
#include "processManager.h" // менеджер управления процессами
#include "Injector.h" // инжектор


int main()
{
	//--------------------------------------- рисуем ЛОГО ---------------------------------------
	std::wcout << logo << L"\n";
	std::wcout << L"[FSOCIETY CORP] All rights reserved\n";
	std::wcout << L"[FSOCIETY CORP] Confidential access only\n";
	// Устанавливаем локаль
	setlocale(LC_ALL, "Russian");
	system("chcp 1251"); // настраиваем кодировку консоли
	ProcessManager pm;
	Injector injector;
	// ----------------------------------- Получаем информацию -----------------------------
	// Получаем данные от пользователя
	std::wstring processName;
	int choice;
	int pID = 0;
	std::wstring fullDllPath;
	std::wcout << L"Enter 1 if you want to give PID\nEnter 2 if you want to give process name ";
	std::wcin >> choice;
	if (choice == 1)
	{
		std::wcout << L"Enter PID of target process ";
		std::wcin >> pID;
	}
	else
	{
		std::wcout << L"Enter process name ";
		std::wcin >> processName;
		pID = pm.getPIDbyName(processName);
		if (!pID) return 1;
	}
	injector.setPID(pID);
	std::wcout << L"Enter DLL absolute path: ";
	std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n');
	std::getline(std::wcin, fullDllPath);
	injector.setDllPath(fullDllPath);

	std::wcout << L"You have entered this path for dll: " << fullDllPath << "\n";

	//--------------------------------------- Инжект -------------------------------------------
	//injector.standardInject();
	injector.threadHijacking();
	return 0;
}
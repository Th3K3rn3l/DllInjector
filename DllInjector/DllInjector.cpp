#include <iostream> // для работы с консолью
#include <Windows.h> // для работы с winapi
#include "logo.h" // для ASCII ART

int main()
{
	//--------------------------------------- рисуем ЛОГО ---------------------------------------
	std::cout << logo << "\n";
	std::cout << "[FSOCIETY CORP] All rights reserved" << "\n";
	std::cout << "[FSOCIETY CORP] Confidential access only" << "\n";
	// ---------------------------------------- Инжект ------------------------------------------
	// Получаем данные от пользователя
	int pID = 0;
	std::string fullDllPath = "";
	std::cout << "Enter target PID: ";
	std::cin >> pID;
	std::cout << "Enter DLL absolute path: ";
	std::getline(std::cin >> std::ws, fullDllPath);

	// Делаем Standard Injection
	HANDLE hTarget = OpenProcess(PROCESS_CREATE_THREAD // для создания потока
		| PROCESS_VM_OPERATION // для VirtualProtectEx
		| PROCESS_VM_READ // для чтения памяти процесса
		| PROCESS_VM_WRITE // для изменения памяти процесса
		, false, // дочерним процессам не нужен этот ключ
		pID // уникальный идентификатор процесса
	);
	if (!hTarget)
	{
		std::cout << "Не удалось получить дескриптор процесса" << "\n";
		return 1;
	}
	void* tAdress = VirtualAllocEx(
		hTarget, // ключ к процессу
		nullptr, // Это желаемый адрес начала области
		fullDllPath.size() + 1, // сколько байт нужно выделить
		MEM_COMMIT | MEM_RESERVE, // Зарезервировать диапазон виртуальных адресов
		// и сделать так чтобы виртуальные адреса указывали на физическую RAM 
		PAGE_READWRITE // права которые должны быть у выделенной области памяти
	);
	if (!tAdress)
	{
		std::cout << "Не получилось выделить память в процессе" << "\n";
		CloseHandle(hTarget);
		return 1;
	}
	bool result = WriteProcessMemory(
		hTarget, // ключ от процесса
		tAdress, // адрес в целевом процессе куда писать
		fullDllPath.c_str(), // адрес начала данных которые нужно писать
		fullDllPath.size() + 1, // сколько байт нужно писать 
		nullptr // куда положить сколько байт удалось записать
	);
	if (!result)
	{
		std::cout << "Не удалось написать в память целевого процесса" << "\n";
		CloseHandle(hTarget);
	}
	LPVOID loadLibAdress = GetProcAddress(
		GetModuleHandleA("kernel32.dll"), // базовый адрес модуля
		"LoadLibraryA" // название функции
	);
	HANDLE hThread = CreateRemoteThread(
		hTarget, // ключ к процессу
		nullptr, // правила безопасности для ключа нового потока
		0, // размер стека в байтах
		(LPTHREAD_START_ROUTINE)loadLibAdress, // адрес функции которую нужно вызвать в потоке
		tAdress, // адрес объекта который нужно передать в функцию потока
		0, // поток выполняется сразу после создания
		nullptr // адрес куда записать идентификатор потока
	);

	if (hThread)
	{
		WaitForSingleObject(hThread, INFINITE);
		DWORD dllBase = 0;
		GetExitCodeThread(hThread, &dllBase);
		if (dllBase == 0)
		{
			std::cout << "DLL not loaded (LoadLibrary returned 0)\n";
		}
		else
		{
			std::cout << "DLL loaded at: 0x" << std::hex << dllBase << "\n";
		}

		CloseHandle(hThread);
	}
	else
	{
		std::cout << "Injection error" << "\n";
	}
	CloseHandle(hTarget);
	return 0;
}
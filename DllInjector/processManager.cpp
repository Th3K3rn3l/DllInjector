#include "processManager.h"
#include <Windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>

std::vector<Process> ProcessManager::getProcesses()
{
	std::vector<Process> processes;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
	{
		std::wcout << L"Cannot get process snapshot\n";
		return processes;
	}
	PROCESSENTRY32 pe{};
	pe.dwSize = sizeof(pe);
	if (Process32First(snapshot, &pe)) {
		do {
			Process process;
			process.ID = pe.th32ProcessID;
			process.name = pe.szExeFile;
			processes.push_back(process);

		} while (Process32Next(snapshot, &pe));
	}

	CloseHandle(snapshot);
	return processes;
}



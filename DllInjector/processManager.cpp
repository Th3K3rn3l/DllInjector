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

ULONGLONG FileTimeToULL(const FILETIME& ft)
{
	// Складывает 2 части времени в одно число
	return (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

ULONGLONG ProcessManager::FindMaxCpuThread(int PID)
{
	// Получаем список всех потоков
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
	{
		std::cout << "Failed to create thread snapshot\n";
		return;
	}
	// Переменные
	THREADENTRY32 te{};
	te.dwSize = sizeof(te);
	// Для хранения ID потока с максимальным временем CPU и хранения времени CPU
	DWORD maxThreadId = 0;
	ULONGLONG maxCpuTime = 0;
	// Ищем поток с максимальным CPU TIME
	if (Thread32First(snapshot, &te))
	{
		do
		{
			if (te.th32OwnerProcessID == PID) // Поток нашего процесса
			{
				HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
				if (hThread)
				{
					FILETIME creation, exit, kernel, user;

					if (GetThreadTimes(hThread, &creation, &exit, &kernel, &user))
					{
						// CPU время потока
						ULONGLONG totalTime = 
							FileTimeToULL(kernel) + FileTimeToULL(user);

						if (totalTime > maxCpuTime)
						{
							maxCpuTime = totalTime;
							maxThreadId = te.th32ThreadID;
						}
					}

					CloseHandle(hThread);
				}
			}

		} while (Thread32Next(snapshot, &te));
	}
	return maxThreadId;
	CloseHandle(snapshot);
}

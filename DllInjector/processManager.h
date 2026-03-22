#pragma once
#include "vector"
#include <string>
#include <windows.h>


struct Process
{
	int ID;
	std::wstring name;
};


class ProcessManager
{
public:
	ProcessManager() {};
	~ProcessManager() {};

	std::vector<Process> getProcesses();
	ULONGLONG FindMaxCpuThread(int PID);
};

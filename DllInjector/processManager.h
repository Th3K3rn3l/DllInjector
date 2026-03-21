#pragma once
#include "vector"
#include <string>

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

};

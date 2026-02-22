#pragma once

#include <string>

class Injector
{
public:
	Injector() {};
	~Injector() {};

	std::wstring getDllPath();
	void setDllPath(std::wstring absFullPath);
	int getPID();
	void setPID(int pid);

	bool standardInject();
private:
	std::wstring absDllPath;
	int pID;
};
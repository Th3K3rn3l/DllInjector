#include <Windows.h>
#include <iostream>

int main()
{
	std::wstring fullPath;
	std::wcin >> fullPath;
	std::wcout << L"You entered this path: " << fullPath << "\n";
	HMODULE result = LoadLibraryW(fullPath.c_str());
	if (!result)
	{
		std::cout << "Not working\n";
	}
	else
	{
		std::cout << "Working\n";
	}
}
// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"
#include <cstdio>
#include <iostream>

DWORD WINAPI MainThread(HMODULE hmodule)
{
    AllocConsole(); // открыть консоль

    // перенаправить стандартные потоки в консоль
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONIN$", "r", stdin);
    freopen_s(&f, "CONERR$", "w", stderr);

    std::cout << R"($$\                                     $$\                     $$\       
\__|                                    $$ |                    $$ |      
$$\ $$$$$$$\  $$\  $$$$$$\   $$$$$$$\ $$$$$$\    $$$$$$\   $$$$$$$ |      
$$ |$$  __$$\ \__|$$  __$$\ $$  _____|\_$$  _|  $$  __$$\ $$  __$$ |      
$$ |$$ |  $$ |$$\ $$$$$$$$ |$$ /        $$ |    $$$$$$$$ |$$ /  $$ |      
$$ |$$ |  $$ |$$ |$$   ____|$$ |        $$ |$$\ $$   ____|$$ |  $$ |      
$$ |$$ |  $$ |$$ |\$$$$$$$\ \$$$$$$$\   \$$$$  |\$$$$$$$\ \$$$$$$$ |      
\__|\__|  \__|$$ | \_______| \_______|   \____/  \_______| \_______|      
        $$\   $$ |                                                        
        \$$$$$$  |                                                        
         \______/                                                         )";
    FreeConsole();
    return 0;
}




BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


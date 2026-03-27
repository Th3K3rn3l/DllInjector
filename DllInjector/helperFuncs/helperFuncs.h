#pragma once
#include <windows.h>
#include <string>
#include <d3d11.h>

std::wstring OpenFileDialogW();

std::string GetMachineHWID();

bool LoadTextureFromMemory(const unsigned char* data, size_t data_size, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);
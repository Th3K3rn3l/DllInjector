#include "helperFuncs.h"
#include <sstream>  // Обязательно для stringstream
#include <iomanip>  // Обязательно для setfill и setw
#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image/stb_image.h"
#include <d3d11.h>
extern ID3D11Device* g_pd3dDevice; // ХЗ 


std::wstring OpenFileDialogW() {
    wchar_t szFile[260] = { 0 }; // Используем wchar_t вместо char
    OPENFILENAMEW ofn;           // Используем структуру W
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);

    // Строки фильтра тоже должны быть широкими (L"...")
    ofn.lpstrFilter = L"DLL Files (*.dll)\0*.dll\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn)) {
        return std::wstring(szFile); // Возвращаем wstring
    }
    return L"";
}

std::string GetMachineHWID() {
    DWORD serialNumber = 0;

    // GetVolumeInformationW работает с системным диском C:
    // Мы передаем NULL в параметры, которые нам не нужны (имя метки, файловая система и т.д.)
    if (GetVolumeInformationW(L"C:\\", NULL, 0, &serialNumber, NULL, NULL, NULL, 0)) {
        std::stringstream ss;
        // Форматируем число в HEX (например: 0x1234ABCD -> "1234ABCD")
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << serialNumber;
        return ss.str();
    }

    return "UNKNOWN_HWID";
}

bool LoadTextureFromMemory(const unsigned char* data, size_t data_size, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    int image_width = 0;
    int image_height = 0;
    int channels = 0;
    unsigned char* rgba_data = stbi_load_from_memory(data, (int)data_size, &image_width, &image_height, &channels, 4);

    if (rgba_data == nullptr) return false;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = rgba_data;
    subResource.SysMemPitch = image_width * 4;

    ID3D11Texture2D* pTexture = nullptr;
    HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
    if (SUCCEEDED(hr))
    {
        g_pd3dDevice->CreateShaderResourceView(pTexture, nullptr, out_srv);
        pTexture->Release();
        *out_width = image_width;
        *out_height = image_height;
    }

    stbi_image_free(rgba_data);
    return SUCCEEDED(hr);
}

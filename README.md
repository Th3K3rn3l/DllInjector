<p align="center">
  <img src="https://img.shields.io/badge/DLL-INJECTOR-blue?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="Dll Injector Logo">
</p>

<h1 align="center">💉 Dll Injector</h1>

<p align="center">
  <strong>A high-performance, lightweight DLL injection utility built with a focus on stability and modern UX.</strong>
  <br />
  <sub>Engineered in C++ with a custom-styled Dear ImGui interface.</sub>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Platform-Windows-0078D4?style=flat-square&logo=windows" alt="Windows">
  <img src="https://img.shields.io/badge/Language-C%2B%2B-00599C?style=flat-square&logo=c%2B%2B" alt="C++">
  <img src="https://img.shields.io/badge/UI-Dear_ImGui-white?style=flat-square&logo=imgui" alt="ImGui">
  <img src="https://img.shields.io/badge/License-MIT-green?style=flat-square" alt="License">
</p>

---

## ✨ Key Features

* **Modern Aesthetics** Features a custom light-themed UI (**White/Black/Blue**) using the **Orbitron** font for a premium, professional look.
* **System-Level Control** Low-level implementation for precise memory manipulation and process handling.
* **Refined UX** Seamless transitions between selectable elements—no standard clunky rectangles or visual noise.

## 🛠 Supported Injection Methods

The injector currently supports two robust methods:

| Method | Description | Benefit |
| :--- | :--- | :--- |
| **LoadLibrary** | Remote thread creation (Standard) | High stability & reliability |
| **Thread hijacking** | Hijacking remote thread | Stealthier, bypasses basic detection |

---

## 🚀 Getting Started

### Prerequisites
* **OS:** Windows 10/11 (x64)
* **IDE:** [Visual Studio 2022](https://visualstudio.microsoft.com/) (Desktop development with C++)
* **Dependencies:** DirectX SDK (for ImGui rendering)

### Installation & Build
```bash
# 1. Clone the repository
git clone [https://github.com/YourUsername/OnyxInjector.git](https://github.com/YourUsername/OnyxInjector.git)

# 2. Open the .sln file in Visual Studio
# 3. Set build configuration to Release | x64
# 4. Build the solution (Ctrl + Shift + B)

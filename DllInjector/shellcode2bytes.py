#!/usr/bin/env python3
"""
Shellcode Generator for NASM with offset detection
Usage: python asm_to_shellcode.py shell_code.asm
"""

import subprocess
import sys
import os
import re
import tempfile

def asm_to_shellcode(asm_file):
    """
    Компилирует .asm файл с NASM и возвращает shellcode в разных форматах
    """
    # Проверяем существование файла
    if not os.path.exists(asm_file):
        print(f"❌ Ошибка: Файл {asm_file} не найден!")
        return False
    
    # Создаем временные файлы
    temp_dir = tempfile.gettempdir()
    base_name = os.path.basename(asm_file).replace('.asm', '')
    bin_file = os.path.join(temp_dir, f"{base_name}_{os.getpid()}.bin")
    lst_file = os.path.join(temp_dir, f"{base_name}_{os.getpid()}.lst")
    
    print(f"\n🔧 Компиляция {asm_file}...")
    print("-" * 60)
    
    # Компилируем с NASM в бинарник, захватывая вывод (включая warning'и)
    result = subprocess.run(
        ['nasm', '-f', 'bin', asm_file, '-o', bin_file, '-l', lst_file],
        capture_output=True,
        text=True
    )
    
    # Парсим warning'и для получения смещений
    offsets = {}
    warning_pattern = r'warning:\s*(.*?):\s*(\d+)\s*\(0x([0-9A-Fa-f]+)\)'
    
    if result.stderr:
        print("📢 Вывод NASM (warning'и):")
        print(result.stderr)
        
        # Извлекаем смещения из warning'ов
        for line in result.stderr.split('\n'):
            match = re.search(warning_pattern, line, re.IGNORECASE)
            if match:
                name = match.group(1).strip()
                decimal = match.group(2)
                hex_val = match.group(3)
                offsets[name] = {'dec': int(decimal), 'hex': hex_val.upper()}
    
    if result.returncode != 0:
        print("❌ Ошибка компиляции NASM:")
        print(result.stderr)
        return False
    
    # Читаем бинарный файл
    with open(bin_file, 'rb') as f:
        shellcode_bytes = f.read()
    
    print(f"\n📊 Информация о shellcode:")
    print("-" * 60)
    print(f"📦 Размер: {len(shellcode_bytes)} байт (0x{len(shellcode_bytes):X})")
    
    # Показываем найденные смещения
    if offsets:
        print("\n📍 Смещения (для инжектора):")
        for name, values in offsets.items():
            print(f"   {name:25} : {values['dec']:4} (0x{values['hex']})")
    else:
        print("\n⚠️  Смещения не найдены. Добавьте в .asm файл:")
        print('   %warning "dll_path offset: " dll_path_offset')
        print('   %warning "loadlibrary_ptr offset: " loadlibrary_ptr_offset')
    
    # Форматируем как C массив (по 16 байт в строке)
    c_array = "BYTE shellcode[] = {\n    "
    for i, b in enumerate(shellcode_bytes):
        if i > 0 and i % 16 == 0:
            c_array += "\n    "
        c_array += f"0x{b:02X}, "
    c_array = c_array[:-2] + "\n};"
    
    # Генерируем заголовочный файл со смещениями
    if offsets:
        h_file = os.path.join(temp_dir, f"{base_name}_offsets.h")
        with open(h_file, 'w') as f:
            f.write("// Auto-generated shellcode offsets\n")
            f.write("// Generated from: " + os.path.basename(asm_file) + "\n\n")
            f.write(f"#define SHELLCODE_SIZE {len(shellcode_bytes)}\n")
            for name, values in offsets.items():
                clean_name = name.replace(' ', '_').upper()
                f.write(f"#define {clean_name}_OFFSET 0x{values['hex']}\n")
        print(f"\n📁 Временный заголовочный файл: {h_file}")
        print("   Скопируйте нужные #define в ваш код")
    
    # Выводим результаты
    print("\n" + "="*70)
    print("✅ РЕЗУЛЬТАТЫ")
    print("="*70)
    
    print("\n📌 C/C++ BYTE массив (для вставки в код):")
    print("-" * 60)
    print(c_array)
    
    print("\n📌 Python bytes объект:")
    print("-" * 60)
    py_bytes = 'b"' + ''.join(f'\\x{b:02x}' for b in shellcode_bytes) + '"'
    print(py_bytes)
    
    print("\n📌 PowerShell массив:")
    print("-" * 60)
    ps_array = '@(' + ', '.join(f'0x{b:02X}' for b in shellcode_bytes) + ')'
    print(ps_array)
    
    print("\n📌 Hex строка:")
    print("-" * 60)
    hex_string = ''.join(f'\\x{b:02x}' for b in shellcode_bytes)
    print(hex_string)
    
    print("\n📌 Сырые байты (hex dump):")
    print("-" * 60)
    for i in range(0, len(shellcode_bytes), 16):
        chunk = shellcode_bytes[i:i+16]
        hex_part = ' '.join(f'{b:02X}' for b in chunk)
        ascii_part = ''.join(chr(b) if 32 <= b <= 126 else '.' for b in chunk)
        print(f"0x{i:04X}: {hex_part:<48} {ascii_part}")
    
    print("\n" + "="*70)
    print(f"✅ Shellcode сгенерирован! Размер: {len(shellcode_bytes)} байт")
    
    # Удаляем временные файлы
    try:
        os.remove(bin_file)
        os.remove(lst_file)
        print(f"🧹 Временные файлы удалены")
    except:
        pass
    
    print("="*70)
    return True

def main():
    if len(sys.argv) < 2:
        print("🔨 SHELLCODE GENERATOR FOR NASM")
        print("=" * 50)
        print("Использование: python asm_to_shellcode.py <файл.asm>")
        print("\nПримеры:")
        print("  python asm_to_shellcode.py shellcode.asm")
        print("  python asm_to_shellcode.py payload.asm")
        print("\nДля отображения смещений добавьте в .asm файл:")
        print('  %warning "dll_path offset: " dll_path_offset')
        print('  %warning "loadlibrary_ptr offset: " loadlibrary_ptr_offset')
        return
    
    asm_file = sys.argv[1]
    asm_to_shellcode(asm_file)

if __name__ == "__main__":
    main()
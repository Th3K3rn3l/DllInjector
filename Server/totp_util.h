#pragma once
#include <string>
#include <chrono>

namespace totp {
    // Генерирует случайный секрет (base32) заданной длины (по умолчанию 32 байта -> 52 символа base32)
    std::string generate_secret(size_t bytes = 32);

    // Вычисляет 6-значный TOTP-код из base32-секрета для текущего времени (шаг 30 сек)
    std::string get_code(const std::string& secret);

    // Проверяет введённый код (допускает отклонение ±1 шаг времени)
    bool verify(const std::string& secret, const std::string& user_code);

    // Формирует otpauth URI для Google Authenticator
    std::string make_uri(const std::string& username, const std::string& secret, const std::string& issuer = "DLLInjector");
}
#include "totp_util.h"
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstring>
#include <ctime>
#include <cmath>

static const std::string base32_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

static std::string base32_encode(const uint8_t* data, size_t len) {
    std::string out;
    int buffer = 0, bits = 0;
    for (size_t i = 0; i < len; ++i) {
        buffer = (buffer << 8) | data[i];
        bits += 8;
        while (bits >= 5) {
            out += base32_chars[(buffer >> (bits - 5)) & 0x1F];
            bits -= 5;
        }
    }
    if (bits > 0)
        out += base32_chars[(buffer << (5 - bits)) & 0x1F];
    return out;
}

static std::vector<uint8_t> base32_decode(const std::string& str) {
    std::vector<uint8_t> out;
    int buffer = 0, bits = 0;
    for (char c : str) {
        if (c == '=') break;
        int val = base32_chars.find(std::toupper(c));
        if (val == std::string::npos) continue;
        buffer = (buffer << 5) | val;
        bits += 5;
        if (bits >= 8) {
            out.push_back((buffer >> (bits - 8)) & 0xFF);
            bits -= 8;
        }
    }
    return out;
}

static uint64_t time_step64() {
    auto now = std::chrono::system_clock::now();
    auto epoch = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch).count();
    return static_cast<uint64_t>(seconds) / 30;
}

static std::string hmac_sha1(const std::vector<uint8_t>& key, const uint8_t* msg, size_t msg_len) {
    unsigned int len = 20;
    uint8_t out[20];
    HMAC(EVP_sha1(), key.data(), key.size(), msg, msg_len, out, &len);
    return std::string(reinterpret_cast<char*>(out), len);
}

static uint32_t dynamic_truncate(const std::string& hmac) {
    int offset = hmac[19] & 0x0F;
    uint32_t code = ((hmac[offset] & 0x7F) << 24) |
        ((hmac[offset + 1] & 0xFF) << 16) |
        ((hmac[offset + 2] & 0xFF) << 8) |
        (hmac[offset + 3] & 0xFF);
    return code % 1000000;
}

std::string totp::generate_secret(size_t bytes) {
    std::vector<uint8_t> rand_bytes(bytes);
    if (RAND_bytes(rand_bytes.data(), bytes) != 1) {
        throw std::runtime_error("RAND_bytes failed");
    }
    return base32_encode(rand_bytes.data(), bytes);
}

std::string totp::get_code(const std::string& secret) {
    auto key = base32_decode(secret);
    uint64_t counter = time_step64();
    uint8_t counter_be[8];
    for (int i = 7; i >= 0; --i) {
        counter_be[i] = counter & 0xFF;
        counter >>= 8;
    }
    std::string hmac = hmac_sha1(key, counter_be, 8);
    uint32_t code = dynamic_truncate(hmac);
    char buf[7];
    snprintf(buf, sizeof(buf), "%06u", code);
    return std::string(buf);
}

bool totp::verify(const std::string& secret, const std::string& user_code) {
    if (secret.empty() || user_code.length() != 6) return false;
    uint64_t cur_step = time_step64();
    for (int64_t offset = -1; offset <= 1; ++offset) {
        uint64_t step = cur_step + offset;
        uint8_t counter_be[8];
        for (int i = 7; i >= 0; --i) {
            counter_be[i] = step & 0xFF;
            step >>= 8;
        }
        auto key = base32_decode(secret);
        std::string hmac = hmac_sha1(key, counter_be, 8);
        uint32_t code = dynamic_truncate(hmac);
        char buf[7];
        snprintf(buf, sizeof(buf), "%06u", code);
        if (user_code == buf) return true;
    }
    return false;
}

std::string totp::make_uri(const std::string& username, const std::string& secret, const std::string& issuer) {
    // otpauth://totp/Issuer:username?secret=...&issuer=Issuer
    std::string encoded_issuer = issuer;
    std::string encoded_user = username;
    // Простейшее URL-кодирование (замените при необходимости)
    for (char& c : encoded_issuer) if (c == ' ') c = '+';
    for (char& c : encoded_user) if (c == ' ') c = '+';
    return "otpauth://totp/" + encoded_issuer + ":" + encoded_user +
        "?secret=" + secret + "&issuer=" + encoded_issuer;
}
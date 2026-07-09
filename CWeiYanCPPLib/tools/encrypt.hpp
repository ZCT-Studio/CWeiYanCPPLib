// Licensed under the GNU Lesser General Public License, Version 2.1

//
// Created by wanjiangzhi on 2026/1/27.
//

#ifndef WEIYAN_ENCRYPT_HPP
#define WEIYAN_ENCRYPT_HPP

#include "../constants.hpp"
#include <string>
#include <vector>
#include <stdexcept>
#include <utility>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER < 0x30000000L
#include <openssl/md5.h>
#else
#include <openssl/evp.h>
#endif


namespace Weiyan {
    class RC4 {
        static constexpr size_t BOX_LEN = 256;
        using ByteArray = std::vector<uint8_t>;

        static void swap_byte(uint8_t& a, uint8_t& b) {
            std::swap(a, b);
        }

        static ByteArray GetKey(const uint8_t* pass, size_t pass_len) {
            ByteArray box(BOX_LEN);
            for (size_t i = 0; i < BOX_LEN; ++i) box[i] = static_cast<uint8_t>(i);

            size_t j = 0;
            for (size_t i = 0; i < BOX_LEN; ++i) {
                j = (pass[i % pass_len] + box[i] + j) % BOX_LEN;
                swap_byte(box[i], box[j]);
            }
            return box;
        }

        static ByteArray Rc4(const uint8_t* data, const size_t data_len, const uint8_t* key, const size_t key_len) {
            ByteArray out(data_len);
            ByteArray box = GetKey(key, key_len);

            size_t x = 0, y = 0;
            for (size_t i = 0; i < data_len; ++i) {
                x = (x + 1) % BOX_LEN;
                y = (box[x] + y) % BOX_LEN;
                swap_byte(box[x], box[y]);
                out[i] = data[i] ^ box[(box[x] + box[y]) % BOX_LEN];
            }
            return out;
        }

        static std::string ByteToHex(const ByteArray& data) {
            std::ostringstream oss;
            for (const auto b : data)
                oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
            return oss.str();
        }

        static ByteArray HexToByte(const std::string& hex) {
            if (hex.size() % 2 != 0) throw std::invalid_argument("Invalid hex string");

            ByteArray out(hex.size() / 2);
            for (size_t i = 0; i < out.size(); ++i) {
                const uint8_t high = static_cast<uint8_t>(std::stoi(hex.substr(i * 2, 1), nullptr, 16));
                const uint8_t low = static_cast<uint8_t>(std::stoi(hex.substr(i * 2 + 1, 1), nullptr, 16));
                out[i] = (high << 4) | low;
            }
            return out;
        }
    public:
        static std::string Encrypt(const std::string& src, const std::string& password) {
            const ByteArray data(src.begin(), src.end());
            const ByteArray key(password.begin(), password.end());
            const ByteArray enc = Rc4(data.data(), data.size(), key.data(), key.size());
            return ByteToHex(enc);
        }

        static std::string Decrypt(const std::string& hexSrc, const std::string& password) {
            const ByteArray data = HexToByte(hexSrc);
            const ByteArray key(password.begin(), password.end());
            ByteArray dec = Rc4(data.data(), data.size(), key.data(), key.size());
            return {dec.begin(), dec.end()};
        }
    protected:
        const std::string mPwd;
    public:
        explicit RC4(std::string pwd) : mPwd(std::move(pwd)) {}

        std::string Enc(const std::string& src) const {
            return Encrypt(src, mPwd);
        }

        std::string Dec(const std::string& hex_src) const {
            return Decrypt(hex_src, mPwd);
        }
    }; // namespace RC4

    inline std::string md5(const std::string& data) {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');

        #if OPENSSL_VERSION_NUMBER < 0x30000000L
        unsigned char digest[MD5_DIGEST_LENGTH];
        MD5(reinterpret_cast<const unsigned char*>(data.data()), data.size(), digest);

        for (const auto& byte : digest)
            oss << std::setw(2) << static_cast<unsigned int>(byte);

        #else
        std::vector<unsigned char> digest(EVP_MAX_MD_SIZE);
        unsigned int digest_len = 0;

        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx) {
            std::cerr << "Failed to create EVP_MD_CTX" << std::endl;
            return {};
        }

        if (1 != EVP_DigestInit_ex(ctx, EVP_md5(), nullptr) ||
            1 != EVP_DigestUpdate(ctx, data.data(), data.size()) ||
            1 != EVP_DigestFinal_ex(ctx, digest.data(), &digest_len)) {
            EVP_MD_CTX_free(ctx);
            std::cerr << "MD5 calculation failed" << std::endl;
            return {};
        }

        EVP_MD_CTX_free(ctx);

        for (unsigned int i = 0; i < digest_len; ++i)
            oss << std::setw(2) << static_cast<unsigned int>(digest[i]);
        #endif

        return oss.str();
    }


    class Hex {
        std::string data;
    public:
        explicit Hex(std::string  data) : data(std::move(data)) {}

        static std::string Enc(const std::string& input) {
            std::ostringstream oss;
            for (const unsigned char c : input) {
                oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
            }
            return oss.str();
        }

        static std::string Dec(const std::string& hexStr) {
            if (hexStr.length() % 2 != 0)
                throw std::invalid_argument("Hex string length must be even");

            std::string output;
            output.reserve(hexStr.length() / 2);

            for (size_t i = 0; i < hexStr.length(); i += 2) {
                std::string byteStr = hexStr.substr(i, 2);
                const char byte = static_cast<char>(std::stoi(byteStr, nullptr, 16));
                output.push_back(byte);
            }

            return output;
        }

        std::string Enc() const {
            return Enc(data);
        }

        std::string Dec() const {
            return Dec(data);
        }
    };

}

#endif //WEIYAN_ENCRYPT_HPP

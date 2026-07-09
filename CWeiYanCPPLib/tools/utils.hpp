// Licensed under the GNU Lesser General Public License, Version 2.1

//
// Created by wanjiangzhi on 2026/1/27.
//

#ifndef WEIYAN_UTILS_HPP
#define WEIYAN_UTILS_HPP

#include "../constants.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include "json.hpp"

namespace Weiyan {
    template <typename _NumTy> // NOLINT(*-reserved-identifier)
    std::string Num2Str(_NumTy number = 0, const std::string& ifZeroTo = "0") {
        if (number == 0) return ifZeroTo;
        return std::to_string(number);
    }

    template <typename _Ty0> // NOLINT(*-reserved-identifier)
    _Ty0 GetData(const json& j, const std::string& path, const _Ty0& def = _Ty0{}, bool asPath = false) {
        std::string input = path;

        for (size_t i = 0; i + 2 < input.size(); ++i) {
            if (input[i] == '<' && input[i + 1] == '.' && input[i + 2] == '>') {
                input.replace(i, 3, ".");
                i += 2;
            }
        }

        const bool isPath = asPath || input.find('.') != std::string::npos;

        const json* currentJson = &j;

        if (isPath) {
            std::vector<std::string> parts;
            std::string current;

            for (size_t i = 0; i < input.size(); ++i) {
                const char c = input[i];
                if (c == '.') {
                    if (!current.empty()) {
                        parts.push_back(current);
                        current.clear();
                    }
                } else if (c == '[') {
                    if (!current.empty()) {
                        parts.push_back(current);
                        current.clear();
                    }
                    const size_t end = input.find(']', i);
                    if (end == std::string::npos) return def;
                    parts.push_back(input.substr(i + 1, end - (i + 1)));
                    i = end;
                } else { current += c; }
            }
            if (!current.empty()) parts.push_back(current);

            for (const auto& part : parts) {
                if (currentJson->is_object()) {
                    if (currentJson->contains(part)) currentJson = &(*currentJson)[part];
                    else return def;
                } else if (currentJson->is_array()) {
                    if (!part.empty() && std::all_of(part.begin(), part.end(), ::isdigit)) {
                        const size_t idx = std::stoul(part);
                        if (idx < currentJson->size()) currentJson = &(*currentJson)[idx];
                        else return def;
                    } else return def;
                } else return def;
            }
        } else {
            if (!currentJson->contains(input)) return def;
            currentJson = &(*currentJson)[input];
        }

        try {
            return currentJson->get<_Ty0>();
        } catch (...) {
            return def;
        }
    }
}

#endif //WEIYAN_UTILS_HPP

// Licensed under the GNU Lesser General Public License, Version 2.1

//
// Created by wanjiangzhi on 2026/1/27.
//

#ifndef WEIYAN_REQUEST_HPP
#define WEIYAN_REQUEST_HPP

#include "macro.hpp"
#include <iostream>
#include <string>
#include <curl/curl.h>

namespace Weiyan {

    namespace request {
        namespace type {
            enum CURL_BOOL : long {
                CURL_FALSE = 0L,
                CURL_TRUE  = 1L
            };
        }

        struct base;

        template <typename _Ty = void> // NOLINT(*-reserved-identifier)
        class curl_Adapter;

        template <>
        class curl_Adapter<void> {
        public:
            explicit curl_Adapter() = delete;

            static void check_error(const CURLcode code) {
                if (code != CURLE_OK)
                    WEIYAN_DEBUG(
                    std::cerr << "curl_easy_perform() failed: "
                    << curl_easy_strerror(code) << std::endl
                );
            }

            class SetOpt {
                CURL** curl_ptr;
                curl_slist* headers = nullptr;
            protected:
                SetOpt& Self() {
                    return *this;
                }

                SetOpt* Ptr() {
                    return this;
                }

            public:
                explicit SetOpt(CURL** curl_p) : curl_ptr(curl_p) {}
                ~SetOpt() = default;

                template <typename _Arg> // NOLINT(*-reserved-identifier)
                SetOpt& operator()(const CURLoption& opt, _Arg arg) {
                    if (curl_ptr && *curl_ptr) curl_easy_setopt(*curl_ptr, opt, std::forward<_Arg>(arg));
                    return Self();
                }

                SetOpt& form(const std::string& formData) {
                    return operator()(CURLOPT_POST, 1L)
                           (CURLOPT_POSTFIELDS, formData.c_str())
                           (CURLOPT_POSTFIELDSIZE, static_cast<long>(formData.length()));
                }

                SetOpt& json(const std::string& jsonData) {
                    if (curl_ptr && *curl_ptr) {
                        // 设置 POST
                        (*this)(CURLOPT_POST, 1L)
                               (CURLOPT_POSTFIELDS, jsonData.c_str())
                               (CURLOPT_POSTFIELDSIZE, static_cast<long>(jsonData.length()));

                        headers = curl_slist_append(headers, "Content-Type: application/json");
                        curl_easy_setopt(*curl_ptr, CURLOPT_HTTPHEADER, headers);
                    }
                    return Self();
                }
            };
        };

        template <>
        class curl_Adapter<base> {
        protected:
            using base = curl_Adapter;

            using public_methods = curl_Adapter<>;

            CURL* curl;

            void ToDoGet(const std::string& link) {
                SetOpt(CURLOPT_URL, link.c_str())(CURLOPT_HTTPGET, type::CURL_TRUE)(CURLOPT_POST, type::CURL_FALSE);
                public_methods::check_error(curl_easy_perform(curl));
            }

            void ToDoPost(const std::string& link) {
                auto* headers = curl_slist_append(nullptr, "Content-Type: application/json");
                SetOpt(CURLOPT_URL, link.c_str())(CURLOPT_POST, type::CURL_TRUE)(CURLOPT_HTTPGET, type::CURL_FALSE)(CURLOPT_HTTPHEADER, headers);

                public_methods::check_error(curl_easy_perform(curl));

                curl_slist_free_all(headers);
            }

        public:
            public_methods::SetOpt SetOpt;

            explicit curl_Adapter(CURL* handle = nullptr) : curl(handle),
                                                            SetOpt(&curl) {
                if (!curl) curl = curl_easy_init();
                if (!curl)
                    WEIYAN_ASSERT(true, "can not init by curl_easy_init()");
            }

            curl_Adapter(const curl_Adapter&) = delete;

            curl_Adapter(curl_Adapter&&) noexcept = default;

            ~curl_Adapter() {
                CleanUp();
            }

            void CleanUp() {
                if (curl) curl_easy_cleanup(curl);
                curl = nullptr;
            }

            [[nodiscard]] const CURL* read_only_handle() const {
                return curl;
            }

            [[nodiscard]] CURL* handle() const {
                return curl;
            }
        };
        
        template <>
        class curl_Adapter<std::string> : public curl_Adapter<base> {
            static size_t receive_Response(const char* contents, const std::size_t size, const std::size_t nmemb, void* userp) {
                const size_t total = size * nmemb;
                static_cast<std::string*>(userp)->append(contents, total);
                return total;
            }

        public:
            std::string result;

            explicit curl_Adapter(CURL* handle = nullptr) : base(handle) {
                SetOpt(CURLOPT_WRITEFUNCTION, receive_Response)(CURLOPT_WRITEDATA, &result);
            }

            using base::curl_Adapter;

            void Clear() {
                result.clear();
            }

            std::string& Get(const std::string& url, const std::string& param = {}) {
                Clear();
                ToDoGet(url + param);
                return result;
            }

            std::string& Post(const std::string& url, const std::string& param = {}) {
                Clear();
                ToDoPost(url + param);
                return result;
            }
        };

        inline std::string get(
            const std::string& url,
            const std::string& param = {}
        ) {
            curl_Adapter<std::string> GetAdapter;
            GetAdapter.SetOpt(CURLOPT_TIMEOUT, 15L);
            GetAdapter.Get(url, param);
            return GetAdapter.result;
        }

        inline std::string post(
            const std::string& url,
            const std::string& param = {},
            const std::string& data = {}  // 默认空数据为 JSON 空对象
        ) {
            curl_Adapter<std::string> PostAdapter;

            PostAdapter.SetOpt(CURLOPT_TIMEOUT, 15L);

            curl_slist* slist = nullptr;
            slist = curl_slist_append(slist, "Content-Type: application/x-www-form-urlencoded");
            PostAdapter.SetOpt(CURLOPT_HTTPHEADER, slist);

            PostAdapter.SetOpt(CURLOPT_POST, 1L);
            PostAdapter.SetOpt(CURLOPT_POSTFIELDS, data.c_str());
            PostAdapter.SetOpt(CURLOPT_POSTFIELDSIZE, static_cast<long>(data.size()));

            PostAdapter.Post(url + param);

            if (slist) curl_slist_free_all(slist);

            return PostAdapter.result;
        }


    }

}

#endif //WEIYAN_REQUEST_HPP

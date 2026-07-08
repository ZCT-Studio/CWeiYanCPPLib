// Licensed under the GNU Lesser General Public License, Version 2.1

//
// Created by wanjiangzhi on 2026/1/27.
//

#ifndef WEIYAN_NOTICE_HPP
#define WEIYAN_NOTICE_HPP

#include <chrono>
#include "context.hpp"
#include "tools/url.hpp"
#include "tools/json.hpp"
#include "tools/types.hpp"
#include "tools/request.hpp"
#include "tools/encrypt.hpp"

namespace Weiyan {
    class Notice {
        Context& mCtx;
    public:
        json data;

        Context& Ctx() const {
            return mCtx;
        }

        Notice& Self() {
            return *this;
        }

        /**
         * 公告对象，用于获取公告
         * @param ctx 上下文共享智能指针类
         * @param defaultInit 是否在创建对象的时候就获取公告（默认为false）
         */
        explicit Notice(Context& ctx, const bool defaultInit = false) : mCtx(ctx) {
            if (defaultInit) Get();
        }

        /**
         * 获取公告
         * @return 响应的json解密数据
         */
        json& Get() {
            const json_result j{
                mCtx->RC4.Dec(
                    request::post(
                        mCtx->API(),
                        URL::Params(
                            {
                                {"id", "notice"},
                                {"app", mCtx->appID()}
                            }
                        )
                    )
                )
            };

            if (j)
                data = *j;

            return data;
        }

        /**
         * 获取是否解绑成功
         * @return 解绑状态
         */
        bool success() const {
            return code() == 200;
        }

        /**
         * 获取自定义检查码
         * @return 检查码字符串
         */
        std::string check() const {
            if (!data.contains("check")) return {};
            return data["check"].get<std::string>();
        }

        /**
         * 获取HTTP响应码
         * @return HTTP响应码
         */
        int code() const {
            if (!data.contains("code")) return -1;
            return data["code"];
        }

        /**
         * 获取公告提示信息（如果没有返回空字符串）
         * @return 提示信息
         */
        std::string msg() const {
            if (!data.contains("msg") || data["msg"].is_object()) return {};
            return data["msg"].get<std::string>();
        }

        /**
         * 获取公告（如果没有返回空字符串）
         * @return 公告
         */
        std::string content() const {
            if (!data.contains("msg") || !data["msg"].is_object() || !data["msg"].contains("app_gg")) return {};
            return data["msg"]["app_gg"].get<std::string>();
        }

        /**
         * 获取响应时间戳（C++）
         * @return C++风格时间戳
         */
        timestamp time() const {
            if (!data.contains("time")) return {};
            return timestamp::build(std::chrono::seconds(data["time"]));
        }
    };
}

#endif //WEIYAN_NOTICE_HPP

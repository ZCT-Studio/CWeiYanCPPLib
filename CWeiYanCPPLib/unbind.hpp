// Licensed under the GNU Lesser General Public License, Version 2.1

//
// Created by wanjiangzhi on 2026/1/27.
//

#ifndef WEIYAN_UNBIND_HPP
#define WEIYAN_UNBIND_HPP

#include "constants.hpp"
#include <chrono>
#include <sstream>
#include "context.hpp"
#include "tools/url.hpp"
#include "tools/json.hpp"
#include "tools/types.hpp"
#include "tools/request.hpp"
#include "tools/encrypt.hpp"

namespace Weiyan {
    class Unbind {
        Context& mCtx;

    public:
        json data;

        Context& Ctx() const {
            return mCtx;
        }

        Unbind& Self() {
            return *this;
        }

        /**
         * 解绑对象，用于卡密解绑
         * @param ctx 上下文共享智能指针类
         */
        explicit Unbind(Context& ctx) : mCtx(ctx) {}

        /**
         * 卡密解绑
         * @param key 卡密
         * @param deviceid 设备标识（不填写默认使用测试ID）
         * @return 响应的json解密数据
         */
        json& operator()(
            const std::string& key,
            const std::string& deviceid = "123456Test"
        ) {
            std::string time = std::to_string(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count()
            );
            std::string sign = [&]() {
                std::stringstream ss;
                ss << "kami=" << key << "&markcode=" << deviceid << "&t=" << time << "&" << mCtx->appKey();
                return md5(ss.str());
            }();

            std::string PostData = mCtx->RC4.Enc(
                URL::Fields(
                    {
                        {"kami", key},
                        {"markcode", deviceid},
                        {"t", std::move(time)},
                        {"sign", std::move(sign)}
                    }
                )
            );

            const json_result j{
                mCtx->RC4.Dec(
                    request::post(
                        mCtx->API(),
                        URL::Params(
                            {
                                {"id", "kmdismiss"},
                                {"app", mCtx->appID()},
                                {"data", std::move(PostData)}
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
         * 获取解绑提示信息（如果没有返回空字符串）
         * @return 提示信息
         */
        std::string msg() const {
            if (!data.contains("msg")) return {};
            return data["msg"].is_object() ?
                success() ?
                    "解绑成功"
                : "未知错误"
            : data["msg"].get<std::string>();
        }

        /**
         * 获取卡密剩余解绑次数
         * @return 解绑次数剩余
         */
        int attempts() const {
            if (!data.contains("msg") || !data["msg"].is_object() || !data["msg"].contains("num")) return -1;
            return data["msg"]["num"].get<int>();
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

#endif //WEIYAN_UNBIND_HPP

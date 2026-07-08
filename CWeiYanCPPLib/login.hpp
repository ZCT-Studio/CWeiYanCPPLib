// Licensed under the GNU Lesser General Public License, Version 2.1

//
// Created by wanjiangzhi on 2026/1/27.
//

#ifndef WEIYAN_LOGIN_HPP
#define WEIYAN_LOGIN_HPP

#include <string>
#include <chrono>
#include <sstream>
#include <random>
#include <unordered_map>
#include "context.hpp"
#include "tools/url.hpp"
#include "tools/json.hpp"
#include "tools/types.hpp"
#include "tools/request.hpp"
#include "tools/encrypt.hpp"

namespace Weiyan {
    class Login {
        Context& mCtx;
        int mSuccess_code;

    protected:
        bool logged = false;
        std::string message;

        bool noMsgArr() const {
            const int localCode = code();
            return localCode == 201 || localCode == -1;
        }
    public:
        json data;

        Context& Ctx() const {
            return mCtx;
        }

        Login& Self() {
            return *this;
        }

        /**
         * 登录对象，用于卡密登录
         * @param ctx 上下文共享智能指针类
         * @param success_code 成功的结果码，可在[应用配置/状态码配置里查看]，默认200
         */
        explicit Login(Context& ctx, const int success_code = 200) : mCtx(ctx),
                                                                     mSuccess_code(success_code) {}

        /**
         * 卡密验证登录
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
            std::string value = []() {
                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<uint32_t> dis(0, 2764472319);
                return std::to_string(dis(gen));
            }();

            std::string PostData = mCtx->RC4.Enc(
                URL::Fields(
                    {
                        {"kami", key},
                        {"markcode", deviceid},
                        {"t", std::move(time)},
                        {"sign", std::move(sign)},
                        {"value", std::move(value)}
                    }
                )
            );

            const json_result j{
                mCtx->RC4.Dec(
                    request::post(
                        mCtx->API(),
                        URL::Params(
                            {
                                {"id", "kmlogon"},
                                {"app", mCtx->appID()},
                                {"data", std::move(PostData)}
                            }
                        )
                    )
                )
            };

            if (j) {
                data = *j;

                auto code = data["code"].get<int>();

                auto GetStatusMessage = [&code, this](const json& j_) {
                    static const std::unordered_map<int, std::string> msg_map = {
                            {this->mSuccess_code, "登录成功"},
                            {100, "未绑定应用 ID"},
                            {102, "应用已关闭"},
                            {104, "签名为空"},
                            {105, "数据过期"},
                            {106, "签名错误"},
                            {107, "数据为空"},
                            {108, "未提交时间变量"},
                            {112, "未提交设备码变量"},
                            {148, "卡密为空"},
                            {149, "卡密不存在"},
                            {150, "卡密已使用"},
                            {152, "卡密已到期"},
                            {153, "卡密被禁用"}
                        };
                    if (noMsgArr())
                        return j_["msg"].get<std::string>();
                    const auto it = msg_map.find(code);
                    return it != msg_map.end() ? it->second : "未知错误";
                };

                logged = code == mSuccess_code;

                message = GetStatusMessage(data);
            }

            return data;
        }

        /**
         * 获取是否登录成功
         * @return 登录状态
         */
        bool success() const {
            return logged;
        }

        /**
         * 获取登录信息
         * @return 登录信息
         */
        const std::string& msg() const {
            return message;
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
         *
         * 示例：
         * 200	登录成功
         * 201	[取msg变量信息]
         * 100	未绑定应用 ID
         * 102	应用已关闭
         * 104	签名为空
         * 105	数据过期
         * 106	签名错误
         * 107	数据为空
         * 108	未提交时间变量
         * 112	未提交设备码变量
         * 148	卡密为空
         * 149	卡密不存在
         * 150	卡密已使用
         * 152	卡密已到期
         * 153	卡密被禁用
         *
         * @return HTTP响应码
         */
        int code() const {
            if (!data.contains("code")) return -1;
            return data["code"];
        }

        /**
         * 获取卡密ID
         * @return 卡密ID
         */
        int key_id() const {
            if (noMsgArr() || !data.contains("msg") || !data["msg"].contains("id")) return -1;
            return data["msg"]["id"].get<int>();
        }

        /**
         * 获取卡密时长类型
         *
         * free	免费模式
         * hour	时卡
         * day	天卡
         * week	周卡
         * month 月卡
         * season 季卡
         * year 年卡
         * longuse 永久卡
         * single 次数卡
         *
         * @return 类型字符串
         */
        std::string key_time_type() const {
            if (noMsgArr() || !data.contains("msg") || !data["msg"].contains("kmtype")) return "unknown";
            return data["msg"]["kmtype"].get<std::string>();
        }

        /**
         * 获取卡密类型(单码:code，次数卡:single)
         * @return 卡密类型字符串
         */
        std::string key_type() const {
            if (noMsgArr() || !data.contains("msg") || !data["msg"].contains("ktype")) return "unknown";
            return data["msg"]["ktype"].get<std::string>();
        }

        /**
         * 获取登录令牌(用于心跳)
         * @return 字符串令牌
         */
        std::string token() const {
            if (noMsgArr() || !data.contains("msg") || !data["msg"].contains("token")) return "unknown";
            return data["msg"]["token"].get<std::string>();
        }

        /**
         * 获取到期时间戳（C++）
         * @return C++风格时间戳
         */
        timestamp expire() const {
            if (noMsgArr() || !data.contains("msg") || !data["msg"].contains("vip")) return {};
            return timestamp::build(std::chrono::seconds(data["msg"]["vip"].get<int>()));
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

#endif //WEIYAN_LOGIN_HPP

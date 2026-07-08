// Licensed under the GNU Lesser General Public License, Version 2.1

//
// Created by wanjiangzhi on 2026/1/27.
//

#ifndef WEIYAN_MACRO_HPP
#define WEIYAN_MACRO_HPP

#define WEIYAN_NS_BEGIN namespace Weiyan {
#define WEIYAN_NS_END }
#define WEIYAN_NS_API ::Weiyan::
#define WEIYAN_API namespace ::Weiyan;

#if defined(NDEBUG) || defined(WEIYAN_NDEBUG)
#define WEIYAN_DEBUG(any_no)
#define WEIYAN_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion helper: " << #cond << "\n" \
                << "Message: " << msg << "\n" \
                << "File: " << __FILE__ << ", Line: " << __LINE__; \
        } \
    } while (0)
#else
#define WEIYAN_DEBUG(any) any
#define WEIYAN_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " << #cond << "\n" \
                << "Message: " << (msg) << "\n" \
                << "File: " << __FILE__ << ", Line: " << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (0)
#endif

#endif //WEIYAN_MACRO_HPP

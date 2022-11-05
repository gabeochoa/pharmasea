
#pragma once

#define SINGLETON_FWD(type) \
    struct type;            \
    static std::shared_ptr<type> type##_single;

#define SINGLETON(type)                                          \
    inline static type* create() { return new type(); }          \
    inline static type& get() {                                  \
        if (!type##_single) type##_single.reset(type::create()); \
        return *type##_single;                                   \
    }                                                            \
    inline static void reset() { type##_single.reset(type::create()); }\

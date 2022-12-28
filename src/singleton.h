
#pragma once

#define SINGLETON_FWD(type) \
    struct type;            \
    static std::shared_ptr<type> type##_single;

#define SINGLETON(type)                                          \
    inline static type* create() { return new type(); }          \
    inline static type& get() {                                  \
        if (!type##_single) type##_single.reset(type::create()); \
        return *type##_single;                                   \
    }

#define SINGLETON_PARAM(type, param_type)                              \
    inline static type* create(param_type pt) { return new type(pt); } \
    inline static type& get_and_create(param_type pt) {                \
        if (!type##_single) type##_single.reset(type::create(pt));     \
        return *type##_single;                                         \
    }                                                                  \
    inline static type& get() { return *type##_single; }

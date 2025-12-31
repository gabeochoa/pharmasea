#include "settings_schema.h"

namespace settings_schema {
namespace {

static void get_resolution(const settings::Data& d, int32_t& a, int32_t& b) {
    a = (int32_t) d.resolution.width;
    b = (int32_t) d.resolution.height;
}

static void set_resolution(settings::Data& d, int32_t a, int32_t b) {
    d.resolution.width = (int) a;
    d.resolution.height = (int) b;
}

}  // namespace

#include "settings_pscfg_keys.h"

}  // namespace

const std::vector<KeySpec>& all_keys() {
    // Note: keep this list stable & deterministic; it defines persistence order.
    static const std::vector<KeySpec> keys = []() {
        std::vector<KeySpec> out;
        out.reserve(32);

#define SPEC_BOOL(section, key, member, _def, life, _comment)              \
    out.push_back(KeySpec{.section = section,                              \
                          .key = key,                                      \
                          .type = ValueType::Bool,                         \
                          .lifecycle = life,                               \
                          .bool_member = &settings::Data::member});

#define SPEC_F32(section, key, member, _def, life, _comment)               \
    out.push_back(KeySpec{.section = section,                              \
                          .key = key,                                      \
                          .type = ValueType::F32,                          \
                          .lifecycle = life,                               \
                          .f32_member = &settings::Data::member});

#define SPEC_STR(section, key, member, _def, life, _comment)               \
    out.push_back(KeySpec{.section = section,                              \
                          .key = key,                                      \
                          .type = ValueType::Str,                          \
                          .lifecycle = life,                               \
                          .str_member = &settings::Data::member});

#define SPEC_I32X2(section, key, _member, _def, life, _comment)            \
    out.push_back(KeySpec{.section = section,                              \
                          .key = key,                                      \
                          .type = ValueType::I32x2,                        \
                          .lifecycle = life,                               \
                          .get_i32x2 = &get_resolution,                    \
                          .set_i32x2 = &set_resolution});

        SETTINGS_PSCFG_LIST(SPEC_BOOL, SPEC_F32, SPEC_STR, SPEC_I32X2)

#undef SPEC_BOOL
#undef SPEC_F32
#undef SPEC_STR
#undef SPEC_I32X2

        return out;
    }();

    return keys;
}

const KeySpec* find_key(std::string_view key) {
    // N is tiny; linear scan is fine and keeps things simple.
    for (const auto& k : all_keys()) {
        if (k.key == key) return &k;
    }
    return nullptr;
}

}  // namespace settings_schema


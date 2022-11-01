
#pragma once

#include <functional>
#include <memory>

#define defer(code) \
    std::shared_ptr<void> _(nullptr, std::bind([&]() { code; }));

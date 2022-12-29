
#pragma once

#include <functional>
#include <memory>

/*
    for example:
     {
         auto X = new int[10];
         defer(free x)
         ... rest of function
     } // free happens here
*/
#define defer(code) \
    std::shared_ptr<void> _(nullptr, std::bind([&]() { code; }));

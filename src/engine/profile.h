
#pragma once

#include "../external_include.h"
//
#include <chrono>

#include "singleton.h"

namespace profile {

constexpr int SAMPLE_SIZE = 1000;
struct Samples {
    std::string filename = "unknown";
    float samples[SAMPLE_SIZE];
    int index = 0;
    int num_items = 0;
    float sum = 0.0;

    void add_sample(float s) {
        index = (index + 1) % SAMPLE_SIZE;
        num_items++;
        sum += s;

        if (num_items < SAMPLE_SIZE) {
            samples[index] = s;
        } else {
            float& old = samples[index];
            sum -= old;
            old = s;
        }
    }

    [[nodiscard]] float average() const {
        return sum / std::min(num_items, SAMPLE_SIZE);
    }
};

typedef std::pair<std::string, Samples> Sample;

SINGLETON_FWD(Profiler);
struct Profiler {
    SINGLETON(Profiler);

    std::map<std::string, Samples> _acc;

    void add_sample(const std::string& name, const std::string& filename,
                    float amt) {
        if (!_acc.contains(name)) {
            _acc[name].filename = filename;
            _acc.insert(std::make_pair(name, Samples()));
        }
        _acc[name].add_sample(amt);
    }

    static void clear() { get()._acc.clear(); }
};

struct function_raii {
    std::string filename;
    std::string name;
    std::chrono::high_resolution_clock::time_point start;

    function_raii(const std::string& fileloc, const std::string& n)
        : filename(fileloc),
          name(n),
          start(std::chrono::high_resolution_clock::now()) {}

    ~function_raii() {
        auto end = std::chrono::high_resolution_clock::now();
        float duration =
            (float) std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                .count();
        Profiler::get().add_sample(name, filename, duration);
    }
};

[[nodiscard]] inline const std::string fmt_filename(const std::string& file,
                                                    int lineNum) {
    return fmt::format("{}:{} ", file, lineNum);
}

// https://stackoverflow.com/a/29856690
[[nodiscard]] inline const std::string computeMethodName(
    const std::string& function, const std::string& prettyFunction,
    const char* extra) {
    // If the input is a constructor, it gets the beginning of
    // the class name, not of the method. That's why later on we
    // have to search for the first parenthesys
    size_t locFunName = prettyFunction.find(function);
    size_t begin = prettyFunction.rfind(" ", locFunName) + 1;
    // Adding function.length() make
    // this faster and also allows to
    // handle operator parenthesis!
    size_t end = prettyFunction.find("(", locFunName + function.length());
    auto suffix = prettyFunction[end + 1] == ')' ? "()" : "(...)";
    return fmt::format("{}{}{}", prettyFunction.substr(begin, end - begin),
                       suffix, extra);
}

#define __PROFILE_FUNC__              \
    fmt_filename(__FILE__, __LINE__), \
        computeMethodName(__FUNCTION__, __PRETTY_FUNCTION__, "")

#define __PROFILE_LOC__(x)            \
    fmt_filename(__FILE__, __LINE__), \
        computeMethodName(__FUNCTION__, __PRETTY_FUNCTION__, x)

}  // namespace profile
   //

#define PROFILE()                            \
    do {                                     \
        using namespace profile;             \
        function_raii __f(__PROFILE_FUNC__); \
    } while (0)

#pragma once

#include <memory>
#include <vector>

#include "../engine/assert.h"
#include "../intro/intro_runner.h"

namespace tests {

class FakeIntroScene : public IntroScene {
   public:
    bool update(float, float) override { return done; }
    bool done = false;
};

inline void test_intro_phase() {
    using IntroPhase = IntroRunner::IntroPhase;

    IntroRunner empty_runner({});
    VALIDATE(empty_runner.phase() == IntroPhase::Done,
             "runner without scenes should be done");

    std::vector<std::unique_ptr<IntroScene>> scenes;
    auto fake = std::make_unique<FakeIntroScene>();
    FakeIntroScene* fake_ptr = fake.get();
    scenes.push_back(std::move(fake));
    IntroRunner runner(std::move(scenes));
    VALIDATE(runner.phase() == IntroPhase::Loading,
             "unknown scene type should fall back to loading");
    VALIDATE(runner.is_phase(IntroPhase::Loading),
             "is_phase should agree with phase()");

    fake_ptr->done = true;
    VALIDATE(runner.update(0.016f, 0.f),
             "runner should finish when its scene completes");
    VALIDATE(runner.phase() == IntroPhase::Done,
             "finished runner should be done");
}

}  // namespace tests

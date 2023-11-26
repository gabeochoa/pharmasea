
#pragma once

#include <memory>
//
#include "../engine/assert.h"
//

#include "../components/is_round_settings_manager.h"

namespace tests {

static std::shared_ptr<IsRoundSettingsManager> irsm;

inline void setup_config_key(float value = 0.f) {
    ConfigValueLibrary::get().load(
        {
            .key = ConfigKey::Test,
            .value = value,
        },
        "INVALID", "test");
    irsm = std::make_shared<IsRoundSettingsManager>();
}

inline void teardown_config_key() {
    irsm.reset();
    ConfigValueLibrary::get().unload_all();
}

inline void test_contains_config_key() {
    setup_config_key();
    //

    M_TEST_EQ(irsm->contains<float>(ConfigKey::Test), true,
              "should be in floats");
    M_TEST_EQ(irsm->contains<int>(ConfigKey::Test), false,
              "should not be in ints");
    M_TEST_EQ(irsm->contains<bool>(ConfigKey::Test), false,
              "should not be in bools");

    //
    teardown_config_key();
}

inline void test_initial_config_value() {
    setup_config_key();
    //
    float value = irsm->get_for_init<float>(ConfigKey::Test);
    M_TEST_EQ(value, 0.f, "should be 0");
    //
    teardown_config_key();
}

inline void test_get_with_default() {
    setup_config_key();
    //
    int value = irsm->get_with_default<int>(ConfigKey::Test, -1);
    M_TEST_EQ(value, -1, "should use default since its not in ints");

    float valuef = irsm->get_with_default<float>(ConfigKey::Test, -1);
    M_TEST_EQ(valuef, 0.f, "should not use default since its in floats");
    //
    teardown_config_key();
}

inline void test_apply_operations() {
    setup_config_key(1.f);
    //

    {
        float two_x = 2.f;
        float before = 1.f;
        float after = irsm->apply_operation_TEST_ONLY(Operation::Multiplier,
                                                      before, two_x);
        M_TEST_NEQ(before, after, "should have changed");
        M_TEST_EQ(before * two_x, after, "should be 2x");
    }

    {
        float two_x = 2.f;
        float before = 0.f;
        float after =
            irsm->apply_operation_TEST_ONLY(Operation::Set, before, two_x);
        M_TEST_NEQ(before, after, "should have changed");
        M_TEST_NEQ(before * two_x, after, "should not be 2x");
        M_TEST_EQ(two_x, after, "should be 2x");
    }

    //
    teardown_config_key();
}

inline void test_fetch_and_apply_operation(Operation type) {
    setup_config_key(1.f);
    //

    float two_x = 2.f;

    switch (type) {
        case Operation::Multiplier: {
            float before = irsm->get<float>(ConfigKey::Test);

            irsm->fetch_and_apply_TEST_ONLY<float>(ConfigKey::Test, type,
                                                   two_x);

            float after = irsm->get<float>(ConfigKey::Test);
            M_TEST_NEQ(before, after, "should have changed");
            M_TEST_EQ(before * two_x, after, "should be 2x");
        } break;
        case Operation::Set: {
            float before = irsm->get<float>(ConfigKey::Test);

            irsm->fetch_and_apply_TEST_ONLY<float>(ConfigKey::Test, type,
                                                   two_x);

            float after = irsm->get<float>(ConfigKey::Test);
            M_TEST_NEQ(before, after, "should have changed");
            M_TEST_EQ(two_x, after, "should be 2x");
        } break;
        case Operation::Custom: {
            // not testing this one
        } break;
    }

    //
    teardown_config_key();
}

inline void test_fetch_and_apply_operations() {
    magic_enum::enum_for_each<Operation>([&](auto val) {
        constexpr Operation type = val;
        test_fetch_and_apply_operation(type);
    });
}

/*
 * TODO need to add this back once we have temp
inline void test_is_temporary() {
    setup_config_key();

    {
        Upgrade upgrade{
            .name = "test",
        };

        M_TEST_EQ(irsm->is_temporary_upgrade(upgrade), false,
                  "should not be a temporary upgrade");
    }

    {
        Upgrade upgrade{
            .name = "test",
            .duration = 1,
        };

        M_TEST_EQ(irsm->is_temporary_upgrade(upgrade), true,
                  "should be a temporary upgrade");
    }

    teardown_config_key();
}
*/

inline void test_apply_simple_upgrade() {
    // apply nothing
    {
        float starting_value = 10.f;
        setup_config_key(starting_value);

        {
            Upgrade upgrade{
                .name = "test",
            };

            float before = irsm->get<float>(ConfigKey::Test);
            irsm->apply_effects(upgrade.name, upgrade.on_unlock);

            float after = irsm->get<float>(ConfigKey::Test);

            M_TEST_EQ(before, starting_value, "before should match starting");
            M_TEST_EQ(before, after,
                      "this upgrade has no effects so they should match");
        }

        teardown_config_key();
    }

    // apply 2x value
    {
        float starting_value = 10.f;
        setup_config_key(starting_value);

        {
            std::vector<UpgradeEffect> test_effects = {{
                .name = ConfigKey::Test,
                .operation = Operation::Multiplier,
                .value = 2.f,
            }};

            Upgrade upgrade{
                .name = "test",
                .on_unlock = test_effects,
            };

            float before = irsm->get<float>(ConfigKey::Test);

            irsm->apply_effects(upgrade.name, upgrade.on_unlock);

            float after = irsm->get<float>(ConfigKey::Test);

            M_TEST_EQ(before, starting_value, "before should match starting");
            M_TEST_NEQ(before, after,
                       "this upgrade has effects so they should not match");

            M_TEST_EQ(before * 2.f, after,
                      "before with multiplier should match after");
        }

        teardown_config_key();
    }
}

inline void test_unapply_simple_upgrade() {
    // apply nothing
    {
        float starting_value = 10.f;
        setup_config_key(starting_value);

        Upgrade upgrade{
            .name = "test",
        };
        {
            float before = irsm->get<float>(ConfigKey::Test);
            irsm->apply_effects(upgrade.name, upgrade.on_unlock);

            float after = irsm->get<float>(ConfigKey::Test);

            M_TEST_EQ(before, starting_value, "before should match starting");
            M_TEST_EQ(before, after,
                      "this upgrade has no effects so they should match");
        }

        {
            float before = irsm->get<float>(ConfigKey::Test);
            irsm->unapply_effects(upgrade.name, upgrade.on_unlock);
            float after = irsm->get<float>(ConfigKey::Test);

            M_TEST_EQ(before, after,
                      "this upgrade has no effects so they should match");
            M_TEST_EQ(after, starting_value, "after should match starting");
        }

        teardown_config_key();
    }

    // apply 2x value
    {
        float starting_value = 10.f;
        setup_config_key(starting_value);

        std::vector<UpgradeEffect> test_effects = {{
            .name = ConfigKey::Test,
            .operation = Operation::Multiplier,
            .value = 2.f,
        }};

        Upgrade upgrade{
            .name = "test",
            .on_unlock = test_effects,
        };

        {
            float before = irsm->get<float>(ConfigKey::Test);
            irsm->apply_effects(upgrade.name, upgrade.on_unlock);
            irsm->unapply_effects(upgrade.name, upgrade.on_unlock);
            float after_unapply = irsm->get<float>(ConfigKey::Test);

            M_TEST_EQ(before, starting_value, "before should match starting");
            M_TEST_EQ(before, after_unapply,
                      "after unapply we should be back at before");
        }

        teardown_config_key();
    }
}

inline void upgrade_tests() {
    test_contains_config_key();
    test_initial_config_value();
    test_get_with_default();
    test_apply_operations();
    test_fetch_and_apply_operations();
    //

    // test_is_temporary();
    test_apply_simple_upgrade();
    test_unapply_simple_upgrade();
}

}  // namespace tests

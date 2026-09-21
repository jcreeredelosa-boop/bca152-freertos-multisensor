#include <unity.h>
#include "logic.h"

// -------- Temperature alarm logic (5 tests) --------
void test_temp_below_lower(void)   { TEST_ASSERT_EQUAL((int)AlarmState::LOW_TEMPERATURE,  (int)evaluateTemperature(17.9f)); }
void test_temp_at_lower(void)      { TEST_ASSERT_EQUAL((int)AlarmState::NORMAL,          (int)evaluateTemperature(18.0f)); }
void test_temp_normal(void)        { TEST_ASSERT_EQUAL((int)AlarmState::NORMAL,          (int)evaluateTemperature(25.0f)); }
void test_temp_at_upper(void)      { TEST_ASSERT_EQUAL((int)AlarmState::NORMAL,          (int)evaluateTemperature(30.0f)); }
void test_temp_above_upper(void)   { TEST_ASSERT_EQUAL((int)AlarmState::HIGH_TEMPERATURE, (int)evaluateTemperature(30.1f)); }

// -------- Display navigation (4 tests) --------
void test_nav_cw_wrap(void) {
    TEST_ASSERT_EQUAL((int)DisplayMode::HUMIDITY,    (int)nextDisplayMode(DisplayMode::TEMPERATURE));
    TEST_ASSERT_EQUAL((int)DisplayMode::LIGHT,       (int)nextDisplayMode(DisplayMode::HUMIDITY));
    TEST_ASSERT_EQUAL((int)DisplayMode::MOTION,      (int)nextDisplayMode(DisplayMode::LIGHT));
    TEST_ASSERT_EQUAL((int)DisplayMode::TEMPERATURE, (int)nextDisplayMode(DisplayMode::MOTION));
}
void test_nav_ccw_wrap(void) {
    TEST_ASSERT_EQUAL((int)DisplayMode::MOTION,      (int)previousDisplayMode(DisplayMode::TEMPERATURE));
    TEST_ASSERT_EQUAL((int)DisplayMode::LIGHT,       (int)previousDisplayMode(DisplayMode::MOTION));
    TEST_ASSERT_EQUAL((int)DisplayMode::HUMIDITY,    (int)previousDisplayMode(DisplayMode::LIGHT));
    TEST_ASSERT_EQUAL((int)DisplayMode::TEMPERATURE, (int)previousDisplayMode(DisplayMode::HUMIDITY));
}
void test_nav_roundtrip(void) {
    DisplayMode m = DisplayMode::TEMPERATURE;
    for (int i = 0; i < 10; ++i) m = nextDisplayMode(m);
    for (int i = 0; i < 10; ++i) m = previousDisplayMode(m);
    TEST_ASSERT_EQUAL((int)DisplayMode::TEMPERATURE, (int)m);
}
void test_nav_single_step(void) {
    TEST_ASSERT_EQUAL((int)DisplayMode::HUMIDITY,
        (int)nextDisplayMode(DisplayMode::TEMPERATURE));
}

// -------- System state (4 tests) --------
void test_state_active_no_timeout(void) {
    TEST_ASSERT_EQUAL((int)SystemState::ACTIVE,
        (int)evaluateSystemState(SystemState::ACTIVE, true, false));
}
void test_state_active_timeout(void) {
    TEST_ASSERT_EQUAL((int)SystemState::INACTIVE,
        (int)evaluateSystemState(SystemState::ACTIVE, false, true));
}
void test_state_inactive_no_motion(void) {
    TEST_ASSERT_EQUAL((int)SystemState::INACTIVE,
        (int)evaluateSystemState(SystemState::INACTIVE, false, true));
}
void test_state_inactive_motion(void) {
    TEST_ASSERT_EQUAL((int)SystemState::ACTIVE,
        (int)evaluateSystemState(SystemState::INACTIVE, true, false));
}

// -------- REQUIRED by Unity --------
void setUp(void)    {}
void tearDown(void) {}

// -------- REQUIRED by native linker --------
int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_temp_below_lower);
    RUN_TEST(test_temp_at_lower);
    RUN_TEST(test_temp_normal);
    RUN_TEST(test_temp_at_upper);
    RUN_TEST(test_temp_above_upper);

    RUN_TEST(test_nav_cw_wrap);
    RUN_TEST(test_nav_ccw_wrap);
    RUN_TEST(test_nav_roundtrip);
    RUN_TEST(test_nav_single_step);

    RUN_TEST(test_state_active_no_timeout);
    RUN_TEST(test_state_active_timeout);
    RUN_TEST(test_state_inactive_no_motion);
    RUN_TEST(test_state_inactive_motion);

    return UNITY_END();
}
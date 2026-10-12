#include <unity.h>

#include "CsiMotionModel.h"

void setUp() {}
void tearDown() {}

static void fillSample(int8_t* sample, int8_t real, int8_t imag) {
  for (int i = 0; i < 128; i += 2) {
    sample[i] = real;
    sample[i + 1] = imag;
  }
}

void test_calibration_and_still_signal() {
  CsiMotionModel model;
  int8_t sample[128];
  fillSample(sample, 20, 10);
  model.reset(0, 1000);
  uint32_t accepted = 0;
  for (uint32_t now = 0; now <= 2200; now += 20) {
    model.addSample(sample, sizeof(sample), -55, now);
    ++accepted;
    model.tick(now, accepted, 0);
  }
  TEST_ASSERT_TRUE(model.calibrated());
  TEST_ASSERT_GREATER_OR_EQUAL(25, model.reliability());
  TEST_ASSERT_LESS_THAN(20, model.motionScore());
}

void test_changed_subcarriers_raise_motion_score() {
  CsiMotionModel model;
  int8_t sample[128];
  fillSample(sample, 20, 10);
  model.reset(0, 500);
  uint32_t accepted = 0;
  for (uint32_t now = 0; now <= 1200; now += 20) {
    model.addSample(sample, sizeof(sample), -55, now);
    ++accepted;
    model.tick(now, accepted, 0);
  }
  for (uint32_t now = 1220; now <= 2200; now += 20) {
    for (int i = 0; i < 128; i += 2) {
      sample[i] = (i % 8 == 0) ? 65 : 4;
      sample[i + 1] = (i % 6 == 0) ? -45 : 30;
    }
    model.addSample(sample, sizeof(sample), -55, now);
    ++accepted;
    model.tick(now, accepted, 0);
  }
  TEST_ASSERT_GREATER_THAN(40, model.motionScore());
  TEST_ASSERT_TRUE(model.state() == CsiMotionModel::State::POSSIBLE_MOTION ||
                   model.state() == CsiMotionModel::State::MOTION);
}

void test_sparse_samples_reduce_reliability() {
  CsiMotionModel model;
  int8_t sample[128];
  fillSample(sample, 20, 10);
  model.reset(0, 100);
  model.addSample(sample, sizeof(sample), -80, 200);
  model.tick(1200, 1, 8);
  TEST_ASSERT_LESS_THAN(25, model.reliability());
  TEST_ASSERT_EQUAL(static_cast<int>(CsiMotionModel::State::INSUFFICIENT_SAMPLES),
                    static_cast<int>(model.state()));
}

void test_relative_phase_variance_is_measured_from_one_receiver() {
  CsiMotionModel model;
  int8_t sample[128];
  fillSample(sample, 20, 10);
  model.reset(0, 500);
  uint32_t accepted = 0;
  for (uint32_t now = 0; now <= 1200; now += 20) {
    model.addSample(sample, sizeof(sample), -55, now);
    ++accepted;
    model.tick(now, accepted, 0);
  }
  for (uint32_t now = 1220; now <= 2200; now += 20) {
    for (int i = 0; i < 128; i += 2) {
      const bool rotated = ((i / 2) + (now / 20)) % 2;
      sample[i] = rotated ? -10 : 20;
      sample[i + 1] = rotated ? 20 : 10;
    }
    model.addSample(sample, sizeof(sample), -55, now);
    ++accepted;
    model.tick(now, accepted, 0);
  }
  TEST_ASSERT_GREATER_THAN(50, model.phaseScore());
  TEST_ASSERT_LESS_THAN(15, model.amplitudeScore());
  TEST_ASSERT_GREATER_THAN(10, model.motionScore());
}

void test_subcarrier_shape_change_reduces_baseline_correlation() {
  CsiMotionModel model;
  int8_t sample[128];
  fillSample(sample, 20, 10);
  model.reset(0, 500);
  uint32_t accepted = 0;
  for (uint32_t now = 0; now <= 1200; now += 20) {
    model.addSample(sample, sizeof(sample), -55, now);
    ++accepted;
    model.tick(now, accepted, 0);
  }
  for (uint32_t now = 1220; now <= 2200; now += 20) {
    for (int i = 0; i < 128; i += 2) {
      sample[i] = (i % 8 == 0) ? 70 : 3;
      sample[i + 1] = (i % 8 == 0) ? 1 : 3;
    }
    model.addSample(sample, sizeof(sample), -55, now);
    ++accepted;
    model.tick(now, accepted, 0);
  }
  TEST_ASSERT_GREATER_THAN(20, model.correlationScore());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_calibration_and_still_signal);
  RUN_TEST(test_changed_subcarriers_raise_motion_score);
  RUN_TEST(test_sparse_samples_reduce_reliability);
  RUN_TEST(test_relative_phase_variance_is_measured_from_one_receiver);
  RUN_TEST(test_subcarrier_shape_change_reduces_baseline_correlation);
  return UNITY_END();
}

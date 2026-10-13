#include <unity.h>

#include "CsiMotionModel.h"

void setUp() {}
void tearDown() {}

static void put12(int8_t* destination, int16_t value) {
  const uint16_t encoded = static_cast<uint16_t>(value) & 0x0fff;
  destination[0] = static_cast<int8_t>(encoded & 0xff);
  destination[1] = static_cast<int8_t>(encoded >> 8);
}

static void fillSample(int8_t* sample, int16_t real, int16_t imag) {
  for (int i = 0; i < 128; i += 4) {
    put12(sample + i, real);
    put12(sample + i + 2, imag);
  }
}

static void fillLegacySample(int8_t* sample, int8_t real, int8_t imag) {
  for (int i = 0; i < 128; i += 2) {
    sample[i] = imag;
    sample[i + 1] = real;
  }
}

void test_calibration_and_still_signal() {
  CsiMotionModel model;
  int8_t sample[128];
  fillSample(sample, 20, 10);
  model.reset(0, 1000);
  uint32_t accepted = 0;
  for (uint32_t now = 0; now <= 4200; now += 20) {
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
  for (uint32_t now = 0; now <= 2200; now += 20) {
    model.addSample(sample, sizeof(sample), -55, now);
    ++accepted;
    model.tick(now, accepted, 0);
  }
  for (uint32_t now = 2220; now <= 5200; now += 20) {
    const bool disturbed_packet = (now % 140) < 60;
    for (int i = 0; i < 128; i += 4) {
      put12(sample + i, disturbed_packet && ((i / 4) % 4 == 0) ? 650 :
                            disturbed_packet ? 40 : 20);
      put12(sample + i + 2, disturbed_packet && ((i / 4) % 3 == 0) ? -450 :
                                disturbed_packet ? 300 : 10);
    }
    model.addSample(sample, sizeof(sample), -55, now);
    ++accepted;
    model.tick(now, accepted, 0);
  }
  TEST_ASSERT_GREATER_THAN(20, model.amplitudeScore());
  TEST_ASSERT_GREATER_THAN(20, model.correlationScore());
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
  TEST_ASSERT_LESS_OR_EQUAL(25, model.reliability());
  TEST_ASSERT_TRUE(model.state() == CsiMotionModel::State::CALIBRATING ||
                   model.state() == CsiMotionModel::State::INSUFFICIENT_SAMPLES);
}

void test_relative_phase_variance_is_measured_from_one_receiver() {
  CsiMotionModel model;
  int8_t sample[128];
  fillSample(sample, 20, 10);
  model.reset(0, 500);
  uint32_t accepted = 0;
  for (uint32_t now = 0; now <= 2200; now += 20) {
    model.addSample(sample, sizeof(sample), -55, now);
    ++accepted;
    model.tick(now, accepted, 0);
  }
  for (uint32_t now = 2220; now <= 5200; now += 20) {
    const bool phase_event = (now % 180) < 60;
    for (int i = 0; i < 128; i += 4) {
      const bool rotated = phase_event && ((i / 4) % 2);
      put12(sample + i, rotated ? -10 : 20);
      put12(sample + i + 2, rotated ? 20 : 10);
    }
    model.addSample(sample, sizeof(sample), -55, now);
    ++accepted;
    model.tick(now, accepted, 0);
  }
  TEST_ASSERT_GREATER_THAN(50, model.phaseScore());
  TEST_ASSERT_LESS_THAN(15, model.amplitudeScore());
  // Fusion uses a median so a phase-only disturbance does not declare motion.
  TEST_ASSERT_LESS_THAN(15, model.motionScore());
}

void test_subcarrier_shape_change_reduces_baseline_correlation() {
  CsiMotionModel model;
  int8_t sample[128];
  fillSample(sample, 20, 10);
  model.reset(0, 500);
  uint32_t accepted = 0;
  for (uint32_t now = 0; now <= 2200; now += 20) {
    model.addSample(sample, sizeof(sample), -55, now);
    ++accepted;
    model.tick(now, accepted, 0);
  }
  for (uint32_t now = 2220; now <= 5200; now += 20) {
    for (int i = 0; i < 128; i += 4) {
      put12(sample + i, ((i / 4) + (now / 20)) % 4 == 0 ? 700 : 30);
      put12(sample + i + 2, 30);
    }
    model.addSample(sample, sizeof(sample), -55, now);
    ++accepted;
    model.tick(now, accepted, 0);
  }
  TEST_ASSERT_GREATER_THAN(20, model.correlationScore());
}

void test_signed_12_bit_words_and_invalid_first_tone() {
  CsiMotionModel model;
  int8_t sample[128];
  fillSample(sample, -61, 15);
  // Deliberately corrupt the first complex tone. The C5 flag must omit it.
  put12(sample, 2047);
  put12(sample + 2, -2048);
  model.reset(0, 1000);
  uint32_t accepted = 0;
  for (uint32_t now = 0; now <= 3200; now += 20) {
    model.addSample(sample, sizeof(sample), -55, now, true);
    ++accepted;
    model.tick(now, accepted, 0);
  }
  TEST_ASSERT_TRUE(model.calibrated());
  TEST_ASSERT_LESS_THAN(20, model.motionScore());
}

void test_signed_8_bit_imag_real_samples_and_invalid_first_word() {
  CsiMotionModel model;
  int8_t sample[128];
  fillLegacySample(sample, -61, 15);
  sample[0] = sample[1] = sample[2] = sample[3] = 127;
  model.reset(0, 1000);
  uint32_t accepted = 0;
  for (uint32_t now = 0; now <= 3200; now += 20) {
    model.addSample(sample, sizeof(sample), -55, now, true,
        CsiMotionModel::SampleEncoding::SIGNED_8_IMAG_REAL);
    ++accepted;
    model.tick(now, accepted, 0);
  }
  TEST_ASSERT_TRUE(model.calibrated());
  TEST_ASSERT_LESS_THAN(20, model.motionScore());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_calibration_and_still_signal);
  RUN_TEST(test_changed_subcarriers_raise_motion_score);
  RUN_TEST(test_sparse_samples_reduce_reliability);
  RUN_TEST(test_relative_phase_variance_is_measured_from_one_receiver);
  RUN_TEST(test_subcarrier_shape_change_reduces_baseline_correlation);
  RUN_TEST(test_signed_12_bit_words_and_invalid_first_tone);
  RUN_TEST(test_signed_8_bit_imag_real_samples_and_invalid_first_word);
  return UNITY_END();
}

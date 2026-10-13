#include "CsiMotionModel.h"

#include <math.h>
#include <string.h>

namespace {
float clampf(float value, float low, float high) {
  return value < low ? low : (value > high ? high : value);
}

float median4(float a, float b, float c, float d) {
  const float minimum = fminf(fminf(a, b), fminf(c, d));
  const float maximum = fmaxf(fmaxf(a, b), fmaxf(c, d));
  return (a + b + c + d - minimum - maximum) * 0.5f;
}
}

int16_t CsiMotionModel::decode12(const int8_t* bytes) {
  uint16_t word = static_cast<uint8_t>(bytes[0]) |
                  (static_cast<uint16_t>(static_cast<uint8_t>(bytes[1])) << 8);
  if (word & 0x0800)
    word |= 0xF000;
  return static_cast<int16_t>(word);
}

float CsiMotionModel::robustMedian(float* values, size_t count) {
  for (size_t i = 1; i < count; ++i) {
    const float value = values[i];
    size_t j = i;
    while (j && values[j - 1] > value) {
      values[j] = values[j - 1];
      --j;
    }
    values[j] = value;
  }
  if (!count) return 0.0f;
  return count & 1 ? values[count / 2] :
      (values[count / 2 - 1] + values[count / 2]) * 0.5f;
}

float CsiMotionModel::featureStd(float sum, float sum_sq, uint16_t count) {
  if (count < 2) return 0.0f;
  const float mean = sum / count;
  return sqrtf(clampf(sum_sq / count - mean * mean, 0.0f, 1000000.0f));
}

void CsiMotionModel::clearWindow(uint32_t now_ms) {
  memset(magnitude_sum_, 0, sizeof(magnitude_sum_));
  memset(magnitude_sq_sum_, 0, sizeof(magnitude_sq_sum_));
  memset(feature_sum_, 0, sizeof(feature_sum_));
  memset(feature_sq_sum_, 0, sizeof(feature_sq_sum_));
  window_samples_ = 0;
  pair_samples_ = 0;
  carrier_count_ = 0;
  window_started_ms_ = now_ms;
}

void CsiMotionModel::reset(uint32_t now_ms, uint32_t calibration_ms) {
  memset(previous_real_, 0, sizeof(previous_real_));
  memset(previous_imag_, 0, sizeof(previous_imag_));
  memset(previous_norm_, 0, sizeof(previous_norm_));
  memset(calibration_features_, 0, sizeof(calibration_features_));
  memset(feature_median_, 0, sizeof(feature_median_));
  memset(feature_scale_, 0, sizeof(feature_scale_));
  memset(history_, 0, sizeof(history_));
  previous_carrier_count_ = 0;
  history_head_ = 0;
  calibration_window_count_ = 0;
  started_ms_ = now_ms;
  calibration_ms_ = calibration_ms;
  last_tick_ms_ = now_ms;
  last_history_ms_ = now_ms;
  previous_accepted_ = 0;
  previous_dropped_ = 0;
  rssi_mean_ = -100.0f;
  rssi_deviation_ = 0.0f;
  smoothed_score_ = 0.0f;
  motion_threshold_z_ = 1.0f;
  samples_per_second_ = 0;
  last_rssi_ = -127;
  motion_score_ = amplitude_score_ = phase_score_ = correlation_score_ = 0;
  reliability_ = motion_votes_ = still_votes_ = 0;
  calibrated_ = false;
  state_ = State::CALIBRATING;
  clearWindow(now_ms);
}

uint8_t CsiMotionModel::calibrationPercent(uint32_t now_ms) const {
  if (calibrated_ || calibration_ms_ == 0) return 100;
  const uint32_t elapsed = now_ms - started_ms_;
  return elapsed >= calibration_ms_ ? 100 :
      static_cast<uint8_t>((elapsed * 100U) / calibration_ms_);
}

void CsiMotionModel::addSample(const int8_t* bytes, size_t length, int8_t rssi,
                               uint32_t now_ms, bool first_word_invalid,
                               SampleEncoding encoding) {
  if (!bytes || length < 16) return;
  const bool legacy = encoding == SampleEncoding::SIGNED_8_IMAG_REAL;
  size_t scalar_count = legacy ? length : length / 2;
  scalar_count &= ~static_cast<size_t>(1);
  // first_word_invalid covers four bytes: two legacy int8 complex tones or
  // one C5 complex tone made from two 16-bit scalar words.
  size_t first_scalar = first_word_invalid ? (legacy ? 4 : 2) : 0;
  if (scalar_count <= first_scalar + 3) return;
  size_t carriers = (scalar_count - first_scalar) / 2;
  if (carriers > kMaxCarriers) carriers = kMaxCarriers;

  float real[kMaxCarriers], imag[kMaxCarriers], magnitude[kMaxCarriers];
  float total = 0.0f;
  for (size_t i = 0; i < carriers; ++i) {
    const size_t scalar = first_scalar + i * 2;
    if (legacy) {
      // Original ESP32 CSI is emitted as signed int8 imaginary/real pairs.
      imag[i] = static_cast<float>(bytes[scalar]);
      real[i] = static_cast<float>(bytes[scalar + 1]);
    } else {
      // Forced L-LTF ESP32-C5 CSI stores signed 12-bit scalar components in
      // little-endian 16-bit words. Two words form one complex tone.
      real[i] = static_cast<float>(decode12(bytes + scalar * 2));
      imag[i] = static_cast<float>(decode12(bytes + (scalar + 1) * 2));
    }
    magnitude[i] = sqrtf(real[i] * real[i] + imag[i] * imag[i]);
    total += magnitude[i];
  }
  if (total < 1.0f) return;

  carrier_count_ = carriers;
  last_rssi_ = rssi;
  const float rssi_delta = static_cast<float>(rssi) - rssi_mean_;
  rssi_mean_ += rssi_delta * 0.03f;
  rssi_deviation_ += (fabsf(rssi_delta) - rssi_deviation_) * 0.03f;
  const float scale = static_cast<float>(carriers) / total;
  float norm[kMaxCarriers];
  for (size_t i = 0; i < carriers; ++i) {
    norm[i] = magnitude[i] * scale;
    magnitude_sum_[i] += norm[i];
    magnitude_sq_sum_[i] += norm[i] * norm[i];
  }
  ++window_samples_;

  if (previous_carrier_count_ == carriers) {
    float dot = 0.0f, energy = 0.0f, previous_energy = 0.0f;
    float temporal = 0.0f;
    float sx = 0.0f, sy = 0.0f, sxx = 0.0f, sxy = 0.0f;
    float phase_delta[kMaxCarriers];
    for (size_t i = 0; i < carriers; ++i) {
      dot += norm[i] * previous_norm_[i];
      energy += norm[i] * norm[i];
      previous_energy += previous_norm_[i] * previous_norm_[i];
      temporal += fabsf(norm[i] - previous_norm_[i]);
      float delta = atan2f(imag[i] * previous_real_[i] - real[i] * previous_imag_[i],
                           real[i] * previous_real_[i] + imag[i] * previous_imag_[i]);
      phase_delta[i] = delta;
      const float x = static_cast<float>(i);
      sx += x; sy += delta; sxx += x * x; sxy += x * delta;
    }
    const float denom = carriers * sxx - sx * sx;
    const float slope = fabsf(denom) > 1e-6f ? (carriers * sxy - sx * sy) / denom : 0.0f;
    const float intercept = (sy - slope * sx) / carriers;
    float phase_energy = 0.0f;
    for (size_t i = 0; i < carriers; ++i) {
      const float residual = phase_delta[i] - (slope * i + intercept);
      phase_energy += residual * residual;
    }
    const float decorrelation = energy > 0.0f && previous_energy > 0.0f ?
        1.0f - clampf(dot / sqrtf(energy * previous_energy), -1.0f, 1.0f) : 0.0f;
    const float features[3] = {
      decorrelation,
      sqrtf(phase_energy / carriers),
      temporal / carriers,
    };
    for (size_t i = 0; i < 3; ++i) {
      feature_sum_[i] += features[i];
      feature_sq_sum_[i] += features[i] * features[i];
    }
    ++pair_samples_;
  }
  memcpy(previous_real_, real, carriers * sizeof(float));
  memcpy(previous_imag_, imag, carriers * sizeof(float));
  memcpy(previous_norm_, norm, carriers * sizeof(float));
  previous_carrier_count_ = carriers;

  while (now_ms - window_started_ms_ >= 1000)
    finishWindow(window_started_ms_ + 1000);
}

void CsiMotionModel::finishCalibration() {
  if (!calibration_window_count_) {
    return;
  }
  float work[kMaxCalibrationWindows];
  for (size_t feature = 0; feature < kFeatureCount; ++feature) {
    for (size_t i = 0; i < calibration_window_count_; ++i)
      work[i] = calibration_features_[i][feature];
    feature_median_[feature] = robustMedian(work, calibration_window_count_);
    for (size_t i = 0; i < calibration_window_count_; ++i)
      work[i] = fabsf(calibration_features_[i][feature] - feature_median_[feature]);
    const float mad = robustMedian(work, calibration_window_count_) * 1.4826f;
    const float floor = feature_median_[feature] * 0.05f;
    // Real captures always contain thermal/quantization noise. A small
    // absolute floor also prevents perfectly constant synthetic calibration
    // data from amplifying float roundoff into a false motion event.
    feature_scale_[feature] = clampf(mad > floor ? mad : floor, 1e-4f, 1000000.0f);
  }
  for (size_t i = 0; i < calibration_window_count_; ++i) {
    float z[kFeatureCount];
    for (size_t f = 0; f < kFeatureCount; ++f)
      z[f] = clampf((calibration_features_[i][f] - feature_median_[f]) /
                    feature_scale_[f], 0.0f, 10.0f);
    work[i] = median4(z[0], z[1], z[2], z[3]);
  }
  robustMedian(work, calibration_window_count_);
  size_t percentile = (calibration_window_count_ * 85 + 99) / 100;
  if (percentile) --percentile;
  motion_threshold_z_ = clampf(work[percentile], 0.75f, 4.0f);
  calibrated_ = true;
  state_ = State::INSUFFICIENT_SAMPLES;
}

void CsiMotionModel::finishWindow(uint32_t now_ms) {
  if (window_samples_ >= 2 && pair_samples_ >= 2 && carrier_count_) {
    float amp_variance = 0.0f;
    for (size_t i = 0; i < carrier_count_; ++i) {
      const float mean = magnitude_sum_[i] / window_samples_;
      amp_variance += sqrtf(clampf(magnitude_sq_sum_[i] / window_samples_ -
                                  mean * mean, 0.0f, 1000000.0f));
    }
    const float features[kFeatureCount] = {
      amp_variance / carrier_count_,
      featureStd(feature_sum_[0], feature_sq_sum_[0], pair_samples_),
      featureStd(feature_sum_[1], feature_sq_sum_[1], pair_samples_),
      featureStd(feature_sum_[2], feature_sq_sum_[2], pair_samples_),
    };
    if (!calibrated_ && calibration_window_count_ < kMaxCalibrationWindows) {
      memcpy(calibration_features_[calibration_window_count_], features,
             sizeof(features));
      ++calibration_window_count_;
    }
    if (!calibrated_ && now_ms - started_ms_ >= calibration_ms_)
      finishCalibration();
    if (calibrated_) {
      float z[kFeatureCount];
      for (size_t i = 0; i < kFeatureCount; ++i)
        z[i] = clampf((features[i] - feature_median_[i]) /
                      feature_scale_[i], 0.0f, 10.0f);
      const float fused = median4(z[0], z[1], z[2], z[3]);
      const float mapped = clampf((fused / motion_threshold_z_) * kMotionThreshold,
                                  0.0f, 100.0f);
      amplitude_score_ = static_cast<uint8_t>(clampf(z[0] * 20.0f, 0.0f, 100.0f) + 0.5f);
      correlation_score_ = static_cast<uint8_t>(clampf(z[1] * 20.0f, 0.0f, 100.0f) + 0.5f);
      phase_score_ = static_cast<uint8_t>(clampf(z[2] * 20.0f, 0.0f, 100.0f) + 0.5f);
      smoothed_score_ += (mapped - smoothed_score_) * 0.55f;
      motion_score_ = static_cast<uint8_t>(smoothed_score_ + 0.5f);
    }
  }
  clearWindow(now_ms);
}

void CsiMotionModel::tick(uint32_t now_ms, uint32_t accepted, uint32_t dropped) {
  while (now_ms - window_started_ms_ >= 1000)
    finishWindow(window_started_ms_ + 1000);
  if (!calibrated_ && calibration_window_count_ &&
      now_ms - started_ms_ >= calibration_ms_)
    finishCalibration();

  const uint32_t elapsed = now_ms - last_tick_ms_;
  if (elapsed >= 1000) {
    const uint32_t new_samples = accepted - previous_accepted_;
    const uint32_t new_drops = dropped - previous_dropped_;
    samples_per_second_ = static_cast<uint16_t>(new_samples > 65535 ? 65535 :
        (new_samples * 1000U) / elapsed);
    previous_accepted_ = accepted;
    previous_dropped_ = dropped;
    last_tick_ms_ = now_ms;
    int quality = samples_per_second_ >= 20 ? 45 : samples_per_second_ >= 10 ? 35 :
                  samples_per_second_ >= 5 ? 24 : samples_per_second_ >= 2 ? 12 : 0;
    quality += rssi_mean_ > -75 ? 20 : rssi_mean_ > -85 ? 12 : 4;
    quality += rssi_deviation_ < 3.0f ? 20 : rssi_deviation_ < 7.0f ? 12 : 3;
    const uint32_t received = new_samples + new_drops;
    if (received) {
      quality += static_cast<int>((new_samples * 15U) / received);
      quality -= static_cast<int>((new_drops * 60U) / received);
    }
    if (samples_per_second_ < 2 && quality > 20) quality = 20;
    reliability_ = calibrated_ ? static_cast<uint8_t>(clampf(quality, 0, 100)) :
                                 calibrationPercent(now_ms) / 4;
    if (!calibrated_) state_ = State::CALIBRATING;
    else if (samples_per_second_ < 2 || reliability_ < 25)
      state_ = State::INSUFFICIENT_SAMPLES;
    else if (motion_score_ >= kMotionThreshold) {
      motion_votes_ = motion_votes_ < 5 ? motion_votes_ + 1 : 5;
      still_votes_ = 0;
      state_ = motion_votes_ >= 2 ? State::MOTION : State::POSSIBLE_MOTION;
    } else {
      motion_votes_ = 0;
      still_votes_ = still_votes_ < 3 ? still_votes_ + 1 : 3;
      if (still_votes_ >= 2) state_ = State::STILL;
      else if (motion_score_ >= kPossibleMotionThreshold)
        state_ = State::POSSIBLE_MOTION;
    }
  }
  if (now_ms - last_history_ms_ >= 1000) {
    last_history_ms_ = now_ms;
    history_[history_head_] = reliability_ < 15 ? 255 : motion_score_;
    history_head_ = (history_head_ + 1) % kHistorySize;
  }
}

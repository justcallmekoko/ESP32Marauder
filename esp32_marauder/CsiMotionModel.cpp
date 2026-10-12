#include "CsiMotionModel.h"

#include <math.h>

namespace {
float clampf(float value, float low, float high) {
  return value < low ? low : (value > high ? high : value);
}

float wrapPhase(float value) {
  while (value > static_cast<float>(M_PI)) value -= 2.0f * static_cast<float>(M_PI);
  while (value < -static_cast<float>(M_PI)) value += 2.0f * static_cast<float>(M_PI);
  return value;
}
}

void CsiMotionModel::reset(uint32_t now_ms, uint32_t calibration_ms) {
  for (size_t i = 0; i < kMaxCarriers; ++i) {
    baseline_[i] = 0.0f;
    noise_[i] = 0.02f;
    previous_phase_[i] = 0.0f;
  }
  for (size_t i = 0; i < kHistorySize; ++i)
    history_[i] = 0;
  carrier_count_ = 0;
  history_head_ = 0;
  started_ms_ = now_ms;
  calibration_ms_ = calibration_ms;
  last_tick_ms_ = now_ms;
  last_history_ms_ = now_ms;
  previous_accepted_ = 0;
  previous_dropped_ = 0;
  rssi_mean_ = -100.0f;
  rssi_deviation_ = 0.0f;
  smoothed_score_ = 0.0f;
  phase_baseline_ = 0.0f;
  phase_noise_ = 0.01f;
  correlation_baseline_ = 0.0f;
  correlation_noise_ = 0.01f;
  samples_per_second_ = 0;
  last_rssi_ = -127;
  motion_score_ = 0;
  amplitude_score_ = 0;
  phase_score_ = 0;
  correlation_score_ = 0;
  reliability_ = 0;
  motion_votes_ = 0;
  still_votes_ = 0;
  calibrated_ = false;
  phase_initialized_ = false;
  state_ = State::CALIBRATING;
}

uint8_t CsiMotionModel::calibrationPercent(uint32_t now_ms) const {
  if (calibrated_ || calibration_ms_ == 0)
    return 100;
  const uint32_t elapsed = now_ms - started_ms_;
  return elapsed >= calibration_ms_ ? 100 :
      static_cast<uint8_t>((elapsed * 100U) / calibration_ms_);
}

void CsiMotionModel::addSample(const int8_t* iq, size_t length, int8_t rssi,
                               uint32_t now_ms) {
  if (!iq || length < 16)
    return;
  size_t carriers = length / 2;
  if (carriers > kMaxCarriers)
    carriers = kMaxCarriers;
  carrier_count_ = carriers;
  last_rssi_ = rssi;
  const float rssi_delta = static_cast<float>(rssi) - rssi_mean_;
  rssi_mean_ += rssi_delta * 0.03f;
  rssi_deviation_ += (fabsf(rssi_delta) - rssi_deviation_) * 0.03f;

  float magnitudes[kMaxCarriers];
  float phases[kMaxCarriers];
  float total = 0.0f;
  for (size_t i = 0; i < carriers; ++i) {
    const float real = static_cast<float>(iq[i * 2]);
    const float imag = static_cast<float>(iq[i * 2 + 1]);
    magnitudes[i] = sqrtf(real * real + imag * imag);
    phases[i] = atan2f(imag, real);
    total += magnitudes[i];
  }
  if (total < 1.0f)
    return;
  const float scale = static_cast<float>(carriers) / total;
  float disturbance = 0.0f;
  float dot = 0.0f;
  float current_energy = 0.0f;
  float baseline_energy = 0.0f;
  float phase_delta_energy = 0.0f;
  const float phase_reference = phases[0];
  for (size_t i = 0; i < carriers; ++i) {
    const float normalized = magnitudes[i] * scale;
    const float relative_phase = wrapPhase(phases[i] - phase_reference);
    if (phase_initialized_) {
      const float phase_delta = wrapPhase(relative_phase - previous_phase_[i]);
      phase_delta_energy += phase_delta * phase_delta;
    }
    previous_phase_[i] = relative_phase;
    if (!calibrated_) {
      const float alpha = baseline_[i] == 0.0f ? 1.0f : 0.025f;
      const float delta = normalized - baseline_[i];
      baseline_[i] += delta * alpha;
      noise_[i] += (fabsf(delta) - noise_[i]) * 0.025f;
    } else {
      const float deviation = fabsf(normalized - baseline_[i]);
      disturbance += deviation / (noise_[i] + 0.025f);
    }
    dot += normalized * baseline_[i];
    current_energy += normalized * normalized;
    baseline_energy += baseline_[i] * baseline_[i];
  }
  const float phase_variance = phase_initialized_ ?
      phase_delta_energy / static_cast<float>(carriers) : 0.0f;
  phase_initialized_ = true;

  float correlation_change = 0.0f;
  if (current_energy > 0.0f && baseline_energy > 0.0f) {
    const float similarity = dot / sqrtf(current_energy * baseline_energy);
    correlation_change = 1.0f - clampf(similarity, -1.0f, 1.0f);
  }

  if (!calibrated_) {
    const float phase_delta = phase_variance - phase_baseline_;
    phase_baseline_ += phase_delta * 0.025f;
    phase_noise_ += (fabsf(phase_delta) - phase_noise_) * 0.025f;
    const float corr_delta = correlation_change - correlation_baseline_;
    correlation_baseline_ += corr_delta * 0.025f;
    correlation_noise_ += (fabsf(corr_delta) - correlation_noise_) * 0.025f;
  }

  if (!calibrated_ && now_ms - started_ms_ >= calibration_ms_) {
    calibrated_ = true;
    state_ = State::INSUFFICIENT_SAMPLES;
  }
  if (calibrated_) {
    const float raw = disturbance / static_cast<float>(carriers);
    // Hardware testing on ESP32-C5 showed useful motion separation in a much
    // narrower raw range than the original conservative mapping exposed.
    const float amplitude_mapped = clampf((raw - 1.18f) * 220.0f, 0.0f, 100.0f);
    const float phase_excess =
        (phase_variance - phase_baseline_) / (phase_noise_ + 0.005f);
    const float phase_mapped = clampf((phase_excess - 1.0f) * 18.0f, 0.0f, 100.0f);
    const float corr_excess =
        (correlation_change - correlation_baseline_) / (correlation_noise_ + 0.001f);
    const float correlation_mapped =
        clampf((corr_excess - 1.0f) * 18.0f, 0.0f, 100.0f);
    amplitude_score_ = static_cast<uint8_t>(amplitude_mapped + 0.5f);
    phase_score_ = static_cast<uint8_t>(phase_mapped + 0.5f);
    correlation_score_ = static_cast<uint8_t>(correlation_mapped + 0.5f);
    // Amplitude remains the anchor until hardware-labelled captures justify
    // stronger phase/correlation weights on ESP32-C5 CSI.
    const float fused = amplitude_mapped * 0.60f + phase_mapped * 0.25f +
                        correlation_mapped * 0.15f;
    smoothed_score_ += (fused - smoothed_score_) * 0.18f;
    motion_score_ = static_cast<uint8_t>(smoothed_score_ + 0.5f);
    if (motion_score_ < 12) {
      for (size_t i = 0; i < carriers; ++i) {
        const float normalized = magnitudes[i] * scale;
        baseline_[i] += (normalized - baseline_[i]) * 0.001f;
      }
    }
  }
}

void CsiMotionModel::tick(uint32_t now_ms, uint32_t accepted, uint32_t dropped) {
  const uint32_t elapsed = now_ms - last_tick_ms_;
  if (elapsed >= 1000) {
    const uint32_t new_samples = accepted - previous_accepted_;
    const uint32_t new_drops = dropped - previous_dropped_;
    samples_per_second_ = static_cast<uint16_t>(
        new_samples > 65535 ? 65535 : (new_samples * 1000U) / elapsed);
    previous_accepted_ = accepted;
    previous_dropped_ = dropped;
    last_tick_ms_ = now_ms;

    int quality = samples_per_second_ >= 20 ? 45 :
                  samples_per_second_ >= 10 ? 35 :
                  samples_per_second_ >= 5 ? 24 :
                  samples_per_second_ >= 2 ? 12 : 0;
    quality += rssi_mean_ > -75 ? 20 : rssi_mean_ > -85 ? 12 : 4;
    quality += rssi_deviation_ < 3.0f ? 20 : rssi_deviation_ < 7.0f ? 12 : 3;
    const uint32_t received = new_samples + new_drops;
    if (received != 0) {
      quality += static_cast<int>((new_samples * 15U) / received);
      quality -= static_cast<int>((new_drops * 60U) / received);
    }
    if (samples_per_second_ < 2 && quality > 20)
      quality = 20;
    if (quality < 0)
      quality = 0;
    reliability_ = calibrated_ ? static_cast<uint8_t>(quality > 100 ? 100 : quality)
                               : calibrationPercent(now_ms) / 4;

    if (!calibrated_) {
      state_ = State::CALIBRATING;
    } else if (samples_per_second_ < 2 || reliability_ < 25) {
      state_ = State::INSUFFICIENT_SAMPLES;
    } else if (motion_score_ >= kMotionThreshold) {
      motion_votes_ = motion_votes_ < 5 ? motion_votes_ + 1 : 5;
      still_votes_ = 0;
      state_ = motion_votes_ >= 2 ? State::MOTION : State::POSSIBLE_MOTION;
    } else if (motion_score_ >= kPossibleMotionThreshold) {
      motion_votes_ = 0;
      still_votes_ = 0;
      state_ = State::POSSIBLE_MOTION;
    } else {
      motion_votes_ = 0;
      still_votes_ = still_votes_ < 3 ? still_votes_ + 1 : 3;
      if (still_votes_ >= 2)
        state_ = State::STILL;
    }
  }
  if (now_ms - last_history_ms_ >= 250) {
    last_history_ms_ = now_ms;
    history_[history_head_] = reliability_ < 15 ? 255 : motion_score_;
    history_head_ = (history_head_ + 1) % kHistorySize;
  }
}

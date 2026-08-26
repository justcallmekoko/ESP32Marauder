#include <unity.h>

#include "ReconProbeQueue.h"

void test_probe_queue_is_bounded_and_fifo() {
  ReconProbeQueue queue;
  ReconProbeEvent event = {};
  for (uint8_t index = 0; index < RECON_PROBE_QUEUE_SIZE - 1; index++) {
    event.channel = index;
    TEST_ASSERT_TRUE(queue.push(event));
  }
  TEST_ASSERT_FALSE(queue.push(event));
  TEST_ASSERT_EQUAL_UINT16(1, queue.dropped());
  for (uint8_t index = 0; index < RECON_PROBE_QUEUE_SIZE - 1; index++) {
    TEST_ASSERT_TRUE(queue.pop(event));
    TEST_ASSERT_EQUAL_UINT8(index, event.channel);
  }
  TEST_ASSERT_FALSE(queue.pop(event));
}

void test_repeat_gate_emits_changes_and_returns() {
  ReconRepeatGate gate;
  const uint8_t mac[6] = {0, 1, 2, 3, 4, 5};
  TEST_ASSERT_FALSE(gate.accept(mac, -50, 1000));
  TEST_ASSERT_FALSE(gate.accept(mac, -55, 2000));
  TEST_ASSERT_TRUE(gate.accept(mac, -70, 3000));
  TEST_ASSERT_TRUE(gate.accept(mac, -70, 33000));
}

void test_repeat_gate_handles_timer_rollover() {
  ReconRepeatGate gate;
  const uint8_t mac[6] = {6, 5, 4, 3, 2, 1};
  TEST_ASSERT_FALSE(gate.accept(mac, -60, UINT32_MAX - 10000));
  TEST_ASSERT_TRUE(gate.accept(mac, -60, 25000));
}

void test_deauth_queue_preserves_alert_details() {
  ReconDeauthQueue queue;
  ReconDeauthEvent expected = {};
  expected.transmitter[5] = 0xAA;
  expected.bssid[5] = 0xBB;
  expected.rssi = -63;
  expected.channel = 11;
  expected.reason = 7;
  TEST_ASSERT_TRUE(queue.push(expected));
  ReconDeauthEvent actual = {};
  TEST_ASSERT_TRUE(queue.pop(actual));
  TEST_ASSERT_EQUAL_UINT8(0xAA, actual.transmitter[5]);
  TEST_ASSERT_EQUAL_UINT8(0xBB, actual.bssid[5]);
  TEST_ASSERT_EQUAL_INT8(-63, actual.rssi);
  TEST_ASSERT_EQUAL_UINT8(11, actual.channel);
  TEST_ASSERT_EQUAL_UINT16(7, actual.reason);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_probe_queue_is_bounded_and_fifo);
  RUN_TEST(test_repeat_gate_emits_changes_and_returns);
  RUN_TEST(test_repeat_gate_handles_timer_rollover);
  RUN_TEST(test_deauth_queue_preserves_alert_details);
  return UNITY_END();
}

#include <unity.h>

#include "CliBusy.h"

void setUp() {}
void tearDown() {}

void test_scan_and_attack_commands_are_gated() {
  TEST_ASSERT_TRUE(marauder::cliIsScanGatedCommand("wardrive"));
  TEST_ASSERT_TRUE(marauder::cliIsScanGatedCommand("scanall"));
  TEST_ASSERT_TRUE(marauder::cliIsScanGatedCommand("sniffraw"));
  TEST_ASSERT_TRUE(marauder::cliIsScanGatedCommand("attack"));
  TEST_ASSERT_TRUE(marauder::cliIsScanGatedCommand("wardrivepoi"));
}

void test_commands_outside_the_gate_are_not_reported() {
  TEST_ASSERT_FALSE(marauder::cliIsScanGatedCommand("stopscan"));
  TEST_ASSERT_FALSE(marauder::cliIsScanGatedCommand("join"));
  TEST_ASSERT_FALSE(marauder::cliIsScanGatedCommand("info"));
  TEST_ASSERT_FALSE(marauder::cliIsScanGatedCommand("ping"));
  TEST_ASSERT_FALSE(marauder::cliIsScanGatedCommand("help"));
  TEST_ASSERT_FALSE(marauder::cliIsScanGatedCommand("recon"));
}

void test_matching_is_exact() {
  TEST_ASSERT_FALSE(marauder::cliIsScanGatedCommand(""));
  TEST_ASSERT_FALSE(marauder::cliIsScanGatedCommand(nullptr));
  TEST_ASSERT_FALSE(marauder::cliIsScanGatedCommand("Wardrive"));
  TEST_ASSERT_FALSE(marauder::cliIsScanGatedCommand("wardrive "));
  TEST_ASSERT_FALSE(marauder::cliIsScanGatedCommand("ward"));
}

void test_each_reason_has_its_own_message() {
  TEST_ASSERT_EQUAL_STRING(
      "WiFi is connected. Run stopscan -f first.",
      marauder::cliBusyMessage(marauder::CliBusyReason::WifiConnected));
  TEST_ASSERT_EQUAL_STRING(
      "Run stopscan before starting a scan.",
      marauder::cliBusyMessage(marauder::CliBusyReason::ShowInfo));
  TEST_ASSERT_EQUAL_STRING(
      "A scan is already running. Run stopscan first.",
      marauder::cliBusyMessage(marauder::CliBusyReason::ScanRunning));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_scan_and_attack_commands_are_gated);
  RUN_TEST(test_commands_outside_the_gate_are_not_reported);
  RUN_TEST(test_matching_is_exact);
  RUN_TEST(test_each_reason_has_its_own_message);
  return UNITY_END();
}

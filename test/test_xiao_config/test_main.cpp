#include <unity.h>

#include "configs.h"

#if !defined(HAS_SCREEN) || !defined(HAS_BUTTONS) || !defined(HAS_PSRAM)
  #error "The XIAO display target lost a required hardware capability"
#endif
#if defined(HAS_TOUCH) || defined(HAS_SD) || defined(USE_SD) || defined(HAS_DIRECT_UPLOAD)
  #error "Unvalidated XIAO touch/SD capabilities must stay disabled"
#endif

void setUp() {}
void tearDown() {}

void test_every_visible_menu_row_fits_on_the_panel() {
  const int first_top = int(KEY_Y) - KEY_H / 2;
  const int last_bottom = int(KEY_Y + (BUTTON_SCREEN_LIMIT - 1) *
                                        (KEY_H + KEY_SPACING_Y)) + KEY_H / 2;
  TEST_ASSERT_TRUE(first_top >= TOP_FIXED_AREA);
  TEST_ASSERT_TRUE(last_bottom < SCREEN_HEIGHT);
  TEST_ASSERT_EQUAL_INT(BUTTON_SCREEN_LIMIT, BUTTON_ARRAY_LEN);
  TEST_ASSERT_EQUAL_INT(172, SCREEN_WIDTH);
  TEST_ASSERT_EQUAL_INT(320, SCREEN_HEIGHT);
}

void test_buttons_and_disabled_card_slot_keep_their_board_pins() {
  TEST_ASSERT_EQUAL_INT(11, D_BTN);
  TEST_ASSERT_EQUAL_INT(42, C_BTN);
  TEST_ASSERT_EQUAL_INT(43, XIAO_SD_CS);
  TEST_ASSERT_TRUE(D_PULL);
  TEST_ASSERT_TRUE(C_PULL);
  TEST_ASSERT_EQUAL_INT(-1, U_BTN);
  TEST_ASSERT_EQUAL_INT(-1, L_BTN);
  TEST_ASSERT_EQUAL_INT(-1, R_BTN);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_every_visible_menu_row_fits_on_the_panel);
  RUN_TEST(test_buttons_and_disabled_card_slot_keep_their_board_pins);
  return UNITY_END();
}

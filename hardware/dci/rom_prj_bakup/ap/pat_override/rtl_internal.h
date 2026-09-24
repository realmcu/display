/*
 * rtl87x3k PERIPHERAL_AUTO_TEST build override (soc side).
 *
 * Pulls in the real platform header, then forces the GPIO/SPI auto-test
 * feature switches off so the guarded references in the (driver-side)
 * harness (peripheral_auto_test.c / app_console_msg.c / app_io_msg.c)
 * compile out. test_gpio.c / test_spi.c stay excluded from this target's
 * build, whose k-porting is not yet done.
 *
 * This file has no include guard on purpose: it must let #include_next
 * reach the real rtl_internal.h (which owns _RTL_INTERNAL_H_).
 * Only wired into the PERIPHERAL_AUTO_TEST target's IncludePath.
 */
#include_next "rtl_internal.h"

#undef AUTO_TEST_SUPPORT_GPIO_TEST
#define AUTO_TEST_SUPPORT_GPIO_TEST (0)

#undef AUTO_TEST_SUPPORT_SPI_TEST
#define AUTO_TEST_SUPPORT_SPI_TEST (0)

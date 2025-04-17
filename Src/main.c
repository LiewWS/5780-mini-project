#include "main.h"
#include <stm32f0xx_hal.h>

int main(void)
{
  #if defined(CP1)
  bt_conf_main();
  #elif defined(MENC)
  magnetic_encoder_main();
  #elif defined(TEST)
  test_main();
  #elif defined(BTENC)
  bt_magnetic_enc_main();
  #elif defined(LAB5)
  lab5_main();
  #elif defined(LAB6)
  lab6_main();
  #elif defined(LAB7)
  lab7_main();
  #else
  #error No valid target specified
  #endif

}

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
  #elif defined(MTR)
  motor_main();
  #elif defined(BTENCSER)
  bt_magnetic_enc_main();
  #elif defined(PROJMAIN)
  project_main();
  #else
  #error No valid target specified
  #endif

}

#include "bluetooth_magenc.h"

int bt_magnetic_enc_main(void)
{
    HAL_Init();
    SystemClock_Config();

#if defined(SENDER)
    send_main();
#else
    recv_main();
#endif

    return 1;
}


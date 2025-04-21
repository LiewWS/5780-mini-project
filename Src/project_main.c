#include "main.h"
#include "magnetic_encoder.h"
#include "hal_usart.h"
#include "bluetooth_buf.h"

// Incremented by systick interrupt
extern uint32_t tick_count;
#define MAX_TICK_COUNT 0xFFFFFFFF

void calibration_loop();
void balance_loop();

int project_main()
{
    HAL_Init();
    SystemClock_Config();

#if defined(SENDER)
    send_main();
#else
    // Enable clock to peripherals
    HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_RCC_USART1_CLK_ENABLE();

#if defined(DEBUG)
    // init LEDs in case needed for debugging
    uint16_t ledPins = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitTypeDef initLED = {ledPins, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL};
    HAL_GPIO_Init(GPIOC, &initLED);
#endif
    
    // Configure USART for bluetooth
    configure_TTL_RXint(USART1, HAL_RCC_GetHCLKFreq() / 9600);
    // USART1 TX Pin (connect to RX of bluetooth)
    GPIO_InitTypeDef init_pa9 = {GPIO_PIN_9, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 1};
    HAL_GPIO_Init(GPIOA, &init_pa9);
    // USART1 RX Pin (connect to TX of bluetooth)
    GPIO_InitTypeDef init_pa10 = {GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 1};
    HAL_GPIO_Init(GPIOA, &init_pa10);
    // Set up NVIC
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(USART1_IRQn, 1);

    // Configure PWM

    calibration_loop();
    balance_loop();
#endif

    return 1;
}

void balance_loop()
{
    // Initialize receive buffer
    buf_head = 0;
    buf_tail = 0;

    uint16_t angle = 0;
    uint16_t old_angle = 0;
    uint32_t time = 1;
    uint32_t old_time = 0;
    uint32_t time_diff = 1;
    int32_t anglev = 0;
    while (1) {
        angle = read_angle();
        time = tick_count;
        if (old_time > time) {
            // tick_count overflowed
            time_diff = (MAX_TICK_COUNT - old_time) + time;
        } else if (time == old_time) {
            // Would not want to div by 0
            time_diff = 1;
        } else {
            time_diff = time - old_time;
        }
        anglev = (angle - old_angle) / time_diff;

        // PID and update PWM

        old_angle = angle;
        old_time = time;
    }
}


#include "main.h"
#include "magnetic_encoder.h"
#include "hal_usart.h"
#include "motor.h"
#include <stdlib.h>
//#include <stdint.h>

#define KP 1
#define KI 1

void send_motor_ctrl(void);
void calibration_loop();
void balance_loop();
void init_magenc(void);
uint16_t read_i2c_raw_angle(void);

#define MAX_TICK_COUNT 65432
#define V_THRESHHOLD (5)
uint16_t initial_angle;
uint8_t allow_reverse;
// uint8_t direction;

// Here we assume that angle = 2048 when hanging down
#define HANG_ANGLE 2048
#define SWING_THRES_LEFT 1024
#define SWING_THRES_RIGHT 3072
#define SWING_VAL_LEFT 1536
#define SWING_VAL_RIGHT 2560

int project_main()
{
    // HAL_Init();
    // SystemClock_Config();

#if defined(SENDER)
    // I2C:
    // Bluetooth RXD: PA9, TXD: PA10
    send_motor_ctrl();
#else
    // Enable clock to peripherals
    HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_RCC_USART1_CLK_ENABLE();

    // init LEDs in case needed for debugging
    uint16_t ledPins = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitTypeDef initLED = {ledPins, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL};
    HAL_GPIO_Init(GPIOC, &initLED);

    // Configure USART for bluetooth
    configure_TTL_RXint(USART1, HAL_RCC_GetHCLKFreq() / 115200);
    // USART1 TX Pin (connect to RX of bluetooth)
    GPIO_InitTypeDef init_pa9 = {GPIO_PIN_9, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 1};
    HAL_GPIO_Init(GPIOA, &init_pa9);
    // USART1 RX Pin (connect to TX of bluetooth)
    GPIO_InitTypeDef init_pa10 = {GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 1};
    HAL_GPIO_Init(GPIOA, &init_pa10);
    // Set up NVIC
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(USART1_IRQn, 1);

    // Configure motor
    // motor enable: PA4
    pwm_init();
    // For motor direction, IN1: PC8, IN2:  PC9
    uint16_t motor_dir_pins = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitTypeDef init_motor_dir = {motor_dir_pins, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL};
    HAL_GPIO_Init(GPIOC, &init_motor_dir);

    //  Stop motor initially
    pwm_setDutyCycle(0, 2);

    uint32_t heartbeat = 0;

    while (1)
    {
        if (heartbeat == 10000)
        {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
            heartbeat = 0;
        }
        ++heartbeat;
    }

#endif

    return 1;
}
void init_magenc(void)
{
    uint16_t init_angle = 0;
    uint8_t writtenData[1];
    
    //read the raw angle
    init_angle = read_i2c_raw_angle();

    uint8_t write_zpos_data[2] = {0x01, init_angle >> 8};
    //write the raw angle to the zpos register, set it to 0
    write_i2c(write_zpos_data, MAG_ADDR, 2);
    write_zpos_data[0] = 0x02;
    write_zpos_data[1] = init_angle & 0x00ff;
    write_i2c(write_zpos_data, MAG_ADDR, 2);

    //read the init angle (this will always be zero)
    writtenData[0] = 0x0E;
    write_i2c(writtenData, MAG_ADDR, 1);
    init_angle = read_i2c(MAG_ADDR) << 8;

    writtenData[0] = 0x0F;
    write_i2c(writtenData, MAG_ADDR, 1);
    init_angle |= read_i2c(MAG_ADDR);

    assert(init_angle == 0);
}
uint16_t read_i2c_raw_angle(void)
{
    uint8_t writtenData[1] = {0x0B};
    write_i2c(writtenData, MAG_ADDR, 1);
    uint8_t status = read_i2c(MAG_ADDR);
    uint16_t angle = 0;

    if (status & 0x20)
    {
        writtenData[0] = 0x0C;
        write_i2c(writtenData, MAG_ADDR, 1);
        angle = read_i2c(MAG_ADDR) << 8;

        writtenData[0] = 0x0D;
        write_i2c(writtenData, MAG_ADDR, 1);
        angle |= read_i2c(MAG_ADDR);
    }

    return angle;
}
uint16_t read_i2c_angle(void)
{
    uint8_t writtenData[1] = {0x0B};
    write_i2c(writtenData, MAG_ADDR, 1);
    uint8_t status = read_i2c(MAG_ADDR);
    uint16_t angle = 0;

    if (status & 0x20)
    {
        writtenData[0] = 0x0E;
        write_i2c(writtenData, MAG_ADDR, 1);
        angle = read_i2c(MAG_ADDR) << 8;

        writtenData[0] = 0x0F;
        write_i2c(writtenData, MAG_ADDR, 1);
        angle |= read_i2c(MAG_ADDR);
    }

    return angle;
}

void calibration_loop()
{
    uint32_t debouncer = 0;
    uint16_t angle = 0;

    while (1)
    {
        angle = read_i2c_raw_angle();

        debouncer = (debouncer << 1);
        if (GPIOA->IDR & 1)
        {
            debouncer |= 0x1;
        }

        if (debouncer == 0x7FFFFFFF)
        {
            initial_angle = angle;
            break;
        }

        // Flash red LED
        GPIOC->ODR ^= (1 << 6);
    }
    init_magenc();

    GPIOC->ODR &= ~(1 << 6);
}

typedef enum
{
    SWING_STATE1,
    SWING_STATE2,
    PID_STATE
} balance_state_t;

uint32_t abs_val(int32_t val)
{
    return (val >= 0) ? val : (val * -1);
}

uint8_t swing_angle_to_pwm(uint16_t cur_angle)
{
    return ((abs_val(cur_angle - HANG_ANGLE)) / ((SWING_VAL_RIGHT - HANG_ANGLE) / 15)) + 55;
}

void balance_loop()
{
    
    uint16_t angle = initial_angle;
    uint16_t angle_diff = 0;
    uint16_t old_angle = 0;
    uint32_t time = 1;
    uint32_t old_time = 0;
    uint32_t time_diff = 1;
    int32_t anglev = 0;
    int32_t old_anglev = 0;
    balance_state_t bstate = PID_STATE;
    uint8_t control_byte;
    uint16_t error = 0;
    uint8_t direction = 0;
    
    GPIOC->ODR ^= (1 << 7);
    allow_reverse = 0;

    USART_send_byte(USART1, 0x00);
    while (1)
    {
        angle = read_i2c_angle();
        // angle_diff = angle - old_angle;
        time = TIM2->CNT;
        if (old_time > time)
        {
            // tick_count overflowed
            time_diff = (MAX_TICK_COUNT - old_time) + time;
        }
        else if (time == old_time)
        {
            // Would not want to div by 0
            time_diff = 1;
        }
        else
        {
            time_diff = time - old_time;
        }
        anglev = (angle - old_angle) / time_diff;

        if (angle == 0)
            continue;

        // if last and current angle are both on the same side of the wheel
        if (((angle < 2000) == (old_angle < 2000)))
        {
            direction = (old_angle < angle);
        }
        else
        {
            // if they are on different sides, and the old one was on the "small" side, than the new must be on the "big" side (travelling in the one direction)
            if ((old_angle > 1000) && (old_angle < 3000))
            {
                direction = !(old_angle < 2000);
            }
            else
                direction = (old_angle < 2000);
        }
        // stop driving the motor if the stick within the threshold of the balance point, and moving in the right direction
        if (((angle < V_THRESHHOLD) && direction) || (((4095 - angle) < V_THRESHHOLD) && !direction))
            USART_send_byte(USART1, 0x00);

        else if (angle > 3700 || angle < 300)
        {
            // if the pendulum is heading towards the balance point, thne
            if ((angle > 3700) && !direction)
                USART_send_byte(USART1, (1 << 7) | (99));
            else if ((angle > 3700) && direction)
                USART_send_byte(USART1, (90));
            else if ((angle < 300) && !direction)
                USART_send_byte(USART1, (99));
            else if ((angle < 300) && direction)
                USART_send_byte(USART1, (1 << 7) | 90);
        }

        bstate = ((angle > SWING_THRES_LEFT) && (angle < SWING_THRES_RIGHT)) ? SWING_STATE1 : SWING_STATE2;
        // this prevents the motor constantly accelerating with the pendulum held steady slightly below the horizontal

        if (bstate == SWING_STATE2)
        {
            if (direction)
            {
                if ((angle < 2200) && (angle >= 1836))
                {
                    USART_send_byte(USART1, (1 << 7) | 95);
                }
                else if ((angle < 1636) && (angle >= 1000))
                {
                    USART_send_byte(USART1, 60);
                    allow_reverse = !allow_reverse;
                }
                else if ((angle < 1000) && (angle >= 200))
                {
                    USART_send_byte(USART1, 90);
                }
                else if (angle < 200)
                {
                    USART_send_byte(USART1, 0);
                    bstate = PID_STATE;
                }
            }
        }
        if (bstate == SWING_STATE1)
        {
            if (!direction)
            {
                allow_reverse = allow_reverse;
                if ((angle >= 1836) && (angle < 2260))
                {
                    USART_send_byte(USART1, 95);
                }
                else if ((angle >= 2460) && (angle < 3000))
                {
                    USART_send_byte(USART1, (1 << 7) | 60);
                    allow_reverse = !allow_reverse;
                }
                else if ((angle >= 3000) && (angle < 3895))
                {
                    USART_send_byte(USART1, 90);
                }
                else if (angle >= 3895)
                {
                    USART_send_byte(USART1, 0);
                    bstate = PID_STATE;
                }
            }
            else
            {
                // bstate == PID_STATE
                if ((angle > 200) && (angle < 2048))
                {
                    USART_send_byte(USART1, 0);
                    bstate = SWING_STATE1;
                }
                else if ((angle < 3895) && (angle >= 2048))
                {
                    USART_send_byte(USART1, 0);
                    bstate = SWING_STATE2;
                }
                else
                {
                    // Stay in PID_STATE, PID_STATE logic goes
                    if (angle > 3895 && angle < 4092)
                    {
                        USART_send_byte(USART1, (1 << 7) | (99));
                    }
                    else if (angle < 200 && angle > 5)
                    {
                        USART_send_byte(USART1, 99);
                    }
                    else
                    {
                        USART_send_byte(USART1, 0);
                    }
                }
            }
        }
        old_angle = angle;
        old_time = time;
        old_anglev = anglev;

    }
}
void send_motor_ctrl(void)
{
    // Initialize USART for bluetooth (from checkpoint 1)
    HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_RCC_USART1_CLK_ENABLE();
    HAL_RCC_TIM2_CLK_ENABLE();

    configure_TTL(USART1, HAL_RCC_GetHCLKFreq() / 115200);
    // USART1 TX Pin (connect to RX of bluetooth)
    GPIO_InitTypeDef init_pa9 = {GPIO_PIN_9 | GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 1};
    HAL_GPIO_Init(GPIOA, &init_pa9);

    // Initialize I2C for magnetic encoder (from checkpoint 2)
    init_i2c();

    // Initialize user button: PA0
    GPIOA->MODER &= ~(GPIO_MODER_MODER0_0 | GPIO_MODER_MODER0_1);
    GPIOC->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEEDR0_0 | GPIO_OSPEEDR_OSPEEDR0_1);
    GPIOC->PUPDR |= GPIO_PUPDR_PUPDR0_1;

    // Initialize timer 2
    // Upcounting edge aligned
    TIM2->CR1 &= 0xFFFFFF8F;
    TIM2->PSC = (0 | 1);
    TIM2->ARR = (0 | MAX_TICK_COUNT);
    TIM2->CR1 |= 0x00000001;

    calibration_loop();
    balance_loop();
}

void USART1_IRQHandler(void)
{
    uint8_t control_byte = (uint8_t)USART1->RDR;
    uint8_t pwm_val = control_byte & 0x7f;

    uint8_t pwm_dir = ((control_byte & 0x80) == 0x80) ? 1 : 0;

    if (pwm_val == 0)
    {
        // Brake
        pwm_setDutyCycle(0, pwm_dir + 2);
    }
    else
    {
        pwm_setDutyCycle(pwm_val, pwm_dir);
    }
}

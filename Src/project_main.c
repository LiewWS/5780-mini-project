#include "main.h"
#include "magnetic_encoder.h"
#include "hal_usart.h"
#include "motor.h"
//`#include <stdint.h>

#define KP 1
#define KI 1

void send_motor_ctrl(void);
void calibration_loop();
void balance_loop();
void init_magenc(void);
uint16_t read_i2c_raw_angle(void);

#define MAX_TICK_COUNT 65432
uint16_t initial_angle;

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
    // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, 1);
    // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 0);
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
void init_magenc(void){
    uint16_t init_angle = 0;
    uint8_t writtenData[1];

    init_angle = read_i2c_raw_angle();

    uint8_t write_zpos_data[2] = {0x01, init_angle >> 8};
    write_i2c(write_zpos_data, MAG_ADDR, 2);
    write_zpos_data[0] = 0x02;
    write_zpos_data[1] = init_angle & 0x00ff;
    write_i2c(write_zpos_data, MAG_ADDR, 2);

    writtenData[0] = 0x0E;
    write_i2c(writtenData, MAG_ADDR, 1);
    init_angle = read_i2c(MAG_ADDR) << 8;

    writtenData[0] = 0x0F;
    write_i2c(writtenData, MAG_ADDR, 1);
    init_angle |= read_i2c(MAG_ADDR);

    //if(init_angle < 50 || init_angle > 3900) HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 | GPIO_PIN_7, 1);

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
            // pwm_setDutyCycle(50, 0);
            // isCalibrating = 0;
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
    GPIOC->ODR ^= (1 << 7);
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

        int32_t dist_center; 

        /*
        if(angle <1000){
            dist_center = angle; 
        }
        else if(angle > 3000 ){
            dist_center = 4995 - angle; 
        }

        if (angle < 1023) { // was 200
            //USART_send_byte(USART1, ( 80 + ((angle)/10)));
            USART_send_byte(USART1, ( dist_center)/60);
           
        }
        else if(angle > 3071) { // was 3800
            //USART_send_byte(USART1, ( (1 << 7) |(80 + (80-(abs(angle - 3800)/2)))));
            USART_send_byte(USART1, ( (1 << 7) |(dist_center / 60)));
        }
        else {
            if (angle < 2048) {
               // USART_send_byte(USART1, (1 << 7) | 60);
               USART_send_byte(USART1, (1 << 7) | 1);
            } else {
                //USART_send_byte(USART1, 60);
                USART_send_byte(USART1, 1);
            }
            // USART_send_byte(USART1, 0x00);
        }
            */

#ifdef DEBUG
        /*
                GPIOC->ODR ^= GPIO_ODR_9;
                GPIOC->ODR ^= GPIO_ODR_8;
                //pwm_setDutyCycle(75, 3);
                uint8_t hold_pwm = 0;
                //USART_send_byte(USART1, 0x00);
                //HAL_Delay(5000);

                USART_send_byte(USART1, ((1<<7) | 30));

                //HAL_Delay(500);

                USART_send_byte(USART1, (30));

                //HAL_Delay(500);
                control_byte = angle & 0xFF;
                */
        /*
        if (control_byte < 64)
        {
            GPIOC->ODR |= (1 << 6);
        }
        else
        {
            GPIOC->ODR &= ~(1 << 6);
        }

        if ((control_byte >= 64) && (control_byte < 128))
        {
            GPIOC->ODR |= (1 << 8);
        }
        else
        {
            GPIOC->ODR &= ~(1 << 8);
        }

        if (control_byte >= 128)
        {
            GPIOC->ODR |= (1 << 9);
        }
        else
        {
            GPIOC->ODR &= ~(1 << 9);
        }
            */
        // if(angle == old_angle) continue;
        // USART_send_byte(USART1, control_byte);
#endif

        // bstate = ((angle > SWING_THRES_LEFT) && (angle < SWING_THRES_RIGHT)) ? SWING_STATE : PID_STATE;
        if (bstate == SWING_STATE2) {
            if ((angle < 2560) && (angle >= 1536)) {
                USART_send_byte(USART1, (1 << 7)| 95);
            } else if ((angle < 1536) && (angle >= 1024)) {
                USART_send_byte(USART1, 60);
            } else if ((angle < 1024) && (angle >= 200)) {
                USART_send_byte(USART1, 0);
            } else if (angle < 200) {
                bstate = PID_STATE;
            }
        } else if (bstate == SWING_STATE1) {
            if (angle >= 3072) {
                USART_send_byte(USART1, 70);
                if (angle >= 3895) {
                    bstate = PID_STATE;
                    USART_send_byte(USART1, 0);
                }
            } else {
                USART_send_byte(USART1, (1 << 7) | 95);
            }
        } else {
            //bstate == PID_STATE
            if ((angle > 200) && (angle < 2048)) {
                USART_send_byte(USART1, 0);
                bstate = SWING_STATE1;
            } else if ((angle < 3895) && (angle >= 2048)) {
                USART_send_byte(USART1, 0);
                bstate = SWING_STATE2;
            } else {
                // Stay in PID_STATE, PID_STATE logic goes
                if(angle > 3895  && angle < 4092)
                {
                    USART_send_byte(USART1,(1 << 7) | (99));
                }
                else if(angle < 200 && angle > 5){
                    USART_send_byte(USART1,  99);
                }
                else 
                {
                    USART_send_byte(USART1, 0);
                }
            }
        }

        old_angle = angle;
        old_time = time;
        old_anglev = anglev;
        // Flash blue LED
        //
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
    // USART1 RX Pin (connect to TX of bluetooth)
    // GPIO_InitTypeDef init_pa10 = {GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 1};
    // HAL_GPIO_Init(GPIOA, &init_pa10);

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

#ifdef DEBUG
/*
    uint32_t ODR_data = 0;
    if (control_byte < 64)
    {
        ODR_data |= (1 << 6);
    }
    else
    {
        ODR_data &= ~(1 << 6);
    }

    if ((control_byte >= 64) && (control_byte < 128))
    {
        ODR_data |= (1 << 8);
    }
    else
    {
        ODR_data &= ~(1 << 8);
    }

    if (control_byte >= 128)
    {
        ODR_data |= (1 << 9);
    }
    else
    {
        ODR_data &= ~(1 << 9);
    }
    GPIOC->ODR &= ~(((1 << 6 | 1 << 8 | 1 << 9)));
    GPIOC->ODR |= ODR_data;
    */
#endif
    /*
    uint8_t tentative_pwm_val = control_byte & 0x7f;
    uint8_t pwm_val = tentative_pwm_val * 15; 
    */
   uint8_t pwm_val = control_byte & 0x7f;
   
    //if(pwm_val > 100) pwm_val = 99; 



    uint8_t pwm_dir = ((control_byte & 0x80) == 0x80) ? 1 : 0;
    /*    if(control_byte == ((1 << 7) | 35)) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 1);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, 0);
    }
    else if(control_byte == 35){
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, 1);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 0);

    }
    else{
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, 1);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, 0);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 0);
    }
        */
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

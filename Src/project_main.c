#include "main.h"
#include "magnetic_encoder.h"
#include "hal_usart.h"
// #include "bluetooth_buf.h"
#include "motor.h"

// Incremented by systick interrupt
extern uint32_t tick_count;
#define MAX_TICK_COUNT 0xFFFFFFFF

void send_pwm();

void calibration_loop();
void balance_loop();

uint16_t initial_angle;

// Here we assume that angle = 2048 when hanging down
#define HANG_ANGLE        2048
#define SWING_THRES_LEFT  1024
#define SWING_THRES_RIGHT 3072
#define SWING_VAL_LEFT    1536
#define SWING_VAL_RIGHT   2560

int project_main()
{
    HAL_Init();
    SystemClock_Config();

#if defined(SENDER)
    // I2C: SDA: PB11, SCL: PB13  
    // Bluetooth RXD: PA9, TXD: PA10
    HAL_RCC_GPIOA_CLK_ENABLE();
    // Initialize user button: PA0
    GPIOA->MODER &= ~(GPIO_MODER_MODER0_0 | GPIO_MODER_MODER0_1);
    GPIOC->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEEDR0_0 | GPIO_OSPEEDR_OSPEEDR0_1);
    GPIOC->PUPDR |= GPIO_PUPDR_PUPDR0_1; 

    send_pwm();
#else
    // Enable clock to peripherals
    
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

    // Configure motor
    // motor enable: PA4
    pwm_init();
    // For motor direction, IN1: PC8, IN2:  PC9
    uint16_t motor_dir_pins = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitTypeDef init_motor_dir = {motor_dir_pins, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL};
    HAL_GPIO_Init(GPIOC, &init_motor_dir);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, 1);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 0);
    // Stop motor initially
    pwm_setDutyCycle(0);

    while (1) {
        // main loop
    }
#endif

    return 1;
}

uint16_t read_magnetic()
{
    uint8_t writtenData[1] = {0x0B};
    write_i2c(writtenData, MAG_ADDR, 1);
    uint8_t status = read_i2c(0x36);
    uint16_t angle = 0;
    if (status & 0x20) {
        writtenData[0] = 0x0C;
            write_i2c(writtenData, MAG_ADDR, 1);
            angle = read_i2c(MAG_ADDR) << 8;

            writtenData[0] = 0x0D;
            write_i2c(writtenData, MAG_ADDR, 1);
            angle |= read_i2c(MAG_ADDR);
    }
    return angle;
}

void calibration_loop()
{
    uint32_t debouncer = 0;
    uint16_t angle = 0;
    uint8_t isCalibrating = 1;
    
    while (isCalibrating) {
        angle = read_magnetic();

        debouncer = (debouncer << 1);
        if(GPIOA->IDR & 1) {
            debouncer |= 0x1;
        }

        if (debouncer == 0x7FFFFFFF) {
            initial_angle = angle;
            pwm_setDutyCycle(50);
            isCalibrating = 0;
            break;
        }

        if (angle < 1000)
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
        else
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);

        if ((angle > 1000) && (angle < 2000))
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 1);
        else
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 0);
        if ((angle > 2000) && (angle < 3000))
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, 1);
        else
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, 0);

        if (angle > 3000)
        {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, 1);
        }
        else HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, 0);
    }
}

void motor_switch_dir()
{
    GPIOC->ODR ^= GPIO_ODR_8;
    GPIOC->ODR ^= GPIO_ODR_9;
}

typedef enum {
    SWING_STATE, PID_STATE
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
    uint8_t bt_data = 0;
    uint16_t angle = initial_angle;
    uint16_t old_angle = 0;
    uint32_t time = 1;
    uint32_t old_time = 0;
    uint32_t time_diff = 1;
    int32_t anglev = 0;
    int32_t old_anglev = 0;
    balance_state_t bstate = SWING_STATE;

    while (1) {
        angle = read_magnetic();
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

        bstate = ((angle > SWING_THRES_LEFT) && (angle < SWING_THRES_RIGHT)) ? SWING_STATE : PID_STATE;
        if (bstate == SWING_STATE) {
            if (abs_val(anglev) < abs_val(old_anglev)) {
                // Passed point of max velocity
                bt_data = swing_angle_to_pwm(angle);
                // Switch direction
                // Set bit 8 to indicate that direction should be flipped.
                bt_data |= (1 << 8);
                USART_send_byte(USART1, bt_data);
            }
        } else if (bstate == PID_STATE) {
            //
        }

        old_angle = angle;
        old_time = time;
        old_anglev = anglev;
    }
}

void send_pwm()
{
    // Initialize USART for bluetooth (from checkpoint 1)
    HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_RCC_USART1_CLK_ENABLE();

    configure_TTL(USART1, HAL_RCC_GetHCLKFreq() / 9600);
    // USART1 TX Pin (connect to RX of bluetooth)
    GPIO_InitTypeDef init_pa9 = {GPIO_PIN_9 | GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 1};
    HAL_GPIO_Init(GPIOA, &init_pa9);
    // USART1 RX Pin (connect to TX of bluetooth)
    // GPIO_InitTypeDef init_pa10 = {GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 1};
    // HAL_GPIO_Init(GPIOA, &init_pa10);

    // Initialize I2C for magnetic encoder (from checkpoint 2)
    init_i2c();
    uint8_t writtenData[1] = {0x0B};
    write_i2c(writtenData, MAG_ADDR, 1);
    uint8_t status = read_i2c(MAG_ADDR);

    calibration_loop();
    balance_loop();
}

void USART1_IRQHandler(void)
{
    uint8_t data = USART1->RDR;
    pwm_setDutyCycle(data & 0x7F);
    if ((data & (1 << 8)) > 0) {
        motor_switch_dir();
    }
}


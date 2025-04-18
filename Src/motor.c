/* -------------------------------------------------------------------------------------------------------------
 *  Motor Control and Initialization Functions
 * -------------------------------------------------------------------------------------------------------------
 */
#include "motor.h"



volatile int16_t error_integral = 0;    // Integrated error signal
volatile uint8_t duty_cycle = 0;    	// Output PWM duty cycle
volatile int16_t target_rpm = 0;    	// Desired speed target
volatile int16_t motor_speed = 0;   	// Measured motor speed
volatile int8_t adc_value = 0;      	// ADC measured motor current
volatile int16_t error = 0;         	// Speed error signal
volatile uint8_t Kp = 25;            	// Proportional gain
volatile uint8_t Ki = 1;            	// Integral gain

volatile uint8_t direction = 0;


// Sets up the entire motor drive system
void motor_init(void) {
    pwm_init();
    tim6_init();
}

void LED_init(void) {
    // Initialize PC8 and PC9 for LED's
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;                                          // Enable peripheral clock to GPIOC
    GPIOC->MODER |= GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0;                  // Set PC8 & PC9 to outputs
    GPIOC->OTYPER &= ~(GPIO_OTYPER_OT_8 | GPIO_OTYPER_OT_9);                    // Set to push-pull output type
    GPIOC->OSPEEDR &= ~((GPIO_OSPEEDR_OSPEEDR8_0 | GPIO_OSPEEDR_OSPEEDR8_1) |
                        (GPIO_OSPEEDR_OSPEEDR9_0 | GPIO_OSPEEDR_OSPEEDR9_1));   // Set to low speed
    GPIOC->PUPDR &= ~((GPIO_PUPDR_PUPDR8_0 | GPIO_PUPDR_PUPDR8_1) |
                      (GPIO_PUPDR_PUPDR9_0 | GPIO_PUPDR_PUPDR9_1));             // Set to no pull-up/down
    GPIOC->ODR &= ~(GPIO_ODR_9);                                   // Shut off LED's
    GPIOC->ODR |= GPIO_ODR_8;
}

// Sets up the PWM and direction signals to drive the H-Bridge
void pwm_init(void) {
    
    // Set up pin PA4 for H-bridge PWM output (TIMER 14 CH1)
    GPIOA->MODER |= (1 << 9);
    GPIOA->MODER &= ~(1 << 8);

    // Set PA4 to AF4,
    GPIOA->AFR[0] &= 0xFFF0FFFF; // clear PA4 bits,
    GPIOA->AFR[0] |= (1 << 18);

    // Set up a PA5, PA6 as GPIO output pins for motor direction control
    //GPIOA->MODER &= ~(GPIO_MODER_MODER5 | GPIO_MODER_MODER6); // clear PA5, PA6 bits,
    //GPIOA->MODER |= (1 << GPIO_MODER_MODER5_Pos) | (1 << GPIO_MODER_MODER6_Pos);
    uint16_t ledPins = GPIO_PIN_6 | GPIO_PIN_5;

    // init LEDs in case needed for debugging
    GPIO_InitTypeDef initMTROUT = {ledPins,
                                GPIO_MODE_OUTPUT_PP,
                                GPIO_SPEED_FREQ_LOW,
                                GPIO_NOPULL};
    HAL_GPIO_Init(GPIOA, &initMTROUT);
    //GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER5 | GPIO_MODER_MODER6)) | GPIO_MODER_MODER13_1 | GPIO_MODER_MODER11_1;

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 0);
    //Initialize one direction pin to high, the other low
     //GPIOA->ODR |= (1 << 6);
     //GPIOA->ODR &= ~(1 << 5);

    // GPIOA->ODR &= ~(1 << 6);
    //  GPIOA->ODR |= (1 << 5);

    // Set up PWM timer
    RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
    TIM14->CR1 = 0;                         // Clear control registers
    TIM14->CCMR1 = 0;                       // (prevents having to manually clear bits)
    TIM14->CCER = 0;

    // Set output-compare CH1 to PWM1 mode and enable CCR1 preload buffer
    TIM14->CCMR1 |= (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE);
    TIM14->CCER |= TIM_CCER_CC1E;           // Enable capture-compare channel 1
    TIM14->PSC = 1;                         // Run timer on 24Mhz
    TIM14->ARR = 1200;                      // PWM at 20kHz
    TIM14->CCR1 = 0;                        // Start PWM at 0% duty cycle
    
    TIM14->CR1 |= TIM_CR1_CEN;              // Enable timer
}

void tim6_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    
    // Select PSC and ARR values that give an appropriate interrupt rate
    TIM6->PSC = 11;
    TIM6->ARR = 30000;
    
    TIM6->DIER |= TIM_DIER_UIE;             // Enable update event interrupt
    TIM6->CR1 |= TIM_CR1_CEN;               // Enable Timer

    NVIC_EnableIRQ(TIM6_DAC_IRQn);          // Enable interrupt in NVIC
    NVIC_SetPriority(TIM6_DAC_IRQn,2);
}

void TIM6_DAC_IRQHandler(void) {

    
    
    // Call the PI update function


    pwm_update(direction);
    TIM6->SR &= ~TIM_SR_UIF;        // Acknowledge the interrupt

}

// Set the duty cycle of the PWM, accepts (0-100)
void pwm_setDutyCycle(uint8_t duty) {
    if(duty <= 100) {
        TIM14->CCR1 = ((uint32_t)duty*TIM14->ARR)/100;  // Use linear transform to produce CCR1 value
        // (CCR1 == "pulse" parameter in PWM struct used by peripheral library)
    }
}

void pwm_update(uint8_t dir)
{   
    // if(dir == 1) pwm_setDutyCycle(57);
    // else pwm_setDutyCycle(43);

    pwm_setDutyCycle(60);

}

void motor_main(void)
{
    HAL_Init();                             // Initialize HAL internals
    SystemClock_Config();
    LED_init();

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    pwm_init();
    pwm_setDutyCycle(60);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 1);
    //uint8_t forward = 1;

    while(1)
    {
        
        HAL_Delay(3000);

        // HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5 | GPIO_PIN_6);
        GPIOC->ODR ^= GPIO_ODR_9;
        GPIOC->ODR ^= GPIO_ODR_8;

       // direction = (direction == 0);

        //GPIOA->ODR ^=  (1 << 5);
        //GPIOA->ODR ^= (1 << 6);



    }

}




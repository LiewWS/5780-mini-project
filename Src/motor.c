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
    
    // Set up pin PA4 and PA2 for H-bridge PWM output (TIMER 14 CH1)
    GPIOA->MODER |= (1 << 9);

    GPIOA->MODER &= ~(1 << 8);

    //tim3
    GPIO_InitTypeDef tim3 = {GPIO_PIN_6, 
        GPIO_MODE_AF_PP, 
        GPIO_SPEED_FREQ_LOW, 
        GPIO_NOPULL};

    HAL_GPIO_Init(GPIOC, &tim3);

    // Set PA4 to AF4,
    GPIOA->AFR[0] &= 0xFF0FFFF; // clear PA4,
    GPIOA->AFR[0] |= (1 << 18);

    GPIOC->AFR[0] &= ~(0xF << (6 * 4));  // Clear bits
    //GPIOC->AFR[0] |= (mode << (6 * 4));

    // Set up a PA5, PA6 as GPIO output pins for motor direction control

    uint16_t ledPins = GPIO_PIN_6 | GPIO_PIN_5;

    // init LEDs in case needed for debugging
    GPIO_InitTypeDef initMTROUT = {ledPins,
                                GPIO_MODE_OUTPUT_PP,
                                GPIO_SPEED_FREQ_LOW,
                                GPIO_NOPULL};
    HAL_GPIO_Init(GPIOA, &initMTROUT);
   

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 0);


    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    //timer 3 setup from lab 3
    TIM3->ARR = 1200;
    TIM3->PSC = 1;
    //set channels to output
    TIM3->CCMR1 &= ~(0x3);
    TIM3->CCMR1 &= ~(0x3 << 8);
    
    //set PWM modes
    TIM3->CCMR1 |= (0x6 << 4);
    TIM3->CCMR1 = (TIM3->CCMR1 &= ~(0x7 << 12)) | (0x6 << 12);

    //enable channels
    TIM3->CCER |= 0x1;
    TIM3->CCER |= (0x1 << 4);

    //set the capture/compare reg
    TIM3->CCR1 = 0;
    TIM3->CCR2 = 0;
    // Set up PWM timers
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
    TIM3->CR1 |= TIM_CR1_CEN;
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


    //pwm_update(direction);
    TIM6->SR &= ~TIM_SR_UIF;        // Acknowledge the interrupt

}

// Set the duty cycle of the PWM, accepts (0-100)
void pwm_setDutyCycle(uint8_t duty, uint8_t dir) {
    if(duty <= 100 && dir == 1) {
        TIM14->CCR1 = ((uint32_t)duty*TIM14->ARR)/100;
        TIM3->CCR1 = 0;
    }
    else if(duty <= 100 && dir == 0) {
        TIM3->CCR1 = ((uint32_t)duty*TIM3->ARR)/100;
        TIM14->CCR1 = 0;
    }
    else if(dir == 2) {
        TIM3->CCR1 = TIM3-> ARR + 1;
        TIM14->CCR1 = TIM14-> ARR + 1;
    }
    else if(dir == 3)
    {
        TIM3->CCR1 = 0;
        TIM14->CCR1 = 0;
    }
}

void pwm_update(uint8_t pwm, uint8_t dir)
{   
    // if(dir == 1) pwm_setDutyCycle(57);
    // else pwm_setDutyCycle(43);

    pwm_setDutyCycle(pwm, dir);

}

void motor_main(void)
{
    HAL_Init();                             // Initialize HAL internals
    //SystemClock_Config();
    LED_init();

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    pwm_init();
    //pwm_setDutyCycle(75, 3);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 1);
    //HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 1);
    //uint8_t forward = 1;

    volatile int i = 500;
    HAL_Delay(500);

    while(1)
    {


        // HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5 | GPIO_PIN_6);
        GPIOC->ODR ^= GPIO_ODR_9;
        GPIOC->ODR ^= GPIO_ODR_8;

        pwm_setDutyCycle(75, 3);

        HAL_Delay(5000);
       
        pwm_setDutyCycle(95, 1);

        HAL_Delay(500);

        pwm_setDutyCycle(95, 0);

        HAL_Delay(500);




        //---------------cycle
        // pwm_setDutyCycle(90, 1);
        // HAL_Delay(250);
    
        // pwm_setDutyCycle(95, 0);
        // HAL_Delay(275);
    
        // pwm_setDutyCycle(50, 3);
        // HAL_Delay(20);
    
        // pwm_setDutyCycle(90, 0);
        // HAL_Delay(250);
    
        // pwm_setDutyCycle(95, 1);
        // HAL_Delay(250);

        // pwm_setDutyCycle(50, 3);
        // HAL_Delay(20);
        //------------------------
        

        // direction = (direction == 0);


        

       // direction = (direction == 0);

        //GPIOA->ODR ^=  (1 << 5);
        //GPIOA->ODR ^= (1 << 6);



    }

}




#include "magnetic_encoder.h"


uint16_t I2C2_Read_ISR(uint32_t bit);
void I2C_Write(uint8_t data);
uint8_t I2C_Read();


void My_HAL_GPIO_AF(GPIO_TypeDef *GPIOx, uint16_t pin, uint16_t mode);

int magnetic_encoder_main(void)
{
    HAL_Init();
    init_i2c();
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9, 0);
    uint8_t writtenData[1] = {0x0B};
    write_i2c(writtenData, MAG_ADDR, 1);
    uint8_t status = read_i2c(MAG_ADDR);
    uint16_t angle = 0;
    while (1)
    {
        writtenData[0] = 0x0B;
        write_i2c(writtenData, MAG_ADDR, 1);
        uint8_t status = read_i2c(0x36);
        // if(status & 0x08) HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6,1);
        // else HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6,0);
        //if (status & 0x10)

            if (status & 0x20)
            {
                //HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, 1);
                writtenData[0] = 0x0C;
                write_i2c(writtenData, MAG_ADDR, 1);
                angle = read_i2c(MAG_ADDR) << 8;

                writtenData[0] = 0x0D;
                write_i2c(writtenData, MAG_ADDR, 1);
                angle |= read_i2c(MAG_ADDR);

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

    // HAL_Init();
    // SystemClock_Config();

    // HAL_RCC_USART3_CLK_ENABLE();
    // HAL_RCC_GPIOB_CLK_ENABLE();
    // HAL_RCC_GPIOC_CLK_ENABLE();
    // HAL_RCC_I2C2_CLK_ENABLE();
    // RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;

    // initialize the GPIO pins for the LEDs
    /*
    GPIO_InitTypeDef initStrLED = {GPIO_PIN_6 | GPIO_PIN_7 |GPIO_PIN_8 | GPIO_PIN_9,
                                GPIO_MODE_OUTPUT_PP,
                                GPIO_SPEED_FREQ_LOW,
                                GPIO_NOPULL};

                                HAL_GPIO_Init(GPIOC, &initStrLED);
                                */
    // initalize I2C SCL and SDA
    /*
    GPIO_InitTypeDef initStrSDA = {GPIO_PIN_11, GPIO_MODE_AF_OD};
    HAL_GPIO_Init(GPIOB, &initStrSDA);

    GPIO_InitTypeDef initStrSCL = {GPIO_PIN_13, GPIO_MODE_AF_OD};
    HAL_GPIO_Init(GPIOB, &initStrSCL);

    GPIO_InitTypeDef initStr1 = {GPIO_PIN_14, GPIO_MODE_OUTPUT_PP};
    HAL_GPIO_Init(GPIOB, &initStr1);

    GPIO_InitTypeDef initStr2 = {GPIO_PIN_0, GPIO_MODE_OUTPUT_PP};
    HAL_GPIO_Init(GPIOC, &initStr2);
    */
    /*
    GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODER13 | GPIO_MODER_MODER11)) | GPIO_MODER_MODER13_1 | GPIO_MODER_MODER11_1;

    GPIOB->OTYPER |= (GPIO_OTYPER_OT_11 | GPIO_OTYPER_OT_13);

    // set AF1 and AF5
    GPIOB->AFR[1] |= 0x01 << GPIO_AFRH_AFSEL11_Pos;
    GPIOB->AFR[1] |= 0x05 << GPIO_AFRH_AFSEL13_Pos;

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
    */
    // My_HAL_GPIO_AF(GPIOB, 11, 1);
    // My_HAL_GPIO_AF(GPIOB, 13, 5);

    // setup USART for debugging
    // setup_USART();

    // USART_send_string(USART3,"START");
    /*

    I2C2->TIMINGR = I2C2->TIMINGR & ((~I2C_TIMINGR_PRESC_Msk) | (1 << I2C_TIMINGR_PRESC_Pos));
    I2C2->TIMINGR = I2C2->TIMINGR & ((~I2C_TIMINGR_SCLL_Msk) | (0x13 << I2C_TIMINGR_SCLL_Pos));
    I2C2->TIMINGR = I2C2->TIMINGR & ((~I2C_TIMINGR_SCLH_Msk) | (0xF << I2C_TIMINGR_SCLH_Pos));
    I2C2->TIMINGR = I2C2->TIMINGR & ((~I2C_TIMINGR_SDADEL_Msk) | (0x2 << I2C_TIMINGR_SDADEL_Pos));
    I2C2->TIMINGR = I2C2->TIMINGR & ((~I2C_TIMINGR_SCLDEL_Msk) | (0x4 << I2C_TIMINGR_SCLDEL_Pos));

    I2C2->CR1 |= I2C_CR1_PE_Msk;
    */

    /*
    I2C2->CR2 &= ~(I2C_CR2_SADD_Msk | I2C_CR2_RD_WRN_Msk | I2C_CR2_START_Msk | 0x7F << 16);

    I2C2->CR2 |= ((0x36 << 1) | (1 << I2C_CR2_NBYTES_Pos) | I2C_CR2_RD_WRN_Msk);
    I2C2->CR2 |= I2C_CR2_START_Msk;
    */
    // printR("CR2", I2C2->CR2); //print cr2 to verify setup

    // USART_send_byte(USART3, '\n');
    // USART_send_byte(USART3, '\r');
    /*
    I2C2->CR2 |= (I2C_CR2_START_Msk); //set start

    //printR("ISR", I2C2->ISR); //print isr

    //USART_send_byte(USART3, '\n');
    //USART_send_byte(USART3, '\r');

    //USART_send_string(USART3,"checkpoint 1");

    while (!((I2C2->ISR & I2C_ISR_TXIS_Msk) | (I2C2->ISR & I2C_ISR_NACKF_Msk)))
        {
        }
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);

       // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 1);
        if (I2C2->ISR & I2C_ISR_NACKF_Msk)
        {
        }
        else if (I2C2->ISR & I2C_ISR_TXIS_Msk)
        {

            I2C2->TXDR = 0x0B;
        }


    // USART_send_string(USART3,"checkpoint 2");

    I2C_Write(0x0B);

    printR("ISR", I2C2->ISR); //print isr

    USART_send_byte(USART3, '\n');
    USART_send_byte(USART3, '\r');

    //If NACK end program
    if (I2C2_Read_ISR(1) == 0)
    {
        USART_send_string(USART3, "ERROR NACK 1");
        return;
    }


    while((I2C2->ISR & (1 << 6)) == 0);

    I2C2->CR2 &= ~(0x3FF); //clear SADD
    I2C2->CR2 &= ~(0xFF << 16); //clear NBYTES

    I2C2->CR2 |= (0x36 << 1); //set slave address
    I2C2->CR2 |= (1 << 16); //set NBYTES
    I2C2->CR2 |= (1 << 10); //set RD_WRN = 1

    if (I2C2_Read_ISR(2) == 0)
    {
        USART_send_string(USART3, "ERROR NACK 1");

        return;
    }

    //while((I2C2->ISR & (1 << 6)) == 0);

    printR("status", I2C2->RXDR);
    */
}

// Function to help read the ISR register
uint16_t I2C2_Read_ISR(uint32_t bit)
{

    while (((I2C2->ISR & (1 << 4)) == 0) && ((I2C2->ISR & (1 << bit)) == 0))
        ;

    printR("read ISR", I2C2->ISR); // print isr
    if (I2C2->ISR & (1 << 4))
        return 0;

    return 1;
}

// fucntion to help wrtire data through the TXDR
void I2C_Write(uint8_t data)
{
    // transmit_String("write");

    I2C2->TXDR = data;

    // printR("write data", I2C2->TXDR);
}

// functino to help read the RXDR
uint8_t I2C_Read()
{
    // transmit_String("read");
    // printR("read data", I2C2->RXDR);

    return I2C2->RXDR;
}




void My_HAL_GPIO_AF(GPIO_TypeDef *GPIOx, uint16_t pin, uint16_t mode)
{
    // make sure teh mode is 0-7
    assert(mode >= 0 && mode <= 7);

    // make sure the pin is in the range of 0-15
    assert(pin >= 0 && pin <= 15);

    // make sure the GPIO passed
    assert_param(IS_GPIO_ALL_INSTANCE(GPIOx));

    if (pin <= 7)
    {
        GPIOx->AFR[0] &= ~(0xF << (pin * 4)); // Clear bits
        GPIOx->AFR[0] |= (mode << (pin * 4)); // set bits
    }
    else
    {
        pin -= 8;
        GPIOx->AFR[0] &= ~(0xF << (pin * 4));
        GPIOx->AFR[1] |= (mode << (pin * 4));
    }
}

void init_i2c()
{
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    // include pin 0 to be used with I2C
    uint16_t ledPins = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_0;

    // init LEDs in case needed for debugging
    GPIO_InitTypeDef initLED = {ledPins,
                                GPIO_MODE_OUTPUT_PP,
                                GPIO_SPEED_FREQ_LOW,
                                GPIO_NOPULL};

    HAL_GPIO_Init(GPIOC, &initLED);

    /*
    GPIO_InitTypeDef initI2C_11_13 = {GPIO_PIN_11 | GPIO_PIN_13,
                                      GPIO_MODE_AF_OD};
*/
    // HAL_GPIO_Init(GPIOB, &initI2C_11_13);

    GPIO_InitTypeDef initI2C_14 = {GPIO_PIN_14,
                                   GPIO_MODE_OUTPUT_PP};

    HAL_GPIO_Init(GPIOB, &initI2C_14);

    // ensure AF mode is selected (should already be done)

    GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODER13 | GPIO_MODER_MODER11)) | GPIO_MODER_MODER13_1 | GPIO_MODER_MODER11_1;

    GPIOB->OTYPER |= (GPIO_OTYPER_OT_11 | GPIO_OTYPER_OT_13);

    // set AF1 and AF5
    GPIOB->AFR[1] |= 0x01 << GPIO_AFRH_AFSEL11_Pos; // sda
    GPIOB->AFR[1] |= 0x05 << GPIO_AFRH_AFSEL13_Pos; // scl

    // SET PB14 and PC0 to high
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, 1);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, 1);

    // enable I2C2 periph
    RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;

    // NOTE: If this is not working, try resetting these bits and then setting them
    /*
    I2C2->TIMINGR |= 1 << I2C_TIMINGR_PRESC_Pos;
    I2C2->TIMINGR |= 0x13 << I2C_TIMINGR_SCLL_Pos;
    I2C2->TIMINGR |= 0xF << I2C_TIMINGR_SCLH_Pos;
    I2C2->TIMINGR |= 0x2 << I2C_TIMINGR_SDADEL_Pos;
    I2C2->TIMINGR |= 0x4 << I2C_TIMINGR_SCLDEL_Pos;
        */
    // This could be necessary, not sure yet

    I2C2->TIMINGR = I2C2->TIMINGR & ((~I2C_TIMINGR_PRESC_Msk) | (1 << I2C_TIMINGR_PRESC_Pos));
    I2C2->TIMINGR = I2C2->TIMINGR & ((~I2C_TIMINGR_SCLL_Msk) | (0x13 << I2C_TIMINGR_SCLL_Pos));
    I2C2->TIMINGR = I2C2->TIMINGR & ((~I2C_TIMINGR_SCLH_Msk) | (0xF << I2C_TIMINGR_SCLH_Pos));
    I2C2->TIMINGR = I2C2->TIMINGR & ((~I2C_TIMINGR_SDADEL_Msk) | (0x2 << I2C_TIMINGR_SDADEL_Pos));
    I2C2->TIMINGR = I2C2->TIMINGR & ((~I2C_TIMINGR_SCLDEL_Msk) | (0x4 << I2C_TIMINGR_SCLDEL_Pos));

    I2C2->CR1 |= I2C_CR1_PE_Msk;
}
uint8_t read_i2c(uint8_t addr)
{
    I2C2->CR2 &= ~(I2C_CR2_SADD_Msk | I2C_CR2_RD_WRN_Msk | I2C_CR2_START_Msk | 0x7F << 16);

    I2C2->CR2 |= ((addr << 1) | (1 << I2C_CR2_NBYTES_Pos) | I2C_CR2_RD_WRN_Msk);
    I2C2->CR2 |= I2C_CR2_START_Msk;

    while (!((I2C2->ISR & I2C_ISR_RXNE_Msk) | (I2C2->ISR & I2C_ISR_NACKF_Msk)))
    {
    }
    if (I2C2->ISR & I2C_ISR_NACKF_Msk)
    {
        return 1;
    }
    else if (I2C2->ISR & I2C_ISR_RXNE_Msk)
    {
        while (!(I2C2->ISR & I2C_ISR_TC_Msk))
        {
        }

        uint8_t hold = I2C2->RXDR;
        I2C2->CR2 |= I2C_CR2_STOP_Msk;
        return hold;
    }
}
uint8_t write_i2c(uint8_t *sent_dat, uint8_t sent_addr, uint8_t num_bytes)
{
    I2C2->CR2 &= ~(I2C_CR2_SADD_Msk | I2C_CR2_RD_WRN_Msk | I2C_CR2_START_Msk | 0x7F << 16);

    I2C2->CR2 |= ((sent_addr << 1) | (num_bytes << I2C_CR2_NBYTES_Pos) | I2C_CR2_START_Msk);

    for (int i = 0; i < num_bytes; i++)
    {

        while (!((I2C2->ISR & I2C_ISR_TXIS_Msk) | (I2C2->ISR & I2C_ISR_NACKF_Msk)))
        {
        }
       
        
        if (I2C2->ISR & I2C_ISR_NACKF_Msk)
        {
            return 1;
        }
        else if (I2C2->ISR & I2C_ISR_TXIS_Msk)
        {

            I2C2->TXDR = sent_dat[i];
        }
    }
    while (!(I2C2->ISR & I2C_ISR_TC_Msk))
    {
    }
    I2C2->ISR &= ~(I2C_ISR_TC_Msk);
    // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
    return 0;
}
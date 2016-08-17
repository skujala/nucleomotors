/*
 * Copyright (C) 2016 Sami Kujala <skujala@iki.fi>
 *
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/stm32/f4/rcc.h>

static void nvic_setup(void)
{

}

static void rcc_setup(void)
{
  rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_84MHZ]);

  /* Enable GPIOD clock for LED & USARTs. */
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);

  /* Enable clocks for USART2. */
  rcc_periph_clock_enable(RCC_USART2);

  /* Enable timers for PWM */
  rcc_periph_clock_enable(RCC_TIM2);
  rcc_periph_clock_enable(RCC_TIM3);
}

static void usart_setup(void)
{
  /* Setup USART2 parameters. */
  usart_set_baudrate(USART2, 115200);
  usart_set_databits(USART2, 8);
  usart_set_stopbits(USART2, USART_STOPBITS_1);
  usart_set_mode(USART2, USART_MODE_TX);
  usart_set_parity(USART2, USART_PARITY_NONE);
  usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

  /* Finally enable the USART. */
  usart_enable(USART2);
}

static void gpio_setup(void)
{
  /*** ONBOARD LED ***/
  
  /* Setup GPIO pin GPIO5 on GPIO port A for LED. */
  gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);


  /*** L6474 DIRECTION PINS for X-NUCLEO-IHM01A1 ***/
  gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8);
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
  
  
  /*** L6474 STBY\\RESET pin ***/  
  gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO9);

  /*** L6474 FLAG OUTPUT pin ***/  
  gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO10);


  /*** USART2 ***/

  /* Setup GPIO pins for USART2 transmit. */
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2|GPIO3);
  gpio_set_af(GPIOA, GPIO_AF7, GPIO2|GPIO3);
  

  /*** PWM TIMERS for X-NUCLEO-IHM01A1 PWM1 (PC7/TIM3_CH2) and PWM2 (PB3/TIM2_CH2) ***/

  /* Setup GPIO PB3 (tim2_ch2) and PC7 (tim3_ch2) alternate functions */
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
  gpio_set_af(GPIOB, GPIO_AF1, GPIO2);
  
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);
  gpio_set_af(GPIOC, GPIO_AF3, GPIO6);
  
  
  /*** SPI1 ***/

  /* CS */
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
  
  /* SPI1 @ PA5-PA7 */
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO5 | GPIO6 | GPIO7);
  gpio_set_af(GPIOA, GPIO_AF5, GPIO5 | GPIO6 | GPIO7);
  
  /* 
    Not sure if this is really needed -- copied from libopencm3-example, e.g., why only
    SPI1_SCK and SPI1_MOSI are set, and SPI1_MISO is left at default? - probably as MISO is
    input...
  */
  gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO5 | GPIO7);
}

static void tim2_setup()
{
  
}

static void tim3_setup()
{
  
}


static void spi_setup()
{
  spi_clean_disable(SPI1);
  
  /* Software NSS */
  spi_disable_software_slave_management(SPI1);
  
  /* 
    L6474 max SPI CLOCK frequency 5 MHz -> NUCLEO Clock rate 84 MHz -> need to divide by 32,
    resulting in 2.625 MHz clock frequency. Probably acceptable
  */
  
  spi_init_master(SPI1, 
    SPI_CR1_BAUDRATE_FPCLK_DIV_32, 
    SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE, 
    SPI_CR1_CPHA_CLK_TRANSITION_1, 
    SPI_CR1_DFF_8BIT, 
    SPI_CR1_MSBFIRST
  );
  
  
  /* Pull cable select UP as no comms will yet happen */    
  gpio_set(GPIOB, GPIO5);
  
  spi_enable(SPI1);
}




int main(void)
{
  rcc_setup();
  nvic_setup();
  gpio_setup();
  usart_setup();
  spi_setup();
  
  
  
}

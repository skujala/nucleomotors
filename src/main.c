/*
 * Copyright (C) 2016 Sami Kujala <skujala@iki.fi>
 *
 * This is work in progress.
 */

#include <libopencmsis/core_cm3.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/stm32/f4/rcc.h>

#include <stdlib.h>

static volatile uint32_t m1_target_period; /* Units steps/sec */

typedef enum {
  STATE_ACCELERATING,
  STATE_DECELERATING,
  STATE_STEADYSTATE,
  STATE_STOPPED
} motor_state_t;

volatile motor_state_t motor1_state;
volatile motor_state_t motor2_state;
volatile uint32_t step_count;
volatile float last_period;


static volatile uint32_t system_millis;

static void nvic_setup(void)
{
  nvic_enable_irq(NVIC_TIM2_IRQ);
}

static void rcc_clock_setup(void)
{
  /* Modified from libopencm3 library function rcc_clock_setup_hse_3v3. Could be refactored. */
  
  /* Enable internal high-period oscillator. */
  rcc_osc_on(RCC_HSI);
  rcc_wait_for_osc_ready(RCC_HSI);
  
  /* Select HSI as SYSCLK source. */
  rcc_set_sysclk_source(RCC_CFGR_SW_HSI);
  
  /* Enable external high-period oscillator 8MHz. */
  rcc_osc_on(RCC_HSE);
  rcc_wait_for_osc_ready(RCC_HSE);
  
  /* Enable high performance mode */
  pwr_set_vos_scale(PWR_SCALE1);
  
  /*
   * Set prescalers for AHB, ADC, ABP1, ABP2.
   * Do this before touching the PLL (TODO: why?).
   */
  rcc_set_hpre(RCC_CFGR_HPRE_DIV_NONE);
  rcc_set_ppre1(RCC_CFGR_PPRE_DIV_2);
  rcc_set_ppre2(RCC_CFGR_PPRE_DIV_NONE);
  
  rcc_set_main_pll_hse(
      8, /* PLL_M */
    384, /* PLL_N */
      4, /* PLL_P */ 
      2  /* PLL_Q */
  );
  
  /* Enable PLL oscillator and wait for it to stabilize. */
  rcc_osc_on(RCC_PLL);
  rcc_wait_for_osc_ready(RCC_PLL);
  
  /* Configure flash settings. */
  flash_set_ws(FLASH_ACR_ICE | FLASH_ACR_DCE | FLASH_ACR_LATENCY_3WS);
  
  /* Select PLL as SYSCLK source. */
  rcc_set_sysclk_source(RCC_CFGR_SW_PLL);
  
  /* Wait for PLL clock to be selected. */
  rcc_wait_for_sysclk_status(RCC_PLL);
  
  /* Set the peripheral clock frequencies used. */
  rcc_ahb_frequency  = 96000000;
  rcc_apb1_frequency = 48000000;
  rcc_apb2_frequency = 96000000;
  
  /* Disable internal high-period oscillator. */
  rcc_osc_off(RCC_HSI);
}

static void rcc_setup(void)
{
  rcc_clock_setup();

  /* Enable GPIOD clock for LED & USARTs. */
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);

  /* Enable clocks for USART2. */
  rcc_periph_clock_enable(RCC_USART2);

  /* Enable timers for PWM and timing */
  rcc_periph_clock_enable(RCC_TIM2);
  rcc_periph_clock_enable(RCC_TIM3);
    
  rcc_periph_clock_enable(RCC_SPI1);
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
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO3);
  gpio_set_af(GPIOB, GPIO_AF1, GPIO3);
  
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO7);
  gpio_set_af(GPIOC, GPIO_AF2, GPIO7);
    
    
  /*** SPI1 ***/

  /* CS */
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
  
  /* SPI1 @ PA5-PA7 */
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO5 | GPIO6 | GPIO7);
  gpio_set_af(GPIOA, GPIO_AF5, GPIO5 | GPIO6 | GPIO7);
  
}


/* Combining the two functions below would be a useful DRY refactor */
static void tim2_setup(void)
{
  timer_reset(TIM2);
  timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
                 TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

  timer_set_prescaler(TIM2, 1);
  timer_enable_preload(TIM2);
  
  timer_continuous_mode(TIM2);

  timer_set_period(TIM2, 0x80000);
  timer_set_oc_mode(TIM2, TIM_OC2, TIM_OCM_PWM1);
  timer_enable_oc_preload(TIM2, TIM_OC2);
  timer_enable_oc_output(TIM2, TIM_OC2);
  timer_set_oc_value(TIM2, TIM_OC2, 0x80000 >> 1);
    
  timer_set_counter(TIM2, 0x00000000);
  
  /* Reference manual for STM32F411 states:

    As the preload registers are transferred to the shadow registers only when an update event
    occurs, before starting the counter, you have to initialize all the registers by setting
    the UG bit in the TIMx_EGR register.

   */
  timer_enable_irq(TIM2, TIM_DIER_CC2IE);
  timer_generate_event(TIM2, TIM_EGR_UG);
}

void tim2_isr(void)
{  
  float next_period;
  uint32_t next_period_trunc;
  
  if (timer_get_flag(TIM2, TIM_SR_CC2IF)) {
    timer_clear_flag(TIM2, TIM_SR_CC2IF);
        
    last_period = TIM2_ARR;
    
    if (last_period > m1_target_period) {
      
      if (step_count == 1) {
        next_period = 0.4056 * last_period;
      } else {
        next_period = last_period - 2. * last_period / (4. * step_count + 1.);        
      }
      
      next_period_trunc = next_period;
      timer_set_period(TIM2, next_period_trunc);
      timer_set_oc_value(TIM2, TIM_OC2, next_period_trunc >> 1);
    }
    
    
    if (step_count++ > 0x20000) {
      timer_disable_counter(TIM2);
    };
    
  }
}


static void tim3_setup(void)
{
  timer_reset(TIM3);
  timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT,
                 TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

  timer_set_prescaler(TIM3, 1);
  timer_enable_preload(TIM3);
  
  timer_continuous_mode(TIM3);

  timer_set_period(TIM3, 0x1000);
  
  timer_set_oc_mode(TIM3, TIM_OC2, TIM_OCM_PWM1);
  timer_enable_oc_preload(TIM3, TIM_OC2);
  timer_enable_oc_output(TIM3, TIM_OC2);
  timer_set_oc_value(TIM3, TIM_OC2, 0x800);

  timer_set_counter(TIM3, 0x0000);

  /* Reference manual for STM32F411 states:

    As the preload registers are transferred to the shadow registers only when an update event
    occurs, before starting the counter, you have to initialize all the registers by setting
    the UG bit in the TIMx_EGR register.

   */
  timer_generate_event(TIM3, TIM_EGR_UG);
}





static void spi_setup(void)
{
  /* 
    L6474 SPI CLOCK frequency max 5 MHz -> SPI1 runs from APB2 bus (rate 96) MHz -> need to divide by 32,
    resulting in 3 MHz clock frequency. Probably acceptable.
  */
  spi_set_baudrate_prescaler(SPI1, SPI_CR1_BR_FPCLK_DIV_16);
  spi_set_standard_mode(SPI1, 3);
  
  spi_set_full_duplex_mode(SPI1);
  spi_set_dff_8bit(SPI1);
  spi_send_msb_first(SPI1);

  spi_enable_software_slave_management(SPI1);
  spi_set_nss_high(SPI1);

  spi_set_master_mode(SPI1);

  spi_enable(SPI1);
    
  /* Pull cable select UP as no comms will yet happen */    
  gpio_set(GPIOB, GPIO6);
  
}

static void l6474_message(uint8_t *msg_tx, uint8_t *msg_rx, uint8_t msg_len)
{
  uint8_t i;
  
  /* begin transmission */
  gpio_clear(GPIOB, GPIO6);
  
  if (msg_rx == NULL) {
    for(i = 0; i < msg_len; i++) {
      (void)spi_xfer(SPI1, msg_tx[i]);
    }
  } else {
    for(i = 0; i < msg_len; i++) {
      msg_rx[i] = spi_xfer(SPI1, msg_tx[i]);
    }
  }
  
  /* end transmission */
  gpio_set(GPIOB, GPIO6);

}



int main(void)
{
  rcc_setup();
  nvic_setup();
  gpio_setup();
  usart_setup();
  spi_setup();
  tim2_setup();
  tim3_setup();  

  /* ------------------- TOP - BOT -- */
  uint8_t message1[] = {0xB8, 0xB8};

  uint8_t reply[3];
  
  gpio_set(GPIOA, GPIO9);
  
  for (uint32_t i = 0; i < 100000; i++){
    asm("nop");
  }
  
  l6474_message(message1, reply, 2);
  
  gpio_clear(GPIOA, GPIO8);
  gpio_set(GPIOB, GPIO5);


  m1_target_period = 3120;
  step_count = 1;
  timer_enable_counter(TIM2); // TOP
  timer_enable_counter(TIM3); // BOT

  
  while(1)
  {
    __WFI();
  }
}

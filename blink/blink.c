#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

static void gpio_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5|GPIO1);
}

int main(void)
{
	int i;

	gpio_setup();
	
	/* Turn on the LED */
	GPIOA_BSRR = GPIO5;
	GPIOA_BSRR = GPIO1;
	
	while (1) {
		for (i = 0; i < 1000000; i++) {	/* Wait a bit. */
			__asm__("nop");
		}
		GPIOA_ODR ^= GPIO5|GPIO1;	/* LED on/off */
	}

	return 0;
}

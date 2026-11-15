#include <stdint.h>

#ifdef STM32F407xx
	#include "hw/stm32f407xx.h"
#elif defined(STM32F030x6)
	#include "hw/stm32f030x6.h"
#endif

#include "gpio.h"

#define GPIO_FAULT asm volatile (".short 0xDE00") /* undefined trap */ 

#ifdef STM32F407xx
	static GPIO_TypeDef * const gpio_base = GPIOA;
	static uint32_t       const gpio_step = ((uint8_t *)GPIOB)-((uint8_t *)GPIOA);

	const uint8_t static gpio_clken_bitpos[] =
	{
		RCC_AHB1ENR_GPIOAEN_Pos, RCC_AHB1ENR_GPIOBEN_Pos,
		RCC_AHB1ENR_GPIOCEN_Pos, RCC_AHB1ENR_GPIODEN_Pos,
		RCC_AHB1ENR_GPIOEEN_Pos, RCC_AHB1ENR_GPIOFEN_Pos,
		RCC_AHB1ENR_GPIOGEN_Pos, RCC_AHB1ENR_GPIOHEN_Pos,
		RCC_AHB1ENR_GPIOIEN_Pos
	};

	static volatile uint32_t * const clken_reg = &RCC->AHB1ENR;

#elif defined(STM32F030x6)

	static GPIO_TypeDef * const gpio_base = GPIOA;
	static uint32_t       const gpio_step = ((uint8_t *)GPIOB)-((uint8_t *)GPIOA);

	const uint8_t static gpio_clken_bitpos[] =
	{
		RCC_AHBENR_GPIOAEN_Pos, RCC_AHBENR_GPIOBEN_Pos,
		RCC_AHBENR_GPIOCEN_Pos, RCC_AHBENR_GPIODEN_Pos,
		33                    , RCC_AHBENR_GPIOFEN_Pos, // no GPIOE
	};
	
	static volatile uint32_t * const clken_reg = &RCC->AHBENR;
#else
	#error MCU type macro not defined!
#endif


static GPIO_TypeDef * get_gpio_addr(struct gpio_pin pin)
{
	if( pin.num >= (sizeof(gpio_clken_bitpos)/sizeof(gpio_clken_bitpos[0])) ) GPIO_FAULT;

	if( gpio_clken_bitpos[pin.num] > 32 ) GPIO_FAULT;

	return ((GPIO_TypeDef *)(((uint8_t *)gpio_base) + pin.num * gpio_step));
}


void gpio_cfg(const struct gpio_cfg * gpio_config, uint32_t initial)
{
	const struct gpio_pin * curr;

	uint32_t gpio_num;
	uint32_t bit_off;
	uint32_t bit_off2;
	uint32_t bit_off4;
	uint32_t word_off4;

	GPIO_TypeDef * gpio;

	uint32_t curr_bit;

	// set up GPIO bits
	for(int i=0; i < gpio_config->mode.count; i++)
	{
		// get GPIO block address
		gpio = get_gpio_addr( gpio_config->pins[i] );

		// turn on clocking for the GPIO
		(*clken_reg) |= (1UL<<gpio_clken_bitpos[gpio_num]);

		// get bit and offset into GPIO config regs
		bit_off = gpio_config->pins[i].pin;

		bit_off2  = bit_off*2;

		bit_off4 = bit_off*4;
		word_off4 = 0;
		if( bit_off4>=32 )
		{
			bit_off4 -= 32;
			word_off4++;
		}

		// set up given GPIO bit
		// first turn it off (analog mode)
		gpio->MODER |= GPIO_MODE_ANA << bit_off2;
		// preset output register
		gpio->BSRR = (initial & (1UL<<(gpio_config->mode.count-1-i))) ? (1UL<<bit_off) : (0x10000UL<<bit_off);
		// cfg PUPD/SPEED/TYPE/AF
		gpio->OTYPER = (gpio->OTYPER & (~(1UL<<bit_off))) | (gpio_config->mode.otyp<<bit_off);
		gpio->OSPEEDR = (gpio->OSPEEDR & (~(3UL<<bit_off2))) | (gpio_config->mode.ospd<<bit_off2);
		gpio->PUPDR = (gpio->PUPDR & (~(3UL<<bit_off2))) | (gpio_config->mode.pupd<<bit_off2);
		gpio->AFR[word_off4] &= ~(15UL<<bit_off4);
		gpio->AFR[word_off4] |= (gpio_config->mode.afnum<<bit_off4);
		// finally configure MODE back
		gpio->MODER = (gpio->MODER & (~(3UL<<bit_off2))) | (gpio_config->mode.mode<<bit_off2);
	}
}


#if 0
// bits from gpio_bit array are put into the result, other bits in the result are 0
uint32_t gpio_read (const struct gpio_bit * gpio_array)
{
	uint32_t result = 0;;

	const struct gpio_bit * curr = gpio_array;

	do
	{
		GPIO_TypeDef * gpio = get_gpio_addr( (curr->gpio_num) & GPIO_MASK, curr->gpio_pin );

		uint32_t bit = ((gpio->IDR) >> (curr->gpio_pin)) & 1;

		result = (result<<1) | bit;

	} while( !(((curr++)->gpio_num) & GPIO_LAST_BIT) );

	return result;
}
// opposite to gpio_read(), bits numbered in gpio_array are taken from value and put to the GPIO bits
void     gpio_write(const struct gpio_bit * gpio_array, uint32_t value)
{
	const struct gpio_bit * curr = gpio_array;

	uint32_t curr_bit = get_gpio_bits(gpio_array)-1;

	do
	{
		GPIO_TypeDef * gpio = get_gpio_addr( (curr->gpio_num) & GPIO_MASK, curr->gpio_pin );

		gpio->BSRR = (value & (1UL<<(curr_bit--))) ? (1UL<<curr->gpio_pin) : (0x00010000UL<<curr->gpio_pin);

	} while( !(((curr++)->gpio_num) & GPIO_LAST_BIT) );
}

// return 0 or 1 if the requested bit is inside any res_bit of gpio_bit array
uint32_t gpio_read_bit (const struct gpio_bit * gpio_array, uint8_t bit_num)
{
	uint32_t num_bits = get_gpio_bits(gpio_array);

	if( bit_num>=num_bits ) GPIO_FAULT;

	GPIO_TypeDef * gpio = get_gpio_addr( (gpio_array[num_bits-bit_num-1].gpio_num) & GPIO_MASK, gpio_array[num_bits-bit_num-1].gpio_pin );

	
	return 1 & ((gpio->IDR) >> (gpio_array[num_bits-bit_num-1].gpio_pin));
}
// value bit 0 is taken
void     gpio_write_bit(const struct gpio_bit * gpio_array, uint8_t bit_num, uint32_t value)
{
	uint32_t num_bits = get_gpio_bits(gpio_array);
	if( bit_num>=num_bits ) GPIO_FAULT;

	uint32_t gpio_num = (gpio_array[num_bits-bit_num-1].gpio_num) & GPIO_MASK;
	uint32_t gpio_pin =  gpio_array[num_bits-bit_num-1].gpio_pin;

	GPIO_TypeDef * gpio = get_gpio_addr( gpio_num, gpio_pin );

	gpio->BSRR = (value & (1UL<<bit_num)) ? (1UL<<gpio_pin) : (0x00010000UL<<gpio_pin);
}
#endif

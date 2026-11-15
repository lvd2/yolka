#ifndef _GPIO_H_
#define _GPIO_H_

#define GPIO_A (0)
#define GPIO_B (1)
#define GPIO_C (2)
#define GPIO_D (3)
#define GPIO_E (4)
#define GPIO_F (5)
#define GPIO_G (6)
#define GPIO_H (7)
#define GPIO_I (8)

#define GPIO_MODE_IN  0
#define GPIO_MODE_OUT 1
#define GPIO_MODE_ALT 2
#define GPIO_MODE_ANA 3

#define GPIO_OTYP_PP  0
#define GPIO_OTYP_OD  1

#define GPIO_OSPD_LO  0
#define GPIO_OSPD_M0  1
#define GPIO_OSPD_M1  2
#define GPIO_OSPD_HI  3

#define GPIO_PUPD_NO  0
#define GPIO_PUPD_PU  1
#define GPIO_PUPD_PD  2



struct gpio_pin
{
	uint8_t num:4; // number of GPIO controller, 0 for GPIOA, 1 for GPIOB, etc.
	uint8_t pin:4; // pin (bit number) in the GPIO controller, 0..15
	// both 0xF means end of list
};

struct gpio_mode
{
	uint16_t mode:2;
	uint16_t otyp:1;
	uint16_t ospd:2;
	uint16_t pupd:2;
	uint16_t afnum:4;
	uint16_t count:5;
};

struct gpio_cfg
{
	struct gpio_mode mode;
	struct gpio_pin pins [];
};

// gpio_array must end with LAST_BIT=1, mode is made of oring GPIO_* flags above
void gpio_cfg(const struct gpio_cfg * gpio_config, uint32_t initial);

// bits from gpio_bit array are put into the result, other bits in the result are 0
//uint32_t gpio_read (const struct gpio_bit * gpio_array);
// opposite to gpio_read(), bits numbered in gpio_array are taken from value and put to the GPIO bits
//void     gpio_write(const struct gpio_bit * gpio_array, uint32_t value);

// return 0 or 1 if the requested bit is inside any res_bit of gpio_bit array
//uint32_t gpio_read_bit (const struct gpio_bit * gpio_array, uint8_t bit_num);
// value bit 0 is taken
//void     gpio_write_bit(const struct gpio_bit * gpio_array, uint8_t bit_num, uint32_t value);


#endif // _GPIO_H_


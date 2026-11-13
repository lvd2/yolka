#include <stdint.h>
#include "../test_ws2812/hw/stm32f0xx.h"

#include "bit_exec.h"

// PB6 -- set to 1 (fet gate)
// PB1 -- gen 3.2MHz clock (TIM14)
// PA5 -- input of 3.2 MHz for SPI1
// PA6 -- MISO from SPI1 (to ws2818)

//#define USE_XTAL 1
#define DO_DMA 1
#define DO_4BITS 1

#define NUM_LED (64)
#define MAX_VAL (16)
#define DELAY   (1)

volatile uint32_t phase = 0;
volatile uint32_t regen_leds = 0;

volatile uint8_t rgb[3*NUM_LED];
volatile uint8_t map_color[1024];

volatile uint32_t   send_ctr;
volatile uint16_t   send_arr[ 3 + ((3*4*NUM_LED)+1)/2 ];
volatile uint16_t * send_ptr;

#ifdef DO_4BITS
	const uint32_t mapmul = 4;
#else
	const uint32_t mapmul = 3;
#endif

void init_leds(void);

void init_dma_chan3(void);

uint32_t rnd(void);

BEGIN_IO_SCRIPT( clkinit )
	// enable clocks: GPIOA, GPIOB, DMA
	{OP_BIS, &RCC->AHBENR, RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_DMAEN},

	// reset GPIOA, GPIOB
	{OP_WRITE, &RCC->AHBRSTR, RCC_AHBRSTR_GPIOARST | RCC_AHBRSTR_GPIOBRST},
	{OP_WRITE, &RCC->AHBRSTR, 0},

	// enable clocks: TIM14, SPI1
	{OP_BIS, &RCC->APB1ENR, RCC_APB1ENR_TIM14EN},
	{OP_BIS, &RCC->APB2ENR, RCC_APB2ENR_SPI1EN},

	// reset: TIM14, SPI1
	{OP_WRITE, &RCC->APB1RSTR, RCC_APB1RSTR_TIM14RST},
	{OP_WRITE, &RCC->APB2RSTR, RCC_APB2RSTR_SPI1RST},
	{OP_WRITE, &RCC->APB1RSTR, 0},
	{OP_WRITE, &RCC->APB2RSTR, 0},

	// turn on prefetch and configure 1 flash wait state for 48MHz clock
	{OP_BICS, &FLASH->ACR, FLASH_ACR_LATENCY_Msk, FLASH_ACR_PRFTBE | (1UL<<FLASH_ACR_LATENCY_Pos)},

	// configure PLL for 48MHz and switch to it
	// switch HSI on
	{OP_BIS, &RCC->CR, RCC_CR_HSION},
	// wait for completion
	//while( !(RCC->CR & RCC_CR_HSIRDY) );
	{OP_WAITMASK, &RCC->CR, RCC_CR_HSIRDY, RCC_CR_HSIRDY},

	// switch to HSI clock, AHB clk /1, APB clk /1, no output clock
	{OP_WRITE, &RCC->CFGR, RCC_CFGR_SW_HSI | RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE_DIV1},
	// wait for switch to finish
	//while( (RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI );
	{OP_WAITMASK, &RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_HSI},

	// turn off PLL
	{OP_BIC, &RCC->CR, RCC_CR_PLLON},
	
	// turn off HSE bypass, CSS, on or off HSE
	{OP_BIC, &RCC->CR, (RCC_CR_HSEON | RCC_CR_HSEBYP | RCC_CR_CSSON)},
#ifdef USE_XTAL
	{OP_BIS, &RCC->CR, RCC_CR_HSEON}.
	//
	// wait for HSE
	//while( !(RCC->CR & RCC_CR_HSERDY) );
	{OP_WAITMASK, &RCC->CR, RCC_CR_HSERDY, RCC_CR_HSERDY},
#endif

	// wait for PLL to unlock
	//while( RCC->CR & RCC_CR_PLLRDY );
	{OP_WAITMASK, &RCC->CR, RCC_CR_PLLRDY, 0},

	// configure PLL to get 48MHz clock:
	//
	{OP_WRITE, &RCC->CFGR2, RCC_CFGR2_PREDIV_DIV2}, // divide HSE by 2; PLL input always 4MHz (either HSI/2 or HSE/2)
	{OP_BICS, &RCC->CFGR, (RCC_CFGR_PLLMUL_Msk | RCC_CFGR_PLLSRC_Msk), RCC_CFGR_PLLMUL12
#ifdef USE_XTAL
	            | RCC_CFGR_PLLSRC_HSE_PREDIV},
#else
	            | RCC_CFGR_PLLSRC_HSI_DIV2},
#endif

	// turn on PLL
	{OP_BIS, &RCC->CR, RCC_CR_PLLON},
	// wait for it turning on
	//while( !(RCC->CR & RCC_CR_PLLRDY) );
	{OP_WAITMASK, &RCC->CR, RCC_CR_PLLRDY, RCC_CR_PLLRDY},

	// switch to PLL clock
	{OP_BICS, &RCC->CFGR, RCC_CFGR_SW_Msk, RCC_CFGR_SW_PLL},
	// wait for switch
	//while( (RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL );
	{OP_WAITMASK, &RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_PLL},
	{OP_STOP}
END_IO_SCRIPT

BEGIN_IO_SCRIPT(gpio)
	
	// PB6 as output emitting 1
	{OP_BICS, &GPIOB->MODER, GPIO_MODER_MODER6_Msk, 1UL<<GPIO_MODER_MODER6_Pos}, // general purpose output
	//
	{OP_BIC, &GPIOB->OTYPER, GPIO_OTYPER_OT_6}, // push-pull
	//
	{OP_BICS, &GPIOB->OSPEEDR, GPIO_OSPEEDR_OSPEEDR6_Msk, (2UL<<GPIO_OSPEEDR_OSPEEDR6_Pos)}, // speed 2
	//
	{OP_WRITE, &GPIOB->BSRR, GPIO_BSRR_BS_6}, // output 1

	// PA5 -- SPI1 SCK (input)
	// PA6 -- SPI1 MISO (output)
	{OP_BICS, &GPIOA->MODER, (GPIO_MODER_MODER5 | GPIO_MODER_MODER6), 
	                (2UL << GPIO_MODER_MODER5_Pos) |
	                (2UL << GPIO_MODER_MODER6_Pos) },
	//
	{OP_BIC, &GPIOA->OTYPER, GPIO_OTYPER_OT_6}, // push-pull (PA6)
	//
	{OP_BICS, &GPIOA->OSPEEDR, (GPIO_OSPEEDR_OSPEEDR5_Msk | GPIO_OSPEEDR_OSPEEDR6_Msk),   // 3rd speed grade
	                           (2UL << GPIO_OSPEEDR_OSPEEDR6_Pos) },                      //
	//
	{OP_BICS, &GPIOA->AFR[0], (GPIO_AFRL_AFRL5_Msk | GPIO_AFRL_AFRL6_Msk), (0UL << GPIO_AFRL_AFRL5_Pos) |  (0UL << GPIO_AFRL_AFRL6_Pos) }, // PA5,6 <- AF0



	// configure PB1 as output, controlled by TIM14
	{OP_BICS, &GPIOB->MODER, GPIO_MODER_MODER1_Msk, (2UL << GPIO_MODER_MODER1_Pos) }, // PB1 alternate function
	//
	{OP_BIC, &GPIOB->OTYPER, GPIO_OTYPER_OT_1 },
	//
	{OP_BICS, &GPIOB->OSPEEDR, GPIO_OSPEEDR_OSPEEDR1_Msk, (2UL << GPIO_OSPEEDR_OSPEEDR1_Pos) },
	//
	{OP_BICS, &GPIOB->AFR[0], GPIO_AFRL_AFRL1_Msk, (0UL << GPIO_AFRL_AFRL1_Pos) }, // PB1 <- AF0
	{OP_STOP}

END_IO_SCRIPT



void main(void)
{

	bit_exec(clkinit);
#if 0
	// enable clocks: GPIOA, GPIOB, DMA
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_DMAEN;

	// reset GPIOA, GPIOB
	RCC->AHBRSTR = RCC_AHBRSTR_GPIOARST | RCC_AHBRSTR_GPIOBRST;
	RCC->AHBRSTR = 0;

	// enable clocks: TIM14, SPI1
	RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

	// reset: TIM14, SPI1
	RCC->APB1RSTR = RCC_APB1RSTR_TIM14RST;
	RCC->APB2RSTR = RCC_APB2RSTR_SPI1RST;
	RCC->APB1RSTR = 0;
	RCC->APB2RSTR = 0;


	
	// turn on prefetch and configure 1 flash wait state for 48MHz clock
	FLASH->ACR = (FLASH->ACR & (~(FLASH_ACR_LATENCY_Msk))) | FLASH_ACR_PRFTBE | (1UL<<FLASH_ACR_LATENCY_Pos);


	// configure PLL for 48MHz and switch to it
	//
	// switch HSI on
	RCC->CR |= RCC_CR_HSION;
	// wait for completion
	while( !(RCC->CR & RCC_CR_HSIRDY) );

	// switch to HSI clock, AHB clk /1, APB clk /1, no output clock
	RCC->CFGR = RCC_CFGR_SW_HSI | RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE_DIV1;
	// wait for switch to finish
	while( (RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI );

	// turn off PLL
	RCC->CR &= ~(RCC_CR_PLLON);
	
	// turn off HSE bypass, CSS, on or off HSE
	RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_HSEBYP | RCC_CR_CSSON);
#ifdef USE_XTAL
	RCC->CR |= RCC_CR_HSEON;
	//
	// wait for HSE
	while( !(RCC->CR & RCC_CR_HSERDY) );
#endif

	// wait for PLL to unlock
	while( RCC->CR & RCC_CR_PLLRDY );

	// configure PLL to get 48MHz clock:
	//
	RCC->CFGR2 = RCC_CFGR2_PREDIV_DIV2; // divide HSE by 2; PLL input always 4MHz (either HSI/2 or HSE/2)
	RCC->CFGR = (RCC->CFGR & (~(RCC_CFGR_PLLMUL_Msk | RCC_CFGR_PLLSRC_Msk))) | RCC_CFGR_PLLMUL12
#ifdef USE_XTAL
	            | RCC_CFGR_PLLSRC_HSE_PREDIV;
#else
	            | RCC_CFGR_PLLSRC_HSI_DIV2;
#endif

	// turn on PLL
	RCC->CR |= RCC_CR_PLLON;
	// wait for it turning on
	while( !(RCC->CR & RCC_CR_PLLRDY) );

	// switch to PLL clock
	RCC->CFGR = (RCC->CFGR & (~(RCC_CFGR_SW_Msk))) | RCC_CFGR_SW_PLL;
	// wait for switch
	while( (RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL );
#endif
	

	bit_exec(gpio);
#if 0
	// PB6 as output emitting 1
	GPIOB->MODER &= ~GPIO_MODER_MODER6_Msk;
	GPIOB->MODER |= 1UL<<GPIO_MODER_MODER6_Pos; // general purpose output
	//
	GPIOB->OTYPER &= ~GPIO_OTYPER_OT_6; // push-pull
	//
	GPIOB->OSPEEDR = (GPIOB->OSPEEDR & (~GPIO_OSPEEDR_OSPEEDR6_Msk))
	                 | (2UL<<GPIO_OSPEEDR_OSPEEDR6_Pos); // speed 2
	//
	GPIOB->BSRR = GPIO_BSRR_BS_6; // output 1

	// PA5 -- SPI1 SCK (input)
	// PA6 -- SPI1 MISO (output)
	GPIOA->MODER &= ~ (GPIO_MODER_MODER5 | GPIO_MODER_MODER6);
	GPIOA->MODER |= (2UL << GPIO_MODER_MODER5_Pos) |
	                (2UL << GPIO_MODER_MODER6_Pos) ;
	//
	GPIOA->OTYPER &= ~GPIO_OTYPER_OT_6; // push-pull (PA6)
	//
	GPIOA->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEEDR5_Msk | GPIO_OSPEEDR_OSPEEDR6_Msk);   // 3rd speed grade
	GPIOA->OSPEEDR |= (2UL << GPIO_OSPEEDR_OSPEEDR6_Pos);                         //
	//
	GPIOA->AFR[0] = (GPIOA->AFR[0] & (~(GPIO_AFRL_AFRL5_Msk | GPIO_AFRL_AFRL6_Msk))) | (0UL << GPIO_AFRL_AFRL5_Pos) |  (0UL << GPIO_AFRL_AFRL6_Pos); // PA5,6 <- AF0



	// configure PB1 as output, controlled by TIM14
	GPIOB->MODER = (GPIOB->MODER & (~GPIO_MODER_MODER1_Msk)) | (2UL << GPIO_MODER_MODER1_Pos); // PB1 alternate function
	//
	GPIOB->OTYPER &= ~( GPIO_OTYPER_OT_1 );
	//
	GPIOB->OSPEEDR = (GPIOB->OSPEEDR & (~GPIO_OSPEEDR_OSPEEDR1_Msk)) | (2UL << GPIO_OSPEEDR_OSPEEDR1_Pos);
	//
	GPIOB->AFR[0] = (GPIOB->AFR[0] & (~GPIO_AFRL_AFRL1_Msk)) | (0UL << GPIO_AFRL_AFRL1_Pos); // PB1 <- AF0
#endif


	// configure TIMER14 (running at APB @ 48 MHz):
	//  timer reload period must be 3.2MHz, there must be single compare event per period, compare should flip output value
	TIM14->PSC  =  1 - 1;
#ifdef DO_4BITS
	TIM14->ARR  = 15 - 1;
#else
	TIM14->ARR  = 20 - 1;
#endif
	TIM14->CCR1 = TIM14->ARR>>1;
	//
	TIM14->EGR = TIM_EGR_CC1G; // generate compare event
	TIM14->CCMR1 = (0x7UL << TIM_CCMR1_OC1M_Pos) | // PWM
	               (0x0UL << TIM_CCMR1_CC1S_Pos) ; // CC1 is output
	TIM14->CCER =  TIM_CCER_CC1E; // OC1 active
	//
	TIM14->CR1 = TIM_CR1_CEN; // start timer


	// init rgb array
	init_leds();

#ifdef DO_DMA
	// configure DMA to work with SPI1_TX: channel 3
	//
	// stop channel 3
	DMA1_Channel3->CCR &= ~(DMA_CCR_EN);
	while( DMA1_Channel3->CCR & DMA_CCR_EN );
	// general DMA cfg
	DMA1_Channel3->CCR = (3UL << DMA_CCR_PL_Pos)    | // top priority
	                     (1UL << DMA_CCR_MSIZE_Pos) | // 16bit
	                     (1UL << DMA_CCR_PSIZE_Pos) | //      transfers
	                     DMA_CCR_MINC               | // memory increment
	                     DMA_CCR_DIR                | // read from mem
	                     DMA_CCR_TCIE               ; // transfer complete indication enable
	// clear IRQ flags for this stream
	DMA1->IFCR = DMA_IFCR_CTCIF3 | DMA_IFCR_CHTIF3 | DMA_IFCR_CTEIF3 | DMA_IFCR_CGIF3;
	//
	init_dma_chan3();

#endif

	// configure SPI1 in slave mode, internal NSS
	SPI1->CR1 = SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_CPHA | SPI_CR1_LSBFIRST; // | 16bit SPI_CR1_DFF;
	SPI1->CR2 = (15UL<<SPI_CR2_DS_Pos); // 16 bits

	SPI1->CR1 |= SPI_CR1_SPE;
	SPI1->CR1 &= ~SPI_CR1_SSI;

	// send initial zeros (and let it repeat until real send). Send so many zeros as to fill whole FIFO with zeros
	for(int i=0;i<3;i++)
	{
		while( !(SPI1->SR & SPI_SR_TXE) );
		SPI1->DR = 0;
	}

#ifdef DO_DMA
	// enable TX DMA interop
	SPI1->CR2 |= SPI_CR2_TXDMAEN;
#else
	// initialize and enable TXE interrupt
	SPI1->CR2 = SPI_CR2_TXEIE;
	// NVIC cfg
	NVIC_SetPriority(SPI1_IRQn,0);
	NVIC_EnableIRQ(SPI1_IRQn);
#endif

	// init systick for 100 Hz interrupts
	SysTick->LOAD = 480000/8-1;
	SysTick->VAL  = 0;
	SysTick->CTRL = /*SysTick_CTRL_CLKSOURCE_Msk |*/ SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
	
	// enable systick interrupt
	NVIC_SetPriority(SysTick_IRQn, 8);



	int dma_ran = 0;

	int ctr=0;

	__SEV();
	__WFE();
	while(1)
	{
		__WFE();

		if( regen_leds )
		{

			regen_leds = 0;


			ctr+=11;
			if(ctr>=256*3*NUM_LED) ctr=0;

			int i;

			// update colors
			for( i=0 ; i < NUM_LED*3 ; i++ )
			{
			#if 0
				uint32_t r = rnd();
				int16_t y = rgb[i];

				//if( r<(0x100000000ULL/3ULL) )
				if( r<(0x100000000ULL/2ULL) )
					y++;
				else //if( r<(0x200000000ULL/3ULL) )
					y--;

				if     ( y<0 ) y=0;
				else if( y>MAX_VAL ) y=MAX_VAL;

				rgb[i] = y;
			#else
				rgb[i]=1;
				rgb[ctr>>8]=ctr&0xFF;
			#endif
			}

			// generate values
			uint8_t * cols = (uint8_t *)rgb;
			uint8_t * vals = ((uint8_t *) send_arr)+2;

			for(i=0;i<NUM_LED*3;i++)
			{
				uint32_t c = (*(cols++)) * mapmul;
				*(vals++) = map_color[ c     ];
				*(vals++) = map_color[ c + 1 ];
				*(vals++) = map_color[ c + 2 ];
#ifdef DO_4BITS
				*(vals++) = map_color[ c + 3 ];
#endif
			}

		#ifdef DO_DMA
			// prepare DMA send
			if( dma_ran )
				while( ! (DMA1->ISR & DMA_ISR_TCIF3) ); // wait for transfer complete
			DMA1_Channel3->CCR &= ~(DMA_CCR_EN); // disable DMA channel
			DMA1->IFCR = DMA_IFCR_CTCIF3 | DMA_IFCR_CHTIF3 | DMA_IFCR_CTEIF3; // clear interrupt flags
			init_dma_chan3(); // init ptrs/cntrs
			DMA1_Channel3->CCR |= DMA_CCR_EN; // rerun DMA
			dma_ran = 1;
		#else
			send_ptr = send_arr;
			send_ctr = sizeof(send_arr)/sizeof(send_arr[0]);
		#endif

			
		}
	}
}


__attribute__ ((used)) void handler_SysTick(void)
{
	phase++;

	if( GPIOA->IDR & GPIO_IDR_0 ) // check PA0
	{
		GPIOA->BSRR = GPIO_BSRR_BS_4; // set PA4
	}
	else
	{
		GPIOA->BSRR = GPIO_BSRR_BR_4; // clr PA4
	}



	if( phase>=DELAY )
	{
		__SEV();
		phase = 0;
		regen_leds = 1;
	}
}


#ifndef DO_DMA
__attribute__ ((used)) void handler_SPI1(void)
{
	if( send_ctr )
	{
		SPI1->DR = *send_ptr++;
		send_ctr--;
	}
	else
		SPI1->DR = 0;
}
#endif



void init_leds(void)
{
	uint8_t * p = (uint8_t *)map_color;
	int i;

#ifdef DO_4BITS
	for(i=0;i<256;i++)
	{
		// 00110001 etc.
		// 0001 for low
		// 0011 for high
	
		*(p++) = 0x11 | ( (i&128) ? 0x02 : 0x00 ) | ( (i& 64) ? 0x20 : 0x00 );
		*(p++) = 0x11 | ( (i& 32) ? 0x02 : 0x00 ) | ( (i& 16) ? 0x20 : 0x00 );
		*(p++) = 0x11 | ( (i&  8) ? 0x02 : 0x00 ) | ( (i&  4) ? 0x20 : 0x00 );
		*(p++) = 0x11 | ( (i&  2) ? 0x02 : 0x00 ) | ( (i&  1) ? 0x20 : 0x00 );
	}
#else
	for(i=0;i<256;i++)
	{
		uint8_t a,b,c;
		// 01001001 10010010 00100100
		a = 0x49;
		b = 0x92;
		c = 0x24;

		if( i & 128 ) a |= 0x02;
		if( i &  64 ) a |= 0x10;
		if( i &  32 ) a |= 0x80;
		if( i &  16 ) b |= 0x40;
		if( i &   8 ) b |= 0x20;
		if( i &   4 ) c |= 0x01;
		if( i &   2 ) c |= 0x08;
		if( i &   1 ) c |= 0x40;

		*(p++) = a;
		*(p++) = b;
		*(p++) = c;
	}
#endif

	for( i=0 ; i < (sizeof(send_arr)/sizeof(send_arr[0])) ; i++ )
	{
		send_arr[i]=0;
	}
	send_ctr = 0;
	send_ptr = 0;

	for(i=0;i<NUM_LED*3;i++)
	{
		rgb[i] = 0;
	}
}

uint32_t rnd(void)
{
	static uint32_t st = 1;

	uint32_t x = st;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	st = x;
	return x;
}


void init_dma_chan3(void)
{
	// length (always the same)
	DMA1_Channel3->CNDTR = sizeof(send_arr)/sizeof(send_arr[0]);
	// periph addr
	DMA1_Channel3->CPAR = (uint32_t)(&(SPI1->DR));
	// memory addr
	DMA1_Channel3->CMAR = (uint32_t)(send_arr);
}


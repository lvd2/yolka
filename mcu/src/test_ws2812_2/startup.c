#include <stdint.h>
#include "hw/stm32f0xx.h"

// this functions sets up bss and data and runs main()
void startup(void) __attribute__ ((noinline,noreturn,naked));

// main() function
extern void main(void) __attribute__ ((noreturn,naked));


// linker definitions
extern uint32_t __vectors__;

extern uint32_t __stk_canary__;
extern uint32_t __stk_top__;

extern uint32_t __bss_start__;
extern uint32_t __bss_end__  ;

extern uint32_t __etext__;

extern uint32_t __data_start__;
extern uint32_t __data_end__  ;




// vectors

typedef void( *ex_handler )( void );

void handler_Reset(void) __attribute__ ((used,naked,noreturn));

void handler_stub (void) __attribute__ ((used));

// weak aliases will be automatically changed to handler_stub if no user function defined
void handler_NMI                 (void) __attribute__ ((weak, alias("handler_stub")));
void handler_HardFault           (void) __attribute__ ((weak, alias("handler_stub")));
void handler_SVCall              (void) __attribute__ ((weak, alias("handler_stub")));
void handler_PendSV              (void) __attribute__ ((weak, alias("handler_stub")));
void handler_SysTick             (void) __attribute__ ((weak, alias("handler_stub")));
void handler_WWDG                (void) __attribute__ ((weak, alias("handler_stub")));
void handler_RTC                 (void) __attribute__ ((weak, alias("handler_stub")));
void handler_FLASH               (void) __attribute__ ((weak, alias("handler_stub")));
void handler_RCC                 (void) __attribute__ ((weak, alias("handler_stub")));
void handler_EXTI0_1             (void) __attribute__ ((weak, alias("handler_stub")));
void handler_EXTI2_3             (void) __attribute__ ((weak, alias("handler_stub")));
void handler_EXTI4_15            (void) __attribute__ ((weak, alias("handler_stub")));
void handler_DMA1_Channel1       (void) __attribute__ ((weak, alias("handler_stub")));
void handler_DMA1_Channel2_3     (void) __attribute__ ((weak, alias("handler_stub")));
void handler_DMA1_Channel4_5     (void) __attribute__ ((weak, alias("handler_stub")));
void handler_ADC                 (void) __attribute__ ((weak, alias("handler_stub")));
void handler_TIM1_BRK_UP_TRG_COM (void) __attribute__ ((weak, alias("handler_stub")));
void handler_TIM1_CC             (void) __attribute__ ((weak, alias("handler_stub")));
void handler_TIM3                (void) __attribute__ ((weak, alias("handler_stub")));
void handler_TIM6                (void) __attribute__ ((weak, alias("handler_stub")));
void handler_TIM14               (void) __attribute__ ((weak, alias("handler_stub")));
void handler_TIM15               (void) __attribute__ ((weak, alias("handler_stub")));
void handler_TIM16               (void) __attribute__ ((weak, alias("handler_stub")));
void handler_TIM17               (void) __attribute__ ((weak, alias("handler_stub")));
void handler_I2C1                (void) __attribute__ ((weak, alias("handler_stub")));
void handler_I2C2                (void) __attribute__ ((weak, alias("handler_stub")));
void handler_SPI1                (void) __attribute__ ((weak, alias("handler_stub")));
void handler_SPI2                (void) __attribute__ ((weak, alias("handler_stub")));
void handler_USART1              (void) __attribute__ ((weak, alias("handler_stub")));
void handler_USART2              (void) __attribute__ ((weak, alias("handler_stub")));
void handler_USART3_4_5_6        (void) __attribute__ ((weak, alias("handler_stub")));
void handler_USB                 (void) __attribute__ ((weak, alias("handler_stub")));



const ex_handler __Vectors[] __attribute__ ((section(".vectors"),used)) = {
	/* Cortex-M4 Exception Vectors */
	(ex_handler)&__stk_top__     ,  // 0x000 initial MSP
	& handler_Reset              ,  // 0x004
	& handler_NMI                ,  // 0x008
	& handler_HardFault          ,  // 0x00C
	& handler_stub               ,  // 0x010 --reserved--
	& handler_stub               ,  // 0x014 --reserved--
	& handler_stub               ,  // 0x018 --reserved--
	& handler_stub               ,  // 0x01C --reserved--
	& handler_stub               ,  // 0x020 --reserved--
	& handler_stub               ,  // 0x024 --reserved--
	& handler_stub               ,  // 0x028 --reserved--
	& handler_SVCall             ,  // 0x02C
	& handler_stub               ,  // 0x030 --reserved--
	& handler_stub               ,  // 0x034 --reserved--
	& handler_PendSV             ,  // 0x038
	& handler_SysTick            ,  // 0x03C
	& handler_WWDG               ,  // 0x040
	& handler_stub               ,  // 0x044 --reserved--
	& handler_RTC                ,  // 0x048
	& handler_FLASH              ,  // 0x04C
	& handler_RCC                ,  // 0x050
	& handler_EXTI0_1            ,  // 0x054
	& handler_EXTI2_3            ,  // 0x058
	& handler_EXTI4_15           ,  // 0x05C
	& handler_stub               ,  // 0x060 --reserved--
	& handler_DMA1_Channel1      ,  // 0x064
	& handler_DMA1_Channel2_3    ,  // 0x068
	& handler_DMA1_Channel4_5    ,  // 0x06C
	& handler_ADC                ,  // 0x070
	& handler_TIM1_BRK_UP_TRG_COM,  // 0x074
	& handler_TIM1_CC            ,  // 0x078
	& handler_stub               ,  // 0x07C --reserved--
	& handler_TIM3               ,  // 0x080
	& handler_TIM6               ,  // 0x084
	& handler_stub               ,  // 0x088 --reserved--
	& handler_TIM14              ,  // 0x08C
	& handler_TIM15              ,  // 0x090
	& handler_TIM16              ,  // 0x094
	& handler_TIM17              ,  // 0x098
	& handler_I2C1               ,  // 0x09C
	& handler_I2C2               ,  // 0x0A0
	& handler_SPI1               ,  // 0x0A4
	& handler_SPI2               ,  // 0x0A8
	& handler_USART1             ,  // 0x0AC
	& handler_USART2             ,  // 0x0B0
	& handler_USART3_4_5_6       ,  // 0x0B4
	& handler_stub               ,  // 0x0B8 --reserved--
	& handler_USB                ,  // 0x0BC
};



void handler_stub(void)
{
	while(1);
}

void handler_Reset(void)
{
	__set_MSP((uint32_t)&__stk_top__);
//	SCB->VTOR = (uint32_t)&__vectors__; // no VTOR in cortex m0

	// set stack canary
	__stk_canary__ = 0xDEADBEEF;

	startup();
}

void startup(void)
{
	uint32_t * sptr;
	uint32_t * dptr;

	// clear BSS
	dptr = & __bss_start__;

	while( dptr < (& __bss_end__) )
		*(dptr++) = 0UL;

	// copy DATA
	dptr = & __data_start__;
	sptr = & __etext__;

	if( sptr!=dptr )
	{
		while( dptr < (& __data_end__) )
			*(dptr++) = *(sptr++);
	}
	
	main();
}


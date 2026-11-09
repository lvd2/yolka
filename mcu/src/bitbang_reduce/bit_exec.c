#include <stdint.h>

#include "bit_exec.h"


uint32_t bit_exec(uint8_t * script)
{
	uint32_t rd_result = 0xDEADBEEF;
	uint8_t opcode;
	uint8_t mmmm;

	uint32_t args[2];


	uint32_t * addr;


	for(;;)
	{
		opcode = *(script++);
		
		mmmm = opcode & OP_MMASK;

		if( opcode & OP_AMASK ); // fetch address
		{
			uint32_t mkaddr;
			uint16_t value;

			value =  *(script++);
			value = (*(script++))<<8;

			mkaddr = value & 0xFFFC;

			value &= 3;
			if( value==3 ) value = 0x0800;
			value |= 0x4000;
			mkaddr |= value<<16;

			addr = (uint32_t *)mkaddr;
		}

		opcode >>= OP_SHIFT;

		// fetch arg(s)
		int i=0;
		while( i < ((opcode+2)>>2) ) // 0,1 - no args; 2,3,4,5 -- 1 arg; 6,7 -- 2 args
		{
		}

		switch( opcode )
		{
		}
	}
}


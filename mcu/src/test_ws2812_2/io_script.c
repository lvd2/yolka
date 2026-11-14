#include <stdint.h>

#include "io_script.h"


uint32_t io_script_exec(const uint8_t * script)
{
	uint32_t rd_result;
	uint8_t opcode;
	uint8_t mmmm;
	uint32_t value;
	uint32_t shift;

	uint32_t args[2];

	volatile uint32_t * addr;


	for(;;)
	{
		opcode = *(script++);
		mmmm = opcode & OP_MMASK;

		if( opcode & OP_AMASK ) // fetch address
		{
			uint32_t mkaddr;

			value  =  *(script++);
			value |= (*(script++))<<8;

			mkaddr = value & 0xFFFC;

			value &= 3;
			if( value==3 ) value = 0x0800;
			value |= 0x4000;
			mkaddr |= value<<16;

			addr = (uint32_t *)mkaddr;
		}

		opcode >>= OP_SHIFT;

		// fetch arg(s)
		for(int i=0; i<((opcode+2)>>2); i++) // 0,1 - no args; 2,3,4,5 -- 1 arg; 6,7 -- 2 args
		{
			value = *(script++); // 1st byte of argument is always fetched

			if( !(mmmm & 0x08) ) // mmmm is 00?? or 01XX -- fetch next 8 bits
			{
				value |= (*(script++))<<8;
			}
			if( !(mmmm & 0x0C) ) // mmmm is 00?? -- fetch highest 16 bits
			{
				value |= (*(script++))<<16;
				value |= (*(script++))<<24;
			}

			if( mmmm & 0x08 ) // 1YYY -- byte
			{
				shift = ((mmmm & 7)<<2) | (value>>6);
				value = (value & 0x3F) << shift;
			}
			else if( mmmm & 0x04 ) // 01XX -- word
			{
				shift = ((mmmm & 3)<<3) | (value>>13);
				value = (value & 0x1FFF) << shift;
			}

			args[i] = value;
		}

		// execute operation
		switch( opcode )
		{
		default: // case OP_STOP
			return rd_result;
		//////////////////////////////////////////////////////////////
		case OP_READ:
			rd_result = *addr;
			break;
		//////////////////////////////////////////////////////////////
		case OP_WRITE:
			*addr = args[0];
			break;
		//////////////////////////////////////////////////////////////
		case OP_BIC:
			*addr &= ~args[0];
			break;
		//////////////////////////////////////////////////////////////
		case OP_BIS:
			*addr |= args[0];
			break;
		//////////////////////////////////////////////////////////////
		case OP_READMASK:
			rd_result = (*addr) & args[0];
			break;
		//////////////////////////////////////////////////////////////
		case OP_BICS:
			*addr = ( (*addr) & (~args[0]) ) | args[1];
			break;
		//////////////////////////////////////////////////////////////
		case OP_WAITMASK:
			while( ((*addr) & args[0]) != args[1] );
			break;
		}
	}
}


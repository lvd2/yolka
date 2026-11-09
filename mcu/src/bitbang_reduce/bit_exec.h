#ifndef _BIT_EXEC_H_
#define _BIT_EXEC_H_

#define OP_STOP     (0)
#define OP_READ     (1)
#define OP_WRITE    (2)
#define OP_BIC      (3)
#define OP_BIS      (4)
#define OP_READMASK (5)
#define OP_BICS     (6)
#define OP_WAITMASK (7)

#define OP_SHIFT (5)

#define OP_AMASK (0x10)

#define OP_MMASK (0x0F)


uint32_t bit_exec(uint8_t *);

#endif // _BIT_EXEC_H_

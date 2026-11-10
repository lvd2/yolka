#ifndef _BIT_EXEC_H_
#define _BIT_EXEC_H_


// also to be used for IO_SCRIPTs
#define OP_STOP     (0)
#define OP_READ     (1)
#define OP_WRITE    (2)
#define OP_BIC      (3)
#define OP_BIS      (4)
#define OP_READMASK (5)
#define OP_BICS     (6)
#define OP_WAITMASK (7)

// following constants are only for bit_exec()
#define OP_SHIFT (5)
#define OP_AMASK (0x10)
#define OP_MMASK (0x0F)




#define TOSTR(name) #name

#define BEGIN_IO_SCRIPT(name)  \
 extern const uint8_t name []; \
 const uint32_t __attribute__ ((section( TOSTR( __io_scripts__ ## name ) ))) __io_script__ ## name [][4] = {

#define END_IO_SCRIPT };




uint32_t bit_exec(uint8_t *);

#endif // _BIT_EXEC_H_

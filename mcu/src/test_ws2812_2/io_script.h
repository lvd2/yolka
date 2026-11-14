#ifndef _IO_SCRIPT_H_
#define _IO_SCRIPT_H_


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
 __attribute__ ((section( TOSTR( __io_scripts__ ## name ) ))) const struct {uint32_t a;volatile uint32_t * b;uint32_t c;uint32_t d;} __io_script__ ## name [] =




uint32_t io_script_exec(const uint8_t *);

#endif // _IO_SCRIPT_H_

#ifndef BOT_H
#define BOT_H

#define BOT_VERSION_MAJOR 1
#define BOT_VERSION_MINOR 2
#define BOT_VERSION_PATCH 0

#define BOT_OP_INT 0x01 // INTERRUPT
#define BOT_OP_NUM 0x10 // INTEGER
#define BOT_OP_JMP 0x20 // JUMP
#define BOT_OP_JMC 0x21 // CONDITIONAL JUMP
#define BOT_OP_CEQ 0x22 // INTEGER COMPARE IF EQUAL
#define BOT_OP_CNE 0x23 // INTEGER COMPARE IF NOT EQUAL
#define BOT_OP_CLS 0x24 // INTEGER COMPARE IF LESS THAN
#define BOT_OP_CLE 0x25 // INTEGER COMPARE IF LESS OR EQUAL
#define BOT_OP_HOP 0x30 // SET STACK COUNTER
#define BOT_OP_POS 0x31 // GET STACK COUNTER
#define BOT_OP_SET 0x32 // SET ADDRESS VALUE
#define BOT_OP_GET 0x33 // GET ADDRESS VALUE
#define BOT_OP_DUP 0x34 // DUPLICATE STACK VALUE
#define BOT_OP_POP 0x35 // POP STACK VALUE
#define BOT_OP_ROT 0x36 // ROTATE VALUES
#define BOT_OP_ADD 0x40 // INTEGER ADD
#define BOT_OP_SUB 0x41 // INTEGER SUBTRACT
#define BOT_OP_MUL 0x42 // INTEGER MULTIPLY
#define BOT_OP_DIV 0x43 // INTEGER DIVIDE
#define BOT_OP_POW 0x44 // INTEGER POWER
#define BOT_OP_MOD 0x45 // INTEGER MODULO
#define BOT_OP_NOT 0x50 // BITWISE NOT
#define BOT_OP_AND 0x51 // BITWISE AND
#define BOT_OP_BOR 0x52 // BITWISE OR
#define BOT_OP_XOR 0x53 // BITWISE XOR
#define BOT_OP_LSH 0x54 // BITWISE LEFT SHIFT
#define BOT_OP_RSH 0x55 // BITWISE RIGHT SHIFT
#define BOT_OP_FPU 0x60 // FLOAT
#define BOT_OP_FTU 0x70 // FLOAT TO INTEGER
#define BOT_OP_UTF 0x71 // FLOAT FROM INTEGER
#define BOT_OP_FEQ 0x72 // FLOAT COMPARE IF EQUAL
#define BOT_OP_FNE 0x73 // FLOAT COMPARE IF NOT EQUAL
#define BOT_OP_FLS 0x74 // FLOAT COMPARE IF LESS THAN
#define BOT_OP_FLE 0x75 // FLOAT COMPARE IF LESS OR EQUAL
#define BOT_OP_FAD 0x80 // FLOAT ADD
#define BOT_OP_FSU 0x81 // FLOAT SUBTRACT
#define BOT_OP_FMU 0x82 // FLOAT MULTIPLY
#define BOT_OP_FDI 0x83 // FLOAT DIVIDE
#define BOT_OP_FPO 0x84 // FLOAT POWER
#define BOT_OP_FMO 0x85 // FLOAT MODULO

#define BOT_INT_NONE              0x00
#define BOT_INT_END_OF_PROGRAM    0x01
#define BOT_INT_INVALID_OPERATION 0x02
#define BOT_INT_INVALID_JUMP      0x03
#define BOT_INT_OUT_OF_BOUNDS     0x04
#define BOT_INT_OUT_OF_MEMORY     0x05

typedef unsigned char bot_char;
typedef unsigned int  bot_uint;
typedef float         bot_real;

typedef union {
	bot_uint uint;
	bot_real real;
} bot_word;

typedef struct {
	bot_uint  INT;
	bot_uint  PC;
	bot_uint  SP;
	bot_uint  rom_size;
	bot_uint  mem_size;
	bot_char *rom;
	bot_word *mem;
} bot_vm;

typedef struct {
	void      *user;
	bot_char (*read)(void *user);
	bot_uint (*size)(void *user);
} bot_rom;

bot_vm* bot_vm_new();

void bot_vm_free(
	bot_vm *vm
);

void bot_vm_load(
	bot_vm *vm,
	bot_rom rom
);

bot_uint bot_vm_resize(
	bot_vm  *vm,
	bot_uint size
);

void bot_vm_interrupt(
	bot_vm  *vm,
	bot_uint interrupt
);

void bot_vm_hop(
	bot_vm  *vm,
	bot_uint address
);

void bot_vm_jump(
	bot_vm  *vm,
	bot_uint address
);

void bot_vm_set_uint(
	bot_vm  *vm,
	bot_uint address,
	bot_uint value
);

void bot_vm_set_real(
	bot_vm  *vm,
	bot_uint address,
	bot_real value
);

void bot_vm_set_string(
	bot_vm  *vm,
	bot_uint address,
	bot_uint length,
	char    *buffer
);

bot_uint bot_vm_get_uint(
	bot_vm  *vm,
	bot_uint address
);

bot_real bot_vm_get_real(
	bot_vm  *vm,
	bot_uint address
);

void bot_vm_get_string(
	bot_vm  *vm,
	bot_uint address,
	bot_uint length,
	char    *buffer
);

void bot_vm_push_uint(
	bot_vm  *vm,
	bot_uint value
);

void bot_vm_push_real(
	bot_vm  *vm,
	bot_real value
);

void bot_vm_push_string(
	bot_vm  *vm,
	bot_uint length,
	char    *buffer
);

bot_uint bot_vm_pop_uint(
	bot_vm *vm
);

bot_real bot_vm_pop_real(
	bot_vm *vm
);

void bot_vm_pop_string(
	bot_vm  *vm,
	bot_uint length,
	char    *buffer
);

void bot_vm_run(
	bot_vm *vm
);

#endif

#ifdef BOT_IMPLEMENTATION

#include <math.h>

#ifndef BOT_MALLOC
	#include <malloc.h>
	#define BOT_MALLOC  malloc
	#define BOT_REALLOC realloc
	#define BOT_FREE    free
#endif

#ifndef BOT_MIN_MEM_SIZE
	#define BOT_MIN_MEM_SIZE 1024
#endif

#ifndef BOT_MAX_MEM_SIZE
	#define BOT_MAX_MEM_SIZE 536870912
#endif

bot_vm* bot_vm_new() {
	bot_vm *vm = BOT_MALLOC(sizeof(bot_vm));

	if (vm==NULL) return NULL;

	vm->INT      = 0;
	vm->PC       = 0;
	vm->SP       = 0;
	vm->rom_size = 0;
	vm->mem_size = BOT_MIN_MEM_SIZE;
	vm->rom      = NULL;
	vm->mem      = BOT_MALLOC(
		sizeof(bot_word)*
		BOT_MIN_MEM_SIZE
	);

	return vm;
}

void bot_vm_free(
	bot_vm *vm
) {
	if (vm==NULL) return;

	if (vm->rom!=NULL) BOT_FREE(vm->rom);
	if (vm->mem!=NULL) BOT_FREE(vm->mem);

	BOT_FREE(vm);
}

void bot_vm_load(
	bot_vm *vm,
	bot_rom rom
) {
	if (vm->rom!=NULL) BOT_FREE(vm->rom);

	vm->rom_size = rom.size(rom.user);
	vm->rom      = BOT_MALLOC(vm->rom_size*sizeof(bot_char));

	if (vm->rom==NULL) return;

	for (bot_uint i=0; i<vm->rom_size; i++) {
		vm->rom[i] = rom.read(rom.user);
	}
}

bot_uint bot_vm_resize(
	bot_vm  *vm,
	bot_uint size
) {
	if (size>BOT_MAX_MEM_SIZE) {
		bot_vm_interrupt(
			vm,
			BOT_INT_OUT_OF_MEMORY
		);

		return 0;
	}

	bot_uint mem_size = BOT_MIN_MEM_SIZE;

	while (mem_size<size) {
		mem_size = mem_size+(mem_size/2);
	}

	bot_word *mem = BOT_REALLOC(
		vm->mem,
		mem_size*sizeof(bot_word)
	);

	if (mem==NULL) {
		bot_vm_interrupt(
			vm,
			BOT_INT_OUT_OF_MEMORY
		);

		return 0;
	}

	vm->mem      = mem;
	vm->mem_size = mem_size;

	return mem_size;
}

void bot_vm_interrupt(
	bot_vm  *vm,
	bot_uint interrupt
) {
	if (vm->INT && interrupt) return;

	vm->INT = interrupt;
}

void bot_vm_hop(
	bot_vm  *vm,
	bot_uint address
) {
	if (
		address>=vm->mem_size &&
		!bot_vm_resize(vm,address+1)
	) {
		bot_vm_interrupt(
			vm,
			BOT_INT_OUT_OF_MEMORY
		);

		return;
	}

	vm->SP = address;
}

void bot_vm_jump(
	bot_vm  *vm,
	bot_uint address
) {
	if (address>vm->rom_size) {
		bot_vm_interrupt(
			vm,
			BOT_INT_INVALID_JUMP
		);

		return;
	};

	vm->PC = address;
}

void bot_vm_set_uint(
	bot_vm  *vm,
	bot_uint address,
	bot_uint value
) {
	if (address>=vm->SP) {
		bot_vm_interrupt(
			vm,
			BOT_INT_OUT_OF_BOUNDS
		);

		return;
	}

	vm->mem[address].uint = value;
}

void bot_vm_set_real(
	bot_vm  *vm,
	bot_uint address,
	bot_real value
) {
	if (address>=vm->SP) {
		bot_vm_interrupt(
			vm,
			BOT_INT_OUT_OF_BOUNDS
		);

		return;
	}

	vm->mem[address].real = value;
}

void bot_vm_set_string(
	bot_vm  *vm,
	bot_uint address,
	bot_uint length,
	char    *buffer
) {
	bot_uint word    = 0;
	bot_uint padding = 0;

	if (length%sizeof(bot_uint)!=0) padding = (
		sizeof(bot_uint)-
		length%sizeof(bot_uint)
	);

	for (bot_uint i=0; i<length; i++) {
		word = (word<<8)|buffer[i];

		if (
			(i+padding+1)%sizeof(bot_uint)==0 ||
			i==length-1
		) {
			bot_vm_set_uint(vm,address++,word);
			word = 0;
		}
	}
}

bot_uint bot_vm_get_uint(
	bot_vm  *vm,
	bot_uint address
) {
	if (address>=vm->SP)  {
		bot_vm_interrupt(
			vm,
			BOT_INT_OUT_OF_BOUNDS
		);

		return 0;
	}

	return vm->mem[address].uint;
}

bot_real bot_vm_get_real(
	bot_vm  *vm,
	bot_uint address
) {
	if (address>=vm->SP)  {
		bot_vm_interrupt(
			vm,
			BOT_INT_OUT_OF_BOUNDS
		);

		return 0;
	}

	return vm->mem[address].real;
}

void bot_vm_get_string(
	bot_vm  *vm,
	bot_uint address,
	bot_uint length,
	char    *buffer
) {
	bot_uint word = 0;
	address      += (length-1)/sizeof(bot_uint);

	for (bot_uint i=0; i<length; i++) {
		if (i%sizeof(bot_uint)==0) {
			word = bot_vm_get_uint(vm,address--);
		}

		buffer[length-i-1] = word&0xFF;
		word               = word>>8;
	}
}

void bot_vm_push_uint(
	bot_vm  *vm,
	bot_uint value
) {
	bot_vm_hop(vm,vm->SP+1);
	bot_vm_set_uint(vm,vm->SP-1,value);
}

void bot_vm_push_real(
	bot_vm  *vm,
	bot_real value
) {
	bot_vm_hop(vm,vm->SP+1);
	bot_vm_set_real(vm,vm->SP-1,value);
}

void bot_vm_push_string(
	bot_vm  *vm,
	bot_uint length,
	char    *buffer
) {
	bot_uint address = vm->SP;
	bot_uint words   = (
		(length+sizeof(bot_uint)-1)/
		sizeof(bot_uint)
	);

	bot_vm_hop(vm,address+words);
	bot_vm_set_string(
		vm,
		address,
		length,
		buffer
	);
}

bot_uint bot_vm_pop_uint(
	bot_vm *vm
) {
	bot_uint value=bot_vm_get_uint(vm,vm->SP-1);
	bot_vm_hop(vm,vm->SP-1);

	return value;
}

bot_real bot_vm_pop_real(
	bot_vm *vm
) {
	bot_real value=bot_vm_get_real(vm,vm->SP-1);
	bot_vm_hop(vm,vm->SP-1);

	return value;
}

void bot_vm_pop_string(
	bot_vm  *vm,
	bot_uint length,
	char    *buffer
) {
	bot_uint words   = (
		(length+sizeof(bot_uint)-1)/
		sizeof(bot_uint)
	);
	bot_uint address = vm->SP-words;

	bot_vm_get_string(
		vm,
		address,
		length,
		buffer
	);
	bot_vm_hop(vm,address);
}

#define INT_SET(I) INT = I;

#define REG_SET(R,L) R = L;

#define ROM_JMP(I) { \
if (I>rom_size) { \
	INT = BOT_INT_INVALID_JUMP; break; \
} else { \
	PC = I; \
}}

#define ROM_JMC(I,V) { \
if (I>rom_size) { \
	INT = BOT_INT_INVALID_JUMP; break; \
} else if (V) { \
	PC = I; \
}}

#define ROM_POP(V) { \
if (PC>=rom_size) { \
	INT = BOT_INT_END_OF_PROGRAM; break; \
} else { \
	V = rom[PC++]; \
}}

#define ROM_POP2(V) { \
if (PC+1>=rom_size) { \
	INT = BOT_INT_END_OF_PROGRAM; break; \
} else { \
	V = ( \
		(rom[PC]<<8) | \
		(rom[PC+1]) \
	); \
	PC += 2; \
}}

#define ROM_POP3(V) { \
if (PC+2>=rom_size) { \
	INT = BOT_INT_END_OF_PROGRAM; break; \
} else { \
	V = ( \
		(rom[PC]   << 16) | \
		(rom[PC+1] << 8)  | \
		(rom[PC+2]) \
	); \
	PC += 3; \
}}

#define ROM_POP4(V) { \
if (PC+3>=rom_size) { \
	INT = BOT_INT_END_OF_PROGRAM; break; \
} else { \
	V = ( \
		(rom[PC]   << 24) | \
		(rom[PC+1] << 16) | \
		(rom[PC+2] << 8)  | \
		(rom[PC+3]) \
	); \
	PC += 4; \
}}

#define MEM_PUT(V) { \
if ( \
	SP>=mem_size && \
	!bot_vm_resize(vm,SP+1) \
) { \
	INT = BOT_INT_OUT_OF_MEMORY; break; \
} else { \
	mem       = vm->mem; \
	mem_size  = vm->mem_size; \
	mem[SP++] = V; \
}}

#define MEM_POP(V) { \
if (SP==0) { \
	INT = BOT_INT_OUT_OF_BOUNDS; break; \
} else { \
	V = mem[--SP]; \
}}

#define MEM_HOP(I) { \
if ( \
	I>=mem_size && \
	!bot_vm_resize(vm,I+1) \
) { \
	INT = BOT_INT_OUT_OF_MEMORY; break; \
} else { \
	SP       = I; \
	mem      = vm->mem; \
	mem_size = vm->mem_size; \
}}

#define MEM_SET(I,V) { \
if (I>=SP) { \
	INT = BOT_INT_OUT_OF_BOUNDS; break; \
} else { \
	mem[I]=V; \
}}

#define MEM_GET(I,V) { \
if (I>=SP) { \
	INT = BOT_INT_OUT_OF_BOUNDS; break; \
} else { \
	V = mem[I]; \
}}

void bot_vm_run(
	bot_vm *vm
) {
	bot_uint mem_size = vm->mem_size;
	bot_uint rom_size = vm->rom_size;

	bot_word *mem = vm->mem;
	bot_char *rom = vm->rom;

	bot_uint INT = vm->INT;
	bot_uint PC  = vm->PC;
	bot_uint SP  = vm->SP;

	while (INT==BOT_INT_NONE) {
		bot_word A,B,C;

		ROM_POP(A.uint);

		switch(A.uint) {
			case BOT_OP_INT:
				MEM_POP(A);
				INT_SET(A.uint);

				break;
			case BOT_OP_NUM:
				REG_SET(A.uint,0);
				MEM_PUT(A);

				break;
			case BOT_OP_NUM+1:
				ROM_POP(A.uint);
				MEM_PUT(A);

				break;
			case BOT_OP_NUM+2:
				ROM_POP2(A.uint);
				MEM_PUT(A);

				break;
			case BOT_OP_NUM+3:
				ROM_POP3(A.uint);
				MEM_PUT(A);

				break;
			case BOT_OP_NUM+4:
				ROM_POP4(A.uint);
				MEM_PUT(A);

				break;
			case BOT_OP_JMP:
				MEM_POP(A);
				ROM_JMP(A.uint);

				break;
			case BOT_OP_JMC:
				MEM_POP(B);
				MEM_POP(A);
				ROM_JMC(B.uint,A.uint);

				break;
			case BOT_OP_CEQ:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.uint==B.uint);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_CNE:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.uint!=B.uint);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_CLS:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.uint<B.uint);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_CLE:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.uint<=B.uint);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_HOP:
				MEM_POP(A);
				MEM_HOP(A.uint);

				break;
			case BOT_OP_POS:
				REG_SET(A.uint,SP);
				MEM_PUT(A);

				break;
			case BOT_OP_SET:
				MEM_POP(B);
				MEM_POP(A);
				MEM_SET(B.uint,A);

				break;
			case BOT_OP_GET:
				MEM_GET(SP-1,A);
				MEM_GET(A.uint,B);
				MEM_SET(SP-1,B);

				break;
			case BOT_OP_DUP:
				MEM_GET(SP-1,A);
				MEM_PUT(A);

				break;
			case BOT_OP_POP:
				MEM_POP(A);

				break;
			case BOT_OP_ROT:
				MEM_GET(SP-2,A);
				MEM_GET(SP-1,B);
				MEM_SET(SP-2,B);
				MEM_SET(SP-1,A);

				break;
			case BOT_OP_ADD:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.uint+B.uint);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_SUB:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.uint-B.uint);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_MUL:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.uint*B.uint);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_DIV:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.uint/B.uint);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_POW:
				MEM_POP(B);
				MEM_GET(SP-1,A);

				for (
					C.uint=1;
					B.uint>0;
					B.uint--
				) {
					REG_SET(C.uint,C.uint*A.uint);
				}

				MEM_SET(SP-1,C);

				break;
			case BOT_OP_MOD:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.uint%B.uint);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_NOT:
				MEM_GET(SP-1,A);
				REG_SET(A.uint,~A.uint);
				MEM_SET(SP-1,A);

				break;
			case BOT_OP_AND:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.uint&B.uint);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_BOR:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.uint|B.uint);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_XOR:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.uint^B.uint);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_LSH:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.uint<<B.uint);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_RSH:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.uint>>B.uint);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_FPU:
				REG_SET(A.real,0);
				MEM_PUT(A);

				break;
			case BOT_OP_FPU+4:
				ROM_POP4(A.uint);
				REG_SET(A.real,(bot_real)A.uint/65536-32768);
				MEM_PUT(A);

				break;
			case BOT_OP_FTU:
				MEM_GET(SP-1,A);
				REG_SET(A.uint,(bot_uint)A.real);
				MEM_SET(SP-1,A);

				break;
			case BOT_OP_UTF:
				MEM_GET(SP-1,A);
				REG_SET(A.real,(bot_real)A.uint);
				MEM_SET(SP-1,A);

				break;
			case BOT_OP_FEQ:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.real==B.real);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_FNE:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.real!=B.real);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_FLS:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.real<B.real);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_FLE:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.uint,A.real<=B.real);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_FAD:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.real,A.real+B.real);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_FSU:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.real,A.real-B.real);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_FMU:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.real,A.real*B.real);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_FDI:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.real,A.real/B.real);
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_FPO:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.real,powf(A.real,B.real));
				MEM_SET(SP-1,C);

				break;
			case BOT_OP_FMO:
				MEM_POP(B);
				MEM_GET(SP-1,A);
				REG_SET(C.real,fmod(A.real,B.real));
				MEM_SET(SP-1,C);

				break;
			default:
				INT_SET(BOT_INT_INVALID_OPERATION);
		}
	}

	vm->INT = INT;
	vm->PC  = PC;
	vm->SP  = SP;
}

#undef INT_SET
#undef REG_SET
#undef ROM_JMP
#undef ROM_JMC
#undef ROM_POP
#undef ROM_POP2
#undef ROM_POP3
#undef ROM_POP4
#undef MEM_PUT
#undef MEM_POP
#undef MEM_HOP
#undef MEM_SET
#undef MEM_GET

#endif

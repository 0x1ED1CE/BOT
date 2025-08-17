/*
MIT License

Copyright (c) 2025 Dice

Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef BOT_H
#define BOT_H

#define BOT_VERSION_MAJOR 1
#define BOT_VERSION_MINOR 3
#define BOT_VERSION_PATCH 2

#define BOT_OP_NOP 0x00 // NO OPERATION
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
#define BOT_OP_POP 0x34 // POP STACK VALUE
#define BOT_OP_ROT 0x35 // ROTATE VALUES
#define BOT_OP_ADD 0x40 // INTEGER ADD
#define BOT_OP_SUB 0x41 // INTEGER SUBTRACT
#define BOT_OP_MUL 0x42 // INTEGER MULTIPLY
#define BOT_OP_DIV 0x43 // INTEGER DIVIDE
#define BOT_OP_MOD 0x44 // INTEGER MODULO
#define BOT_OP_MIN 0x45 // INTEGER MIN
#define BOT_OP_NOT 0x50 // BITWISE NOT
#define BOT_OP_AND 0x51 // BITWISE AND
#define BOT_OP_BOR 0x52 // BITWISE OR
#define BOT_OP_XOR 0x53 // BITWISE XOR
#define BOT_OP_LSH 0x54 // BITWISE LEFT SHIFT
#define BOT_OP_RSH 0x55 // BITWISE RIGHT SHIFT

#define BOT_INT_NONE              0x00
#define BOT_INT_END_OF_PROGRAM    0x01
#define BOT_INT_INVALID_OPERATION 0x02
#define BOT_INT_INVALID_JUMP      0x03
#define BOT_INT_OUT_OF_BOUNDS     0x04
#define BOT_INT_OUT_OF_MEMORY     0x05

typedef unsigned char      bot_char;
typedef unsigned int       bot_uint;
typedef unsigned long long bot_long;

typedef struct {
	bot_uint  INT;
	bot_uint  PC;
	bot_uint  SP;
	bot_uint  mem_size;
	bot_uint  rom_size;
	bot_uint *mem;
	bot_char *rom;
} bot_vm;

typedef struct {
	void      *user;
	bot_uint (*size)(void *user);
	bot_char (*read)(void *user);
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

void bot_vm_set_uint_array(
	bot_vm  *vm,
	bot_uint address,
	bot_uint length,
	bot_uint values[]
);

void bot_vm_set_string(
	bot_vm   *vm,
	bot_uint  address,
	bot_uint  length,
	bot_char *buffer
);

bot_uint bot_vm_get_uint(
	bot_vm  *vm,
	bot_uint address
);

void bot_vm_get_uint_array(
	bot_vm  *vm,
	bot_uint address,
	bot_uint length,
	bot_uint values[]
);

void bot_vm_get_string(
	bot_vm   *vm,
	bot_uint  address,
	bot_uint  length,
	bot_char *buffer
);

void bot_vm_push_uint(
	bot_vm  *vm,
	bot_uint value
);

void bot_vm_push_string(
	bot_vm   *vm,
	bot_uint  length,
	bot_char *buffer
);

bot_uint bot_vm_pop_uint(
	bot_vm *vm
);

void bot_vm_pop_string(
	bot_vm   *vm,
	bot_uint  length,
	bot_char *buffer
);

void bot_vm_run(
	bot_vm *vm
);

#endif

#ifdef BOT_IMPLEMENTATION

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
	vm->mem_size = BOT_MIN_MEM_SIZE;
	vm->rom_size = 0;
	vm->mem      = BOT_MALLOC(
		sizeof(bot_uint)*
		BOT_MIN_MEM_SIZE
	);
	vm->rom      = NULL;

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

	bot_uint *mem = BOT_REALLOC(
		vm->mem,
		mem_size*sizeof(bot_uint)
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

	vm->mem[address] = value;
}

void bot_vm_set_uint_array(
	bot_vm  *vm,
	bot_uint address,
	bot_uint length,
	bot_uint values[]
) {
	if (address+length>vm->SP) {
		bot_vm_interrupt(
			vm,
			BOT_INT_OUT_OF_BOUNDS
		);

		return;
	}

	for (bot_uint i=0; i<length; i++) {
		vm->mem[address+i]=values[i];
	}
}

void bot_vm_set_string(
	bot_vm   *vm,
	bot_uint  address,
	bot_uint  length,
	bot_char *buffer
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

	return vm->mem[address];
}

void bot_vm_get_uint_array(
	bot_vm  *vm,
	bot_uint address,
	bot_uint length,
	bot_uint values[]
) {
	if (address+length>vm->SP) {
		bot_vm_interrupt(
			vm,
			BOT_INT_OUT_OF_BOUNDS
		);

		return;
	}

	for (bot_uint i=0; i<length; i++) {
		values[i]=vm->mem[address+i];
	}
}

void bot_vm_get_string(
	bot_vm   *vm,
	bot_uint  address,
	bot_uint  length,
	bot_char *buffer
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

void bot_vm_push_string(
	bot_vm   *vm,
	bot_uint  length,
	bot_char *buffer
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
	if (vm->SP==0) {
		bot_vm_interrupt(
			vm,
			BOT_INT_OUT_OF_BOUNDS
		);

		return 0;
	}

	return vm->mem[--vm->SP];
}

void bot_vm_pop_string(
	bot_vm   *vm,
	bot_uint  length,
	bot_char *buffer
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

#define VSET(R,L) R = L;

#define RJMP(I) { \
if (I>rom_size) { \
	INT = BOT_INT_INVALID_JUMP; break; \
} else { \
	PC = I; \
}}

#define RJMC(I,V) { \
if (I>rom_size) { \
	INT = BOT_INT_INVALID_JUMP; break; \
} else if (V) { \
	PC = I; \
}}

#define RPOP(V) { \
if (PC>=rom_size) { \
	INT = BOT_INT_END_OF_PROGRAM; break; \
} else { \
	V = rom[PC++]; \
}}

#define RPO2(V) { \
if (PC+1>=rom_size) { \
	INT = BOT_INT_END_OF_PROGRAM; break; \
} else { \
	V = ( \
		(rom[PC]<<8) | \
		(rom[PC+1]) \
	); \
	PC += 2; \
}}

#define RPO3(V) { \
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

#define RPO4(V) { \
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

#define MPUT(V) { \
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

#define MPOP(V) { \
if (SP==0) { \
	INT = BOT_INT_OUT_OF_BOUNDS; break; \
} else { \
	V = mem[--SP]; \
}}

#define MHOP(I) { \
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

#define MSET(I,V) { \
if (I>=SP) { \
	INT = BOT_INT_OUT_OF_BOUNDS; break; \
} else { \
	mem[I]=V; \
}}

#define MSE2(I,V) { \
if (I+1>=SP) { \
	INT = BOT_INT_OUT_OF_BOUNDS; break; \
} else { \
	mem[I]   = V>>32; \
	mem[I+1] = V&0xFFFFFFFF; \
}}

#define MGET(I,V) { \
if (I>=SP) { \
	INT = BOT_INT_OUT_OF_BOUNDS; break; \
} else { \
	V = mem[I]; \
}}

#define TEST(S,E) { \
if (S) { \
	E \
}}

void bot_vm_run(
	bot_vm *vm
) {
	bot_uint mem_size = vm->mem_size;
	bot_uint rom_size = vm->rom_size;

	bot_uint *mem = vm->mem;
	bot_char *rom = vm->rom;

	bot_uint INT = vm->INT;
	bot_uint PC  = vm->PC;
	bot_uint SP  = vm->SP;

	bot_uint A,B,C;
	bot_long E,F,G;

	while (INT==BOT_INT_NONE) {
		RPOP(A);

		switch(A) {
			case BOT_OP_NOP:                                                             break;
			case BOT_OP_INT:   MPOP(A);      VSET(INT,A);                                break;
			case BOT_OP_NUM:   VSET(A,0);    MPUT(A);                                    break;
			case BOT_OP_NUM+1: RPOP(A);      MPUT(A);                                    break;
			case BOT_OP_NUM+2: RPO2(A);      MPUT(A);                                    break;
			case BOT_OP_NUM+3: RPO3(A);      MPUT(A);                                    break;
			case BOT_OP_NUM+4: RPO4(A);      MPUT(A);                                    break;
			case BOT_OP_JMP:   MPOP(A);      RJMP(A);                                    break;
			case BOT_OP_JMC:   MPOP(B);      MPOP(A);       RJMC(B,A);                   break;
			case BOT_OP_CEQ:   MPOP(B);      MGET(SP-1,A);  VSET(C,A==B);  MSET(SP-1,C); break;
			case BOT_OP_CNE:   MPOP(B);      MGET(SP-1,A);  VSET(C,A!=B);  MSET(SP-1,C); break;
			case BOT_OP_CLS:   MPOP(B);      MGET(SP-1,A);  VSET(C,A<B);   MSET(SP-1,C); break;
			case BOT_OP_CLE:   MPOP(B);      MGET(SP-1,A);  VSET(C,A<=B);  MSET(SP-1,C); break;
			case BOT_OP_HOP:   MPOP(A);      MHOP(A);                                    break;
			case BOT_OP_POS:   VSET(A,SP);   MPUT(A);                                    break;
			case BOT_OP_SET:   MPOP(B);      MPOP(A);       MSET(B,A);                   break;
			case BOT_OP_GET:   MGET(SP-1,A); MGET(A,B);     MSET(SP-1,B);                break;
			case BOT_OP_POP:   MPOP(A);                                                  break;
			case BOT_OP_ROT:   MGET(SP-2,A); MGET(SP-1,B);  MSET(SP-2,B);  MSET(SP-1,A); break;
			case BOT_OP_ADD:   MPOP(B);      MGET(SP-1,A);  VSET(C,A+B);   MSET(SP-1,C); break;
			case BOT_OP_SUB:   MPOP(B);      MGET(SP-1,A);  VSET(C,A-B);   MSET(SP-1,C); break;
			case BOT_OP_MUL:   MGET(SP-1,F); MGET(SP-2,E);  VSET(G,E*F);   MSE2(SP-2,G); break;
			case BOT_OP_DIV:   MPOP(B);      MGET(SP-1,A);  VSET(C,A/B);   MSET(SP-1,C); break;
			case BOT_OP_MOD:   MPOP(B);      MGET(SP-1,A);  VSET(C,A%B);   MSET(SP-1,C); break;
			case BOT_OP_MIN:   MGET(SP-2,A); MGET(SP-1,B);
			                   TEST(B<A,     MSET(SP-2,B);  MSET(SP-1,A);)               break;
			case BOT_OP_NOT:   MGET(SP-1,A); VSET(A,~A);    MSET(SP-1,A);                break;
			case BOT_OP_AND:   MPOP(B);      MGET(SP-1,A);  VSET(C,A&B);   MSET(SP-1,C); break;
			case BOT_OP_BOR:   MPOP(B);      MGET(SP-1,A);  VSET(C,A|B);   MSET(SP-1,C); break;
			case BOT_OP_XOR:   MPOP(B);      MGET(SP-1,A);  VSET(C,A^B);   MSET(SP-1,C); break;
			case BOT_OP_LSH:   MPOP(B);      MGET(SP-1,A);  VSET(C,A<<B);  MSET(SP-1,C); break;
			case BOT_OP_RSH:   MPOP(B);      MGET(SP-1,A);  VSET(C,A>>B);  MSET(SP-1,C); break;
			default:           VSET(INT,BOT_INT_INVALID_OPERATION);
		}
	}

	vm->INT = INT;
	vm->PC  = PC;
	vm->SP  = SP;
}

#undef VSET
#undef RJMP
#undef RJMC
#undef RPOP
#undef RPO2
#undef RPO3
#undef RPO4
#undef MPUT
#undef MPOP
#undef MHOP
#undef MSET
#undef MSE2
#undef MGET
#undef TEST

#endif

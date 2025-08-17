#ifndef BOX_H
#define BOX_H

#define BOT_INT_ARGUMENT      0x0A
#define BOT_INT_CONSOLE_PRINT 0x0B
#define BOT_INT_CONSOLE_INPUT 0x0C
#define BOT_INT_CONSOLE_DEBUG 0x0D

bot_uint bot_vm_file_load(
	bot_vm *vm,
	char   *file_name
);

void bot_vm_report(
	bot_vm *vm
);

void bot_vm_io_run(
	bot_vm *vm
);

#endif

#ifdef BOT_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bot_char bot_rom_read(
	void *user
) {
	return (bot_char)fgetc((FILE*)user);
}

bot_uint bot_rom_size(
	void *user
) {
	FILE *file = (FILE*)user;
	long head  = ftell(file);

	fseek(file,0,SEEK_END);
	long size=ftell(file);
	fseek(file,head,SEEK_SET);

	return (bot_uint)size;
}

bot_uint bot_vm_file_load(
	bot_vm *vm,
	char   *file_name
) {
	FILE *bot_rom_file = fopen(file_name,"rb");

	if (bot_rom_file==NULL) return 1;

	bot_vm_load(
		vm,
		(bot_rom){
			bot_rom_file,
			bot_rom_size,
			bot_rom_read
		}
	);

	fclose(bot_rom_file);

	return 0;
}

void bot_vm_report(
	bot_vm *vm
) {
	switch(vm->INT) {
		case BOT_INT_INVALID_OPERATION:
			printf("[INVALID OPERATION]\n");

			break;
		case BOT_INT_INVALID_JUMP:
			printf("[JUMP TO INVALID ADDRESS]\n");

			break;
		case BOT_INT_OUT_OF_BOUNDS:
			printf("[ACCESS TO INVALID MEMORY]\n");

			break;
		case BOT_INT_OUT_OF_MEMORY:
			printf("[OUT OF MEMORY]\n");

			break;
		default:
			printf("[UNHANDLED INTERRUPT]\n");
	}

	printf(
		"INT: %.8X\n"
		"PC:  %.8X\n"
		"SP:  %.8X\n",
		vm->INT,
		vm->PC,
		vm->SP
	);
}

void bot_vm_io_run(
	bot_vm *vm
) {
	switch(vm->INT) {
		case BOT_INT_ARGUMENT:
			bot_vm_interrupt(vm,BOT_INT_NONE);

			break;
		case BOT_INT_CONSOLE_PRINT:
			bot_vm_interrupt(vm,BOT_INT_NONE);

			{
				bot_uint address = bot_vm_pop_uint(vm);
				bot_uint length  = bot_vm_get_uint(vm,address);

				bot_char buffer[length];

				bot_vm_get_string(
					vm,
					address+1,
					length,
					buffer
				);
				printf("%s",(char *)buffer);
			}

			break;
		case BOT_INT_CONSOLE_INPUT:
			bot_vm_interrupt(vm,BOT_INT_NONE);

			{
				bot_uint max_length = bot_vm_pop_uint(vm);
				bot_char buffer[max_length];

				fgets(
					(char *)buffer,
					sizeof(bot_char)*max_length,
					stdin
				);

				bot_uint length = (bot_uint)strlen((char *)buffer);

				bot_vm_push_uint(vm,length);
				bot_vm_push_string(
					vm,
					length,
					buffer
				);
			}

			break;
		case BOT_INT_CONSOLE_DEBUG:
			bot_vm_interrupt(vm,BOT_INT_NONE);

			printf("%u",bot_vm_pop_uint(vm));

			break;
	}
}

#endif

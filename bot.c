#include <stdio.h>
#include <stdlib.h>

#define BOT_IMPLEMENTATION
#include "bot.h"
#include "box.h"

int main(
	int argc,
	char *argv[]
) {
	if (argc<2) {
		printf(
			"BOT %u.%u.%u Copyright (C) 2025 Dice\n"
			"Usage: bot <file>\n",
			BOT_VERSION_MAJOR,
			BOT_VERSION_MINOR,
			BOT_VERSION_PATCH
		);

		return 0;
	}

	bot_vm *vm = bot_vm_new();

	if (vm==NULL) {
		printf("Failed to create vm instance\n");

		return -1;
	}

	if (bot_vm_file_load(vm,argv[1])) {
		printf(
			"Cannot open file: %s\n",
			argv[1]
		);

		return -1;
	}

	do {
		bot_vm_run(vm);
		bot_vm_io_run(vm);
	} while (!vm->INT);

	if (vm->INT!=BOT_INT_END_OF_PROGRAM) {
		bot_vm_report(vm);
	}

	bot_vm_free(vm);

	return 0;
}

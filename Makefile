# To build run 'make all'

PROGRAM = bot
CC      = gcc
CFLAGS  = -std=gnu99 -Wall -Wextra -Wno-attributes -s -O2

.PHONY: demos

all: demos
	$(CC) $(CFLAGS) $(PROGRAM).c -o $(PROGRAM)

demos: hello.bin fixed_point.bin

fixed_point.bin:
	lua bot.lua -i \
	lib/stdlib.bot \
	lib/string.bot \
	lib/stdio.bot \
	lib/fpu.bot \
	demos/fixed_point.bot \
	-e fixed_point.bin

hello.bin:
	lua bot.lua -i \
	lib/stdlib.bot \
	lib/string.bot \
	lib/stdio.bot \
	demos/hello.bot \
	-e hello.bin

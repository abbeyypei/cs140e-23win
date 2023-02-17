BOOTLOADER=my-install

START = staff-start.o

# part 6: final
PROGS = 6-single-step-proc.c

# have a common library for the tests
LIB = libasm.a

# part 0
# STAFF_LIB_OBJS += staff-0-user-sp-lr-asm.o
LIB_SRC += interrupt-asm.S  breakpoint.c

# automatically run.
RUN=1

CAN_EMIT=1

EXCLUDE ?= grep -v simple_boot
include $(CS140E_2023_PATH)/libpi/mk/Makefile.template-v2

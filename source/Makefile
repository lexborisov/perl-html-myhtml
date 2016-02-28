TARGET := myhtml
SRCDIR := myhtml

CC ?= gcc
CFLAGS ?= -Wall -std=c99 -I. -pthread

SRCS := $(wildcard $(SRCDIR)/*.c)
HDRS := $(wildcard $(SRCDIR)/*.h)
OBJS := $(patsubst %.c,%.o,$(SRCS))

CFLAGS  += -pthread
LIBNAME := myhtml

all: create
	$(MAKE) -C $(SRCDIR) all
	cp $(SRCDIR)/*lib$(LIBNAME).* lib/

clean:
	$(MAKE) -C $(SRCDIR) clean
	rm -rf lib/*

create:
	mkdir -p lib

.PHONY:all clean
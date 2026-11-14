ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)
include $(ROOT)/config.mk

OBJDIR := bin/$(VARIANT)
SRCS   := $(wildcard *.c)
OBJS   := $(SRCS:%.c=$(OBJDIR)/%.o)
LIB    := $(OBJDIR)/libcommon.a

CFLAGS += -Iinclude

.PHONY: all test clean
all: $(LIB)

$(LIB): $(OBJS)
	ar rcs $@ $^

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $@

test:
	@echo "common: no tests yet"

clean:
	rm -rf bin

-include $(OBJS:.o=.d)
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)
include $(ROOT)/config.mk

OBJDIR := bin/$(VARIANT)
SRCS   := $(wildcard *.c)
OBJS   := $(SRCS:%.c=$(OBJDIR)/%.o)
LIB    := $(OBJDIR)/libcommon.a

CFLAGS += -Iinclude

UNITY_OBJ := $(OBJDIR)/unity.o
TEST_SRCS := $(wildcard tests/test_*.c)
TEST_BINS := $(TEST_SRCS:tests/%.c=$(OBJDIR)/tests/%)

.PHONY: all test clean
all: $(LIB)

$(LIB): $(OBJS)
	ar rcs $@ $^

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(UNITY_OBJ): $(UNITY_DIR)/unity.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/tests/%: tests/%.c $(UNITY_OBJ) $(LIB) | $(OBJDIR)/tests
	$(CC) $(CFLAGS) -I$(UNITY_DIR) $< $(UNITY_OBJ) $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(OBJDIR) $(OBJDIR)/tests:
	mkdir -p $@

# timeout exits 124 on a hang, so a deadlocked test fails the build
test: $(TEST_BINS)
	@for t in $(TEST_BINS); do \
		echo "== $$t"; \
		timeout $(TEST_TIMEOUT) ./$$t || exit 1; \
	done

clean:
	rm -rf bin

-include $(OBJS:.o=.d)
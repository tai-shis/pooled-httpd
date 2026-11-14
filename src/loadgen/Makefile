NAME := loadgen
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)
include $(ROOT)/config.mk

OBJDIR  := bin/$(VARIANT)
SRCS    := $(wildcard *.c)
OBJS    := $(SRCS:%.c=$(OBJDIR)/%.o)
TARGET  := $(ROOT)/$(NAME)

CFLAGS  += -Iinclude -I$(COMMON_DIR)/include

# Everything except main.o, so tests can link the code without two mains
LIB_OBJS   := $(filter-out $(OBJDIR)/main.o,$(OBJS))
UNITY_OBJ  := $(OBJDIR)/unity.o
TEST_SRCS  := $(wildcard tests/test_*.c)
TEST_BINS  := $(TEST_SRCS:tests/%.c=$(OBJDIR)/tests/%)

.PHONY: all test clean common
all: $(TARGET)

# Each executable builds common itself so it works standalone
common:
	$(MAKE) -C $(COMMON_DIR) VARIANT=$(VARIANT)
$(COMMON_LIB): common

$(TARGET): $(OBJS) $(COMMON_LIB)
	$(CC) $(LDFLAGS) $(OBJS) $(COMMON_LIB) $(LDLIBS) -o $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(UNITY_OBJ): $(UNITY_DIR)/unity.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/tests/%: tests/%.c $(UNITY_OBJ) $(LIB_OBJS) $(COMMON_LIB) | $(OBJDIR)/tests
	$(CC) $(CFLAGS) -I$(UNITY_DIR) $< $(UNITY_OBJ) $(LIB_OBJS) \
		$(COMMON_LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(OBJDIR) $(OBJDIR)/tests:
	mkdir -p $@

# timeout exits 124 on a hang, so a deadlocked test fails the build
test: $(TEST_BINS)
	@for t in $(TEST_BINS); do \
		echo "== $$t"; \
		timeout $(TEST_TIMEOUT) ./$$t || exit 1; \
	done

clean:
	rm -rf bin $(TARGET)

-include $(OBJS:.o=.d)
SRC_DIRS := server loadgen view

.PHONY: all debug tsan test test-tsan clean

# $(1) = target to pass down, $(2) = variant
define build
	$(MAKE) -C src/common $(1) VARIANT=$(2)
	@for d in $(SRC_DIRS); do \
		$(MAKE) -C src/$$d $(1) VARIANT=$(2) || exit 1; \
	done
endef

all:
	$(call build,all,release)

debug:
	$(call build,all,debug)

tsan:
	$(call build,all,tsan)

test:
	$(call build,test,release)

test-tsan:
	$(call build,test,tsan)

clean:
	$(MAKE) -C src/common clean
	@for d in $(SRC_DIRS); do $(MAKE) -C src/$$d clean; done
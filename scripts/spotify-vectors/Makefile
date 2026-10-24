DESKTOP ?= ../../..
BUILD ?= build
OUT ?= ../../spotify
CC ?= cc
CXX ?= c++

PROTOCOL := $(DESKTOP)/src/spotify/protocol
BEARSSL := $(DESKTOP)/contrib/bearssl
BEARSSL_SOURCES := $(sort $(wildcard $(BEARSSL)/src/*/*.c))
PROTOCOL_SOURCES := $(sort $(wildcard $(PROTOCOL)/*.cpp))
GENERATOR_SOURCES := main.cpp primitives.cpp handshake.cpp wire.cpp
GENERATOR_HEADERS := json.h vectors.h

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(PROTOCOL)/access_point.cpp),)
$(error DESKTOP=$(DESKTOP) has no src/spotify/protocol: point it at a checkout of Kickoman/QiYaa)
endif
endif

.PHONY: vectors clean

vectors: $(BUILD)/spotify-vectors
	mkdir -p $(OUT)
	$(BUILD)/spotify-vectors $(OUT)

$(BUILD)/spotify-vectors: $(GENERATOR_SOURCES) $(GENERATOR_HEADERS) $(PROTOCOL_SOURCES) $(PROTOCOL)/modpow.c $(BEARSSL_SOURCES)
	@rm -rf $(BUILD)/c && mkdir -p $(BUILD)/c
	cd $(BUILD)/c && $(CC) -std=c99 -O2 -w -c -I$(abspath $(BEARSSL))/inc -I$(abspath $(BEARSSL))/src \
		-I$(abspath $(DESKTOP))/src $(abspath $(BEARSSL_SOURCES)) $(abspath $(PROTOCOL))/modpow.c
	$(CXX) -std=c++20 -O2 -Wall -Wextra -I$(DESKTOP)/src -I$(BEARSSL)/inc -I. -o $@ \
		$(GENERATOR_SOURCES) $(PROTOCOL_SOURCES) $(BUILD)/c/*.o

clean:
	rm -rf $(BUILD)

DESKTOP ?= ../../..
BUILD ?= build
OUT ?= ../../spotify
SPEC ?= ../..
QT ?= $(HOME)/Qt/6.8.3/gcc_64
MOC ?= $(QT)/libexec/moc
CC ?= cc
CXX ?= c++

PROTOCOL := $(DESKTOP)/src/spotify/protocol
BEARSSL := $(DESKTOP)/contrib/bearssl
BEARSSL_SOURCES := $(sort $(wildcard $(BEARSSL)/src/*/*.c))
PROTOCOL_SOURCES := $(sort $(wildcard $(PROTOCOL)/*.cpp))
GENERATOR_SOURCES := main.cpp helpers.cpp primitives.cpp handshake.cpp wire.cpp
CATALOG_BUILD := $(BUILD)/catalog
QT_FLAGS := -fPIC -DQT_NO_KEYWORDS -I$(QT)/include -I$(QT)/include/QtCore -I$(QT)/include/QtNetwork
QT_LIBS := -L$(QT)/lib -Wl,-rpath,$(abspath $(QT))/lib -lQt6Network -lQt6Core

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(PROTOCOL)/access_point.cpp),)
$(error DESKTOP=$(DESKTOP) has no src/spotify/protocol: point it at a checkout of Kickoman/QiYaa)
endif
ifneq ($(filter catalog,$(MAKECMDGOALS)),)
ifeq ($(wildcard $(MOC)),)
$(error QT=$(QT) has no libexec/moc: point it at a Qt 6 installation, the one the desktop builds with)
endif
endif
endif

.PHONY: vectors catalog clean $(BUILD)/spotify-vectors $(BUILD)/spotify-catalog

vectors: $(BUILD)/spotify-vectors
	mkdir -p $(OUT)
	$(BUILD)/spotify-vectors $(OUT)

$(BUILD)/spotify-vectors:
	@rm -rf $(BUILD)/c $(BUILD)/spotify-vectors && mkdir -p $(BUILD)/c
	cd $(BUILD)/c && $(CC) -std=c99 -O2 -w -c -I$(abspath $(BEARSSL))/inc -I$(abspath $(BEARSSL))/src \
		-I$(abspath $(DESKTOP))/src $(abspath $(BEARSSL_SOURCES)) $(abspath $(PROTOCOL))/modpow.c
	$(CXX) -std=c++20 -O2 -Wall -Wextra -I$(DESKTOP)/src -I$(BEARSSL)/inc -I. -o $@ \
		$(GENERATOR_SOURCES) $(PROTOCOL_SOURCES) $(BUILD)/c/*.o

catalog: $(BUILD)/spotify-catalog
	$(BUILD)/spotify-catalog $(SPEC)

$(BUILD)/spotify-catalog:
	@rm -rf $(CATALOG_BUILD) $(BUILD)/spotify-catalog && mkdir -p $(CATALOG_BUILD)
	cd $(CATALOG_BUILD) && $(CC) -std=c99 -O2 -w -c -I$(abspath $(BEARSSL))/inc -I$(abspath $(BEARSSL))/src \
		-I$(abspath $(DESKTOP))/src $(abspath $(BEARSSL_SOURCES)) $(abspath $(PROTOCOL))/modpow.c
	$(MOC) -p sources/spotify $(DESKTOP)/src/sources/spotify/library.h -o $(CATALOG_BUILD)/moc_library.cpp
	$(MOC) -p spotify/client $(DESKTOP)/src/spotify/client/session.h -o $(CATALOG_BUILD)/moc_session.cpp
	$(CXX) -std=c++20 -O2 -Wall -Wextra $(QT_FLAGS) -I$(DESKTOP)/src -I$(BEARSSL)/inc -I. -o $@ \
		catalog.cpp helpers.cpp $(CATALOG_BUILD)/moc_library.cpp $(CATALOG_BUILD)/moc_session.cpp \
		$(DESKTOP)/src/core/track.cpp $(DESKTOP)/src/sources/spotify/tracks.cpp $(PROTOCOL_SOURCES) \
		$(CATALOG_BUILD)/*.o $(QT_LIBS)

clean:
	rm -rf $(BUILD)/c $(BUILD)/spotify-vectors $(CATALOG_BUILD) $(BUILD)/spotify-catalog

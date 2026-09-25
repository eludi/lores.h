# Linux: make             (requires ALSA development headers/libraries)
# Windows: make           (run in a MinGW/MSYS2 shell)
# Linux -> Windows: make mingw
# Without audio dependencies: make AUDIO=0
# Override CROSS for another MinGW toolchain, e.g. CROSS=i686-w64-mingw32-

ifeq ($(OS),Windows_NT)
PLATFORM ?= windows
CROSS ?=
else
PLATFORM ?= linux
CROSS ?= x86_64-w64-mingw32-
endif

AUDIO ?= 1
ifeq ($(AUDIO),0)
CPPFLAGS += -DLORES_NO_AUDIO
BUILD_SUFFIX = -no-audio
endif

ifeq ($(PLATFORM),windows)
ifeq ($(origin CC),default)
CC = $(CROSS)gcc
endif
ifeq ($(origin CXX),default)
CXX = $(CROSS)g++
endif
EXE = .exe
else
ifneq ($(AUDIO),0)
CFLAGS += -pthread
CXXFLAGS += -pthread
LDLIBS += -lasound -lm -pthread
endif
endif

CFLAGS += -O2 -std=gnu11
CXXFLAGS += -O2 -std=gnu++11
BUILD_DIR ?= build/$(PLATFORM)$(BUILD_SUFFIX)
PROGRAMS = $(addprefix $(BUILD_DIR)/,snake$(EXE) lowiz$(EXE) blockbuf_test$(EXE))

.PHONY: all linux mingw test clean
all: $(PROGRAMS)

linux:
	$(MAKE) PLATFORM=linux all

mingw:
	$(MAKE) PLATFORM=windows all

$(BUILD_DIR):
	mkdir -p "$@"

$(BUILD_DIR)/snake$(EXE): snake.c lores.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o "$@" snake.c $(LDLIBS)

$(BUILD_DIR)/blockbuf_test$(EXE): tests/blockbuf_test.c lores.h sprites.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o "$@" tests/blockbuf_test.c $(LDLIBS)

$(BUILD_DIR)/lowiz$(EXE): lowiz.cpp lores.h sprites.c | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o "$@" lowiz.cpp $(LDLIBS)

# Linux-only sanitizer checks; uses Bash and the native GCC toolchain.
test:
	bash tests/run.sh

clean:
	$(RM) $(PROGRAMS)

SHELL := /bin/bash

# Linux-friendly build for this repo (game + networking + raylib).
# Targets:
#   make           -> build output/pharmasea
#   make run       -> build + run
#   make deps      -> fetch + build third-party libs (raylib + GameNetworkingSockets)
#   make clean     -> remove build artifacts

BUILD_DIR := output
OBJ_DIR := $(BUILD_DIR)/obj
# Build the executable at repo root to match existing docs/scripts.
BIN := pharmasea.exe

THIRD_PARTY := third_party

RAYLIB_DIR := $(THIRD_PARTY)/raylib
RAYLIB_BUILD := $(RAYLIB_DIR)/build
RAYLIB_TAG := 5.5
RAYLIB_LIB := $(RAYLIB_BUILD)/raylib/libraylib.a
RAYLIB_INC := $(RAYLIB_DIR)/src

GNS_DIR := $(THIRD_PARTY)/GameNetworkingSockets
GNS_BUILD := $(GNS_DIR)/build
GNS_LIB := $(GNS_BUILD)/src/libGameNetworkingSockets_s.a
GNS_INC := $(GNS_DIR)/include

CC := gcc
CXX := g++

STD := -std=c++20

# IMPORTANT (Linux): avoid colliding with libc's <strings.h>.
# Using -iquote keeps project headers available for `#include "..."` without
# letting system headers accidentally resolve to `src/strings.h`.
INCLUDES := -Ivendor -Ivendor/cereal/include -iquote src -I$(RAYLIB_INC) -I$(GNS_INC)

# Keep warnings reasonable for local iteration; treat warnings as errors later in CI.
WARNINGS := -Wall -Wextra -Wshadow -Wuninitialized

DEFINES := -DTRACY_ENABLE

CXXFLAGS := $(STD) $(WARNINGS) -g -O0 $(DEFINES) $(INCLUDES) \
	-Wno-missing-field-initializers -Wno-unused-function -Wno-sign-conversion \
	-Wfatal-errors -include src/pch.hpp

CPPFLAGS :=

# Linker libs for raylib desktop (X11/OpenGL) + GameNetworkingSockets deps.
# Include libdw/libunwind + -rdynamic so backward-cpp can symbolize stacks.
LDFLAGS := -rdynamic
LDLIBS := -ldw -lunwind -lprotobuf -lssl -lcrypto -lz -lpthread -ldl -lm \
	-lX11 -lXrandr -lXi -lXinerama -lXcursor -lGL

# Main game sources + required vendor .cpp units
SRC := $(shell find src -type f -name '*.cpp' | sort) vendor/tracy/TracyClient.cpp

OBJ := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))
DEPS := $(OBJ:.o=.d)

.PHONY: all run clean deps third-party raylib gns

all: $(BIN)

run: $(BIN)
	"./$(BIN)"

deps: third-party

third-party: $(RAYLIB_LIB) $(GNS_LIB)

$(BIN): third-party $(OBJ)
	@mkdir -p "$(dir $@)"
	$(CXX) $(LDFLAGS) $(OBJ) $(GNS_LIB) $(RAYLIB_LIB) $(LDLIBS) -o "$(BIN)"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p "$(dir $@)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c "$<" -o "$@"

clean:
	rm -rf "$(BUILD_DIR)"

# --- raylib ---
$(RAYLIB_DIR)/.git:
	@mkdir -p "$(THIRD_PARTY)"
	git clone --depth 1 --branch "$(RAYLIB_TAG)" https://github.com/raysan5/raylib.git "$(RAYLIB_DIR)"

$(RAYLIB_LIB): $(RAYLIB_DIR)/.git
	cmake -S "$(RAYLIB_DIR)" -B "$(RAYLIB_BUILD)" -G Ninja \
		-DBUILD_SHARED_LIBS=OFF \
		-DBUILD_EXAMPLES=OFF \
		-DPLATFORM=Desktop \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_COMPILER="$(CC)" \
		-DCMAKE_CXX_COMPILER="$(CXX)"
	cmake --build "$(RAYLIB_BUILD)" --target raylib

# --- GameNetworkingSockets ---
$(GNS_DIR)/.git:
	@mkdir -p "$(THIRD_PARTY)"
	git clone --depth 1 https://github.com/ValveSoftware/GameNetworkingSockets.git "$(GNS_DIR)"

$(GNS_LIB): $(GNS_DIR)/.git
	cmake -S "$(GNS_DIR)" -B "$(GNS_BUILD)" -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_COMPILER="$(CC)" \
		-DCMAKE_CXX_COMPILER="$(CXX)"
	cmake --build "$(GNS_BUILD)" --target GameNetworkingSockets_s

-include $(DEPS)



# Prefer system raylib if available; otherwise fall back to vendored headers.
# (Vendored repo includes headers but not a Linux lib, so full linking may still
# require a system raylib install.)
RAYLIB_FLAGS := $(shell pkg-config --cflags raylib 2>/dev/null) -Ivendor/raylib
RAYLIB_LIB := $(shell pkg-config --libs raylib 2>/dev/null)

# Local GameNetworkingSockets paths (built from ~/p/GameNetworkingSockets)
# Default to vendored copy; override if you have a local build.
GNS_INC := vendor
GNS_LIBDIR := vendor

RELEASE_FLAGS = -std=c++2a $(RAYLIB_FLAGS) -DNDEBUG

# TIMEFLAG = -ftime-trace
TIMEFLAG =

# Optimized flags for debug builds (fewer expensive warnings)
DEBUG_WARNING_FLAGS = -Wall -Wextra -Wuninitialized -Wshadow -Wno-conversion \
                      -Wno-pedantic -Wno-sign-conversion -Wno-implicit-int-float-conversion

# Full flags for release builds
RELEASE_WARNING_FLAGS = -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow \
                        -Wconversion

FLAGS = -std=c++2a $(DEBUG_WARNING_FLAGS) -g $(RAYLIB_FLAGS) -DTRACY_ENABLE $(TIMEFLAG)

# LEAKFLAGS = -fsanitize=address
NOFLAGS = -Wno-deprecated-volatile -Wno-missing-field-initializers \
		  -Wno-c99-extensions -Wno-unused-function -Werror
# IMPORTANT (Linux): avoid colliding with libc's <strings.h>.
# Using -iquote keeps project headers available for `#include "..."` without
# letting system headers accidentally resolve to `src/strings.h`.
INCLUDES = -I$(GNS_INC) -Ivendor/ -iquote src
LIBS = -L$(GNS_LIBDIR) -lGameNetworkingSockets -Lvendor/ $(RAYLIB_LIB)

# backward-cpp (Debug only) - cache pkg-config calls
UNAME_S := $(shell uname -s)
BACKWARD_DW_LIBS := $(shell pkg-config --libs libdw 2>/dev/null)
BACKWARD_DW_CFLAGS := $(shell pkg-config --cflags libdw 2>/dev/null)
BACKWARD_UNWIND_LIBS := $(shell pkg-config --libs libunwind 2>/dev/null)
BACKWARD_UNWIND_CFLAGS := $(shell pkg-config --cflags libunwind 2>/dev/null)
BACKWARD_FLAGS := -DBACKWARD_HAS_DW=$(if $(BACKWARD_DW_LIBS),1,0) -DBACKWARD_HAS_LIBUNWIND=$(if $(BACKWARD_UNWIND_LIBS),1,0)

FLAGS += $(BACKWARD_FLAGS) $(BACKWARD_DW_CFLAGS) $(BACKWARD_UNWIND_CFLAGS)
ifeq ($(UNAME_S),Darwin)
BACKWARD_PLATFORM_LIBS :=
else
BACKWARD_PLATFORM_LIBS := -ldl -Wl,--export-dynamic
endif
LIBS += $(BACKWARD_DW_LIBS) $(BACKWARD_UNWIND_LIBS) $(BACKWARD_PLATFORM_LIBS)

# SRC_FILES := $(wildcard src/*.cpp src/**/*.cpp src/engine/**/*.cpp vendor/tracy/TracyClient.cpp)
SRC_FILES := $(wildcard src/*.cpp src/**/*.cpp src/engine/**/*.cpp src/network/**/*.cpp)
H_FILES := $(wildcard src/**/*.h src/engine/**/*.h) 
OBJ_DIR := ./output
OBJ_FILES := $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

# Precompiled header setup
PCH_HEADER := src/pch.hpp
PCH_GCH := src/pch.hpp.gch

OUTPUT_EXE := pharmasea.exe

# CXX := g++
CXX := clang++
# CXX := clang++ --analyze
# CXX := include-what-you-use

# Use ccache if available for faster recompilation
CCACHE := $(shell command -v ccache 2>/dev/null)
ifdef CCACHE
    CXX := ccache $(CXX)
endif

OUTPUT_LOG = $(OBJ_DIR)/build.log
GAME_LOG = $(OBJ_DIR)/game.log

# For tracing you have to run the game, and then connect from Tracy-release

.PHONY: all clean

all: post-build

pre-build:
	@true

main-build: pre-build $(OUTPUT_EXE) 

post-build: main-build
	install_name_tool -change @rpath/libGameNetworkingSockets.dylib $(GNS_LIBDIR)/libGameNetworkingSockets.dylib $(OUTPUT_EXE)
	# ./$(OUTPUT_EXE) 2>&1 $(GAME_LOG)
# -g disables sounds 
# ./$(OUTPUT_EXE) -g 2>&1 $(GAME_LOG)


$(OUTPUT_EXE): $(H_FILES) $(OBJ_FILES) 
	$(CXX) $(FLAGS) $(LEAKFLAGS) $(NOFLAGS) $(INCLUDES) $(OBJ_FILES) $(LIBS) -o $(OUTPUT_EXE) 

release: FLAGS=$(RELEASE_FLAGS)
release: NOFLAGS=
release: clean all
	rm -rf release
	mkdir release
	cp $(OUTPUT_EXE) release/
	cp README.md release/ 
	cp libGameNetworkingSockets.dylib release/
	cp -r resources release/
	cp -r vendor release/

modeltest:
	$(CXX) -std=c++2a -g $(RAYLIB_FLAGS) $(RAYLIB_LIB) -Ivendor/ model_test.cpp;./a.out

$(OBJ_DIR)/%.o: %.cpp makefile $(PCH_GCH)
	@mkdir -p $(dir $@)
	$(CXX) $(FLAGS) $(NOFLAGS) $(INCLUDES) -include $(PCH_HEADER) -Wno-deprecated-literal-operator -Wno-invalid-utf8 -Wno-implicit-float-conversion -Wno-c99-extensions -c $< -o $@ -MMD -MF $(@:.o=.d)

%.d: %.cpp
	$(MAKEDEPEND)

# Build the precompiled header (GCC/Clang-compatible). Clang and GCC will
# consume this automatically when using -include $(PCH_HEADER) if the .gch
# is present next to the header path.
# Note: PCH will be rebuilt automatically by make when pch.hpp or makefile changes.
# If vendor files change and PCH becomes stale, run 'make clean' or delete $(PCH_GCH) manually.
$(PCH_GCH): $(PCH_HEADER) makefile src/bitsery_include.h src/std_include.h src/log/log.h
	$(CXX) $(FLAGS) $(INCLUDES) -DFMT_HEADER_ONLY -DFMT_USE_NONTYPE_TEMPLATE_PARAMETERS=0 -DFMT_CONSTEVAL= -Wno-deprecated-literal-operator -Wno-invalid-utf8 -Wno-implicit-float-conversion -Wno-c99-extensions -x c++-header -o $@ $<

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(PCH_GCH)
	mkdir -p $(OBJ_DIR)/src/network/
	mkdir -p $(OBJ_DIR)/src/network/internal/
	mkdir -p $(OBJ_DIR)/src/layers/
	mkdir -p $(OBJ_DIR)/src/dataclass/
	mkdir -p $(OBJ_DIR)/src/engine/
	mkdir -p $(OBJ_DIR)/src/engine/network/
	mkdir -p $(OBJ_DIR)/src/engine/simulated_input/
	mkdir -p $(OBJ_DIR)/src/components/
	mkdir -p $(OBJ_DIR)/src/engine/ui/
	mkdir -p $(OBJ_DIR)/src/system/
	mkdir -p $(OBJ_DIR)/src/intro/
	mkdir -p $(OBJ_DIR)/vendor/tracy/
	mkdir -p $(OBJ_DIR)/vendor/backward/

count: 
	git ls-files | grep "src" | grep -v "ui_color.h" | grep -v "vendor" | grep -v "resources" | grep -v "color.h" | xargs wc -l | sort -rn | pr -2 -t -w 100

countall: 
	git ls-files | xargs wc -l | sort -rn

todo: 
	{ git grep -niE '(FIXME|TODO)' src/; cat todo.md | grep -e "- \[\s";} 

todoadded: 
	git log -n 10 --pretty=format:%H | xargs -I {} git diff {} | grep -E "^\+.*TODO"

# for shuf you might need to 
# brew install coreutils
todor:
	{ git grep -niE '(FIXME|TODO)' src/; cat todo.md | grep -e "- \[\s";} | shuf -n1

cppcheck: 
	cppcheck src/ --enable=all --std=c++20 --language=c++ --suppress=noConstructor --suppress=noExplicitConstructor --suppress=useStlAlgorithm --suppress=unusedStructMember --suppress=useInitializationList --suppress=duplicateCondition --suppress=nullPointerRedundantCheck --suppress=cstyleCast
 
gendocs:
	doxygen doxyconfig
	git add .
	git commit -m "update docs" 

prof: 
	rm -rf recording.trace/
	xctrace record --template 'Game Performance' --output 'recording.trace' --launch $(OUTPUT_EXE)

leak: 
	rm -rf recording.trace/
	codesign -s - -f --verbose --entitlements ent_pharmasea.plist $(OUTPUT_EXE)
	xctrace record --template 'Leaks' --output 'recording.trace' --launch $(OUTPUT_EXE)

alloc: 
	rm -rf recording.trace/
	codesign -s - -f --verbose --entitlements ent_pharmasea.plist $(OUTPUT_EXE)
	xctrace record --template 'Allocations' --output 'recording.trace' --launch $(OUTPUT_EXE)

translate:
	python3 scripts/reverse_translation.py > src/translation_en_rev.h

findstr:
	grep -r "\"" src/ | grep -v "preload"  | grep -v "game.cpp" | grep -v "src//strings.h" | grep -v "include" | grep -v "src//test" | grep -v "src//engine" | grep -v "src//dataclass" | grep -v "log" | grep -v "TODO" | grep -v "VALIDATE" 

cleansave:
	rm "/Users/gabeochoa/Library/Application Support/pharmasea/settings.bin"

bring:
	cp ~/p/GameNetworkingSockets/build/bin/libGameNetworkingSockets.dylib .
	cp ~/p/GameNetworkingSockets/build/bin/libGameNetworkingSockets.dylib vendor/
	git update-index --assume-unchanged libGameNetworkingSockets.dylib 
	git update-index --assume-unchanged vendor/libGameNetworkingSockets.dylib 

heavycompile:
	ls output/src | grep json | xargs -n1 -I {} wc -c output/src/{} | sort -r

raylib4.5:
	brew install --build-from-source ./raylib.rb

	
# https://www.flourish.org/cinclude2dot/cinclude2dot
# visualize in https://dreampuf.github.io/Graphvi 
headermap:
	scripts/cinclude2dot > output/source.dot
	python3 scripts/count_includes.py

# When using lldb, you have to run these commands:
# 	settings set platform.plugin.darwin.ignored-exceptions EXC_BAD_INSTRUCTION
# 	c
# 	c
# 	c
# and then itll work fine :) 
# from: https://stackoverflow.com/questions/74059978/why-is-lldb-generating-exc-bad-instruction-with-user-compiled-library-on-macos
# You can also do: 
# 	lldb ./pharmasea
# 	command source lldbcommands 
# and thatll handle it for you


# if you are getting dylib errors,
# build GameNetworkingSockets locally 
# 	git clone ... cmake ... ninja ... 
# and then copy the new dylib into this folder (it should be the same) 
#
# then make and if you see protobuf or other dylib try installing the one you just added .
# install_name_tool -id @executable_path/libGameNetworkingSockets.dylib libGameNetworkingSockets.dylib

-include $(OBJ_FILES:.o=.d)



RAYLIB_FLAGS := `pkg-config --cflags raylib` 
RAYLIB_LIB := `pkg-config --libs raylib` 

RELEASE_FLAGS = -std=c++2a $(RAYLIB_FLAGS) 

FLAGS = -std=c++2a -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow \
		-Wmost -Wconversion -g $(RAYLIB_FLAGS) -DTRACY_ENABLE 
# LEAKFLAGS = -fsanitize=address
NOFLAGS = -Wno-deprecated-volatile -Wno-missing-field-initializers \
		  -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion
INCLUDES = -Ivendor/ 
LIBS = -L. -lGameNetworkingSockets -Lvendor/ $(RAYLIB_LIB)

# SRC_FILES := $(wildcard src/*.cpp src/**/*.cpp src/engine/**/*.cpp vendor/tracy/TracyClient.cpp)
SRC_FILES := $(wildcard src/*.cpp src/**/*.cpp src/engine/**/*.cpp vendor/tracy/TracyClient.cpp vendor/backward/backward.cpp)
H_FILES := $(wildcard src/**/*.h src/engine/**/*.h) 
OBJ_DIR := ./output
OBJ_FILES := $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

OUTPUT_EXE := pharmasea.exe

# CXX := g++
CXX := clang++

.PHONY: all clean

# For tracing you have to run the game, and then connect from Tracy-release



all: $(H_FILES) $(OBJ_FILES) 
	$(CXX) $(FLAGS) $(LEAKFLAGS) $(NOFLAGS) $(INCLUDES) $(LIBS) $(OBJ_FILES) -o $(OUTPUT_EXE) 
	./$(OUTPUT_EXE) -t && ./$(OUTPUT_EXE) -S
	# ./$(OUTPUT_EXE) -t


mp: $(H_FILES) $(OBJ_FILES) 
	$(CXX) $(FLAGS) $(NOFLAGS) $(INCLUDES) $(LIBS) $(OBJ_FILES) -o $(OUTPUT_EXE) && ./$(OUTPUT_EXE) test host > host_log & sleep 5;./pharmasea test client

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


$(OBJ_DIR)/%.o: %.cpp makefile
	$(CXX) $(FLAGS) $(NOFLAGS) $(INCLUDES) -c $< -o $@ -MMD -MF $(@:.o=.d)

%.d: %.cpp
	$(MAKEDEPEND)

clean:
	rm -r $(OBJ_DIR)
	mkdir -p $(OBJ_DIR)/src/network/
	mkdir -p $(OBJ_DIR)/src/engine/
	mkdir -p $(OBJ_DIR)/src/engine/network/
	mkdir -p $(OBJ_DIR)/src/system/
	mkdir -p $(OBJ_DIR)/vendor/tracy/
	mkdir -p $(OBJ_DIR)/vendor/backward/

profile: 
	xctrace record --output . --template "Time Profiler" --time-limit 10s --attach `ps -ef | grep "\./$(OUTPUT_EXE)" | grep -v "grep"  | cut -d' ' -f4`

count: 
	git ls-files | grep "src" | grep -v "ui_color.h" | grep -v "vendor" | grep -v "resources" | xargs wc -l | sort -rn

countall: 
	git ls-files | xargs wc -l | sort -rn

todo: 
	{ git grep -niE '(FIXME|TODO)' src/; cat todo.md | grep -e "- \[\s";} 

# for shuf you might need to 
# brew install coreutils
todor:
	{ git grep -niE '(FIXME|TODO)' src/; cat todo.md | grep -e "- \[\s";} | shuf -n1

cppcheck: 
	cppcheck --enable=all --std=c++20 --language=c++ --output-file=cppcheck_err src/

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

translate:
	msgfmt -o resources/translations/en_us.mo resources/translations/en_us.po
	msgfmt -o resources/translations/en_rev.mo resources/translations/en_rev.po

findstr:
	grep -r "\"" src/ | grep -v "preload"  | grep -v "game.cpp" | grep -v "src//strings.h" | grep -v "include" | grep -v "src//test" | grep -v "src//engine" | grep -v "src//dataclass" | grep -v "log" | grep -v "TODO" | grep -v "VALIDATE" 

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

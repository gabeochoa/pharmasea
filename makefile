

RAYLIB_FLAGS := `pkg-config --cflags raylib` 
RAYLIB_LIB := `pkg-config --libs raylib` 

FLAGS = -std=c++2a -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow \
		-Wmost -Wconversion -g -fsanitize=address $(RAYLIB_FLAGS)
NOFLAGS = -Wno-deprecated-volatile -Wno-missing-field-initializers \
		  -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion
INCLUDES = -Ivendor/ 
LIBS = -lGameNetworkingSockets -Lvendor/ $(RAYLIB_LIB)

SRC_FILES := $(wildcard src/*.cpp src/**/*.cpp src/engine/**/*.cpp)
H_FILES := $(wildcard src/**/*.h src/engine/**/*.h) 
OBJ_DIR := ./output
OBJ_FILES := $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

CXX := clang++

.PHONY: all clean


all: $(H_FILES) $(OBJ_FILES)
	$(CXX) $(FLAGS) $(NOFLAGS) $(INCLUDES) $(LIBS) $(OBJ_FILES) -o pharmasea && ./pharmasea


$(OBJ_DIR)/%.o: %.cpp makefile
	$(CXX) $(FLAGS) $(NOFLAGS) $(INCLUDES) -c $< -o $@ -MMD -MF $(@:.o=.d)

%.d: %.cpp
	$(MAKEDEPEND)

clean:
	rm -r $(OBJ_DIR)
	mkdir -p $(OBJ_DIR)/src/network/
	mkdir -p $(OBJ_DIR)/src/engine/
	mkdir -p $(OBJ_DIR)/src/engine/network/

profile: 
	xctrace record --output . --template "Time Profiler" --time-limit 10s --attach `ps -ef | grep "\./pharmasea" | grep -v "grep"  | cut -d' ' -f4`

count: 
	git ls-files | grep "src" | grep -v "ui_color.h" | grep -v "vendor" | grep -v "resources" | xargs wc -l | sort -rn

countall: 
	git ls-files | xargs wc -l | sort -rn

todo: 
	git grep -niE '(FIXME|TODO)' src/

-include $(OBJ_FILES:.o=.d)

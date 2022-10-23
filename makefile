

FLAGS = -std=c++2a -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow -Wmost -Wconversion -g -fsanitize=address
NOFLAGS = -Wno-deprecated-volatile -Wno-missing-field-initializers -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion
INCLUDES = -Ivendor/ 
CPPS = src/game.cpp src/network/network.cpp


all:
	clang++ $(FLAGS) $(NOFLAGS) $(CPPS) `pkg-config --libs --cflags raylib` $(INCLUDES) -o pharmasea && ./pharmasea 

count: 
	git ls-files | grep "src" | grep -v "ui_color.h" | grep -v "vendor"| grep -v "resources" | xargs wc -l | sort -rn

countall: 
	git ls-files | xargs wc -l | sort -rn


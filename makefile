

FLAGS = -std=c++2a -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow -Wmost -g -fsanitize=address
NOFLAGS = -Wno-deprecated-volatile -Wno-missing-field-initializers -Wno-c99-extensions -Wno-unused-function
INCLUDES = -Ivendor/ 


all:
	clang++ $(FLAGS) $(NOFLAGS) src/game.cpp `pkg-config --libs --cflags raylib` $(INCLUDES) -o pharmasea >> log && ./pharmasea >> log


compile:
	clang++ $(FLAGS) $(NOFLAGS) src/game.cpp `pkg-config --libs --cflags raylib` $(INCLUDES) -o pharmasea >> log 

count: 
	git ls-files | grep "src" | grep -v "ui_color.h" | grep -v "vendor"| grep -v "resources" | xargs wc -l | sort -rn

countall: 
	git ls-files | xargs wc -l | sort -rn


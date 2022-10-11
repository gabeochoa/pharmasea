

FLAGS = -std=c++2a -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow -Wmost -g 
NOFLAGS = -Wno-deprecated-volatile -Wno-missing-field-initializers -Wno-c99-extensions -Wno-unused-function
INCLUDES = -Ivendor/ 


all: 
	clang++ $(FLAGS) $(NOFLAGS) src/game.cpp `pkg-config --libs --cflags raylib` $(INCLUDES) -o pharmasea && ./pharmasea

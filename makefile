

FLAGS = -std=c++2a -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow -Wmost -g -Wno-deprecated-volatile
INCLUDES = -Ivendor/ 


all: 
	clang++ $(FLAGS) game.cpp `pkg-config --libs --cflags raylib` $(INCLUDES) -o pharmasea && ./pharmasea



FLAGS = -std=c++2a -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow -Wmost -g -Wno-deprecated-volatile -Wno-missing-field-initializers -Wno-c99-extensions
INCLUDES = -Ivendor/ 


all: 
	clang++ $(FLAGS) src/package.cpp $(INCLUDES) -o packager && ./packager

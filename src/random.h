
#pragma once 

#include <random>

inline int randIn(int a, int b) { return a + (std::rand() % (b - a + 1)); }

inline int randSign() { 
	int sign = randIn(0, 1);
	if (sign == 0) {
		return -1;
	}
	else {
		return 1;
	}
}

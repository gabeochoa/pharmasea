
#pragma once 

#include <random>

inline int randIn(int a, int b) { return a + (std::rand() % (b - a + 1)); }

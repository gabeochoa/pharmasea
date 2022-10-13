#pragma once 

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang diagnostic ignored "-Wdeprecated-volatile"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#ifdef WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#include "raylib.h"
#include "drawing_util.h"

#ifdef WRITE_FILES
#include "../vendor/sago/platform_folders.h"
#endif

#ifdef __APPLE__
#pragma clang diagnostic pop
#else
#pragma enable_warn
#endif

#ifdef WIN32
#pragma GCC diagnostic pop
#endif


typedef Vector2 vec2;
typedef Vector3 vec3;
typedef Vector4 vec4;

#include <atomic>
#include <optional>
#include <vector>
#include <memory>
#include <stdio.h>
#include <iostream>
#include <functional>
#include <map>
#include <set>
#include <limits>
#include <numeric>
#include <ostream>
#include <deque>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <stack>
#include <unordered_map>

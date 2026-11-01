#pragma once

#include "external_include.h"

// Global Defines

// steam networking uses an "app id" that we dont have
// also the code isnt written yet :)
// TODO :IMPACT: add support for steam connections
#include "engine/log.h"
#define BUILD_WITHOUT_STEAM

// Enable tracing
// its also defined in:
// - vendor/tracy/TracyClient.cpp
// - app.cpp
// TODO :INFRA: find a way to enable this in the compiler so we dont have to do
// this #define ENABLE_TRACING 1 #include "engine/tracy.h"
#define ZoneScoped

///
#include "globals.h"
///

#include "engine.h"
//
#include "strings.h"
//
#include "layers/all_layers.h"

extern float DEADZONE;
extern ui::UITheme UI_THEME;
extern std::vector<std::string> theme_keys;
extern std::map<std::string, ui::UITheme> themes;

#if ENABLE_TESTS
// This one should be last
#include "./tests/all_tests.h"
#endif

int main(int argc, char* argv[]);

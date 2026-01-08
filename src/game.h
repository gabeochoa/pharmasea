
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

#define ENABLE_DEV_FLAGS 1

#if ENABLE_DEV_FLAGS
#include <argh.h>
#endif

///
#include "globals.h"
///

#include "engine.h"
//
#include "strings.h"
//
#include "layers/aboutlayer.h"
#include "layers/debug_settings.h"
#include "layers/fpslayer.h"
#include "layers/gamedebuglayer.h"
#include "layers/gamelayer.h"
#include "layers/handlayer.h"
#include "layers/menulayer.h"
#include "layers/networklayer.h"
#include "layers/pauselayer.h"
#include "layers/recipe_book_layer.h"
#include "layers/round_end_reason_layer.h"
#include "layers/seedmanagerlayer.h"
#include "layers/settingslayer.h"
#include "layers/streamersafelayer.h"
#include "layers/toastlayer.h"
#include "layers/uitestlayer.h"
#include "layers/versionlayer.h"
#include "layers/mapviewerlayer.h"

extern float DEADZONE;
extern ui::UITheme UI_THEME;
extern std::vector<std::string> theme_keys;
extern std::map<std::string, ui::UITheme> themes;

#if ENABLE_TESTS
// This one should be last
#include "./tests/all_tests.h"
#endif

int main(int argc, char* argv[]);

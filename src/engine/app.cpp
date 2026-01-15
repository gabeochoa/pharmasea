
#include "app.h"

#include "resolution.h"

#define ENABLE_TRACING 1
#include "tracy.h"
//
#include "event.h"
#include "keymap.h"
#include "mouse_map.h"
#include "raylib.h"
//

#include "log.h"
#include "settings.h"
#include "shader_library.h"
#include "simulated_input/simulated_input.h"

#ifdef AFTER_HOURS_ENABLE_MCP
#include <afterhours/src/plugins/mcp_server.h>

extern bool MCP_ENABLED;

namespace {
std::vector<uint8_t> capture_screenshot_png() {
  App& app = App::get();
  raylib::Image image = raylib::LoadImageFromTexture(app.get_main_texture());
  if (image.data == nullptr) {
    return {};
  }
  raylib::ImageFlipVertical(&image);

  int file_size = 0;
  unsigned char* png_data = raylib::ExportImageToMemory(image, ".png", &file_size);
  raylib::UnloadImage(image);

  if (png_data == nullptr || file_size <= 0) {
    return {};
  }

  std::vector<uint8_t> result(png_data, png_data + file_size);
  raylib::MemFree(png_data);
  return result;
}

void init_mcp_server() {
  if (!MCP_ENABLED) {
    return;
  }

  afterhours::mcp::MCPConfig config;
  config.get_screen_size = []() {
    return std::make_pair(WIN_W(), WIN_H());
  };
  config.capture_screenshot = capture_screenshot_png;
  config.mouse_move = [](int x, int y) {
    raylib::Rectangle rect{static_cast<float>(x), static_cast<float>(y), 1.0f, 1.0f};
    input_injector::schedule_mouse_click_at(rect);
  };
  config.mouse_click = [](int x, int y, int /* button */) {
    raylib::Rectangle rect{static_cast<float>(x), static_cast<float>(y), 1.0f, 1.0f};
    input_injector::schedule_mouse_click_at(rect);
    input_injector::inject_scheduled_click();
  };
  config.key_down = [](int keycode) {
    input_injector::set_key_down(keycode);
  };
  config.key_up = [](int keycode) {
    input_injector::set_key_up(keycode);
  };

  afterhours::mcp::init(config);
}
} // namespace
#endif

App App::instance;
bool App::created = false;

void App::create(const AppSettings& pt) {
    if (created) {
        log_error("Trying to create App twice");
        return;
    }
    new (&instance) App(pt);
    created = true;
}

App& App::get() {
    if (!created) {
        log_error("trying to fetch app but none was created");
    }
    return instance;
}

void App::start_post_processing() {
    if (!Settings::get().data.enable_postprocessing) return;
    auto post_processing_shader = ShaderLibrary::get().get("post_processing");
    raylib::BeginShaderMode(post_processing_shader);
}

void App::end_post_processing() {
    if (!Settings::get().data.enable_postprocessing) return;
    raylib::EndShaderMode();
}

App::App(const AppSettings& settings) {
    // This goes above Init since that loves to spew errors
    raylib::SetTraceLogLevel(settings.logLevel);
    raylib::SetTargetFPS(settings.fps);
    //
    width = settings.width;
    height = settings.height;

    raylib::InitWindow(width, height, settings.title);

    KeyMap::create();

    mainRT = raylib::LoadRenderTexture(width, height);
}

void App::loadLayers(const std::vector<Layer*>& layers) {
    for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
        layerstack[++max_layer] = *it;
    }
    log_info("Loaded {} layers", max_layer);
}

App::~App() {
    UnloadRenderTexture(mainRT);
    // TODO do we need to / can we remove mainRT from globals
}

void App::onEvent(Event& event) {
    EventDispatcher dispatcher(event);
    dispatcher.dispatch<WindowResizeEvent>(
        std::bind(&App::onWindowResize, this, std::placeholders::_1));

    dispatcher.dispatch<WindowFullscreenEvent>(
        std::bind(&App::onWindowFullscreen, this, std::placeholders::_1));
}

bool App::onWindowResize(WindowResizeEvent event) {
    log_trace("Got Window Resize Event: {}, {}", event.width, event.height);

    width = event.width;
    height = event.height;

    raylib::SetWindowSize(width, height);

    UnloadRenderTexture(mainRT);
    mainRT = raylib::LoadRenderTexture(width, height);
    return true;
}

bool App::onWindowFullscreen(WindowFullscreenEvent& event) {
    log_trace("Got Window Fullscreen toggle Event");

    bool isFullscreenOn = raylib::IsWindowFullscreen();

    // We want to turn it on and its already on
    if (event.on && isFullscreenOn) {
        return true;
    }

    // turn off and already off
    if (!event.on && !isFullscreenOn) {
        return true;
    }

    if (event.on && !isFullscreenOn) {
        prev_width = width;
        prev_height = height;

        int monitor = raylib::GetCurrentMonitor();

        int mon_width = raylib::GetMonitorWidth(monitor);
        int mon_height = raylib::GetMonitorHeight(monitor);

        Settings::get().update_window_size(
            rez::ResolutionInfo{.width = mon_width, .height = mon_height});

        raylib::ToggleFullscreen();
        return true;
    }

    if (!event.on && isFullscreenOn) {
        raylib::ToggleFullscreen();

        if (prev_width == -1) prev_width = width;
        if (prev_height == -1) prev_height = height;

        Settings::get().update_window_size(
            rez::ResolutionInfo{.width = prev_width, .height = prev_height});

        return true;
    }

    log_warn("Trying to process fs event but got to an unhandled state {} {}",
             event.on, isFullscreenOn);
    return false;
}

void App::processEvent(Event& e) {
    TRACY_ZONE_SCOPED;

    this->onEvent(e);
    input_recorder::record(e);
    if (e.handled) {
        return;
    }

    // Have the top most layers get the event first,
    // if they handle it then no need for the lower ones to get the rest
    // eg imagine UI pause menu blocking game UI elements
    //    we wouldnt want the player to click pass the pause menu

    int i = max_layer;
    while (i >= 0) {
        Layer* layer = layerstack[i];
        if (layer) layer->onEvent(e);
        if (e.handled) {
            break;
        }
        i--;
    }
}

void App::close() { running = false; }

void App::run() {
#ifdef ENABLE_TRACING
    log_info("Tracing is enabled");
#else
    log_info("Tracing is not enabled");
#endif

    for (Layer* layer : layerstack) {
        if (layer) layer->onStartup();
    }

#ifdef AFTER_HOURS_ENABLE_MCP
    init_mcp_server();
#endif

    running = true;
    while (running && !raylib::WindowShouldClose()) {
        float dt = raylib::GetFrameTime();
        this->loop(dt);
    }

#ifdef AFTER_HOURS_ENABLE_MCP
    if (MCP_ENABLED) {
        afterhours::mcp::shutdown();
    }
#endif

    simulated_input::stop();
    raylib::CloseWindow();
}

void App::loop(float dt) {
    TRACY_ZONE_SCOPED;

#ifdef AFTER_HOURS_ENABLE_MCP
    if (MCP_ENABLED) {
        afterhours::mcp::update();
        input_injector::update_key_hold(dt);
    }
#endif

    for (Layer* layer : layerstack) {
        if (layer) layer->onUpdate(dt);
    }

    // Replay recorded input and handle bypass injections/holds
    simulated_input::update(dt);

    draw_all_to_texture(dt);
    render_to_screen();

    // Check Input
    {
        KeyMap::get().forEachInputInMap(
            std::bind(&App::processEvent, this, std::placeholders::_1));

        KeyMap::get().forEachCharTyped(
            std::bind(&App::processEvent, this, std::placeholders::_1));

        MouseMap::get().forEachMouseInput(
            std::bind(&App::processEvent, this, std::placeholders::_1));
    }

#ifdef AFTER_HOURS_ENABLE_MCP
    if (MCP_ENABLED) {
        input_injector::release_scheduled_click();
    }
#endif

    TRACY_FRAME_MARK("app::loop");
}

void App::draw_all_to_texture(float dt) {
    TRACY_ZONE_SCOPED;

    raylib::BeginTextureMode(mainRT);
    for (Layer* layer : layerstack) {
        if (layer) layer->onDraw(dt);
    }
    raylib::EndTextureMode();
}

void App::render_to_screen() {
    TRACY_ZONE_SCOPED;

    raylib::BeginDrawing();
    {
        App::start_post_processing();
        {
            DrawTextureRec(mainRT.texture, {0, 0, width * 1.f, -height * 1.f},
                           {0, 0}, WHITE);
        }
        App::end_post_processing();
    }
    raylib::EndDrawing();
}

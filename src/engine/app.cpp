
#include "app.h"

#define ENABLE_TRACING 1
#include "tracy.h"
//
#include "keymap.h"
#include "raylib.h"
//

#include "log.h"
#include "settings.h"
#include "../libraries/shader_library.h"
#include "simulated_input/simulated_input.h"

// Layered input system
#include "../game_actions.h"
#include "../input_mapping_setup.h"
#include "../input_mapping_persistence.h"
#include "input_helper.h"

#ifdef AFTER_HOURS_ENABLE_MCP
#include <afterhours/src/plugins/mcp_server.h>

#include <condition_variable>

extern bool MCP_ENABLED;

namespace {
struct ScreenshotState {
  std::mutex mutex;
  std::condition_variable cv;
  bool requested = false;
  bool ready = false;
  std::vector<uint8_t> data;
};

ScreenshotState& screenshot_state() {
  static ScreenshotState state;
  return state;
}

std::vector<uint8_t> capture_screenshot_png_main_thread() {
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

std::vector<uint8_t> capture_screenshot_png() {
  auto& state = screenshot_state();
  std::unique_lock<std::mutex> lock(state.mutex);
  state.requested = true;
  state.ready = false;
  state.data.clear();
  state.cv.notify_all();

  if (!state.cv.wait_for(lock, std::chrono::milliseconds(2000),
                         [&state] { return state.ready; })) {
    state.requested = false;
    return {};
  }
  return state.data;
}

void process_screenshot_request_if_needed() {
  auto& state = screenshot_state();
  std::unique_lock<std::mutex> lock(state.mutex);
  if (!state.requested || state.ready) {
    return;
  }
  lock.unlock();

  std::vector<uint8_t> data = capture_screenshot_png_main_thread();

  lock.lock();
  state.data = std::move(data);
  state.ready = true;
  state.requested = false;
  lock.unlock();
  state.cv.notify_all();
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

    // Check for resolution changes via window_manager component
    {
        auto current_resolution = afterhours::window_manager::fetch_current_resolution();
        if (!(current_resolution == last_resolution)) {
            log_trace("Resolution changed from {}x{} to {}x{}",
                     last_resolution.width, last_resolution.height,
                     current_resolution.width, current_resolution.height);

            width = current_resolution.width;
            height = current_resolution.height;
            last_resolution = current_resolution;

            __WIN_W = width;
            __WIN_H = height;

            UnloadRenderTexture(mainRT);
            mainRT = raylib::LoadRenderTexture(width, height);
        }
    }

#ifdef AFTER_HOURS_ENABLE_MCP
    if (MCP_ENABLED) {
        afterhours::mcp::update();
        input_injector::update_key_hold(dt);
    }
#endif

    // Poll input for all states (menu and game)
    input_helper::poll(dt);

    for (Layer* layer : layerstack) {
        if (layer) layer->onUpdate(dt);
    }

    // Replay recorded input and handle bypass injections/holds
    simulated_input::update(dt);

    draw_all_to_texture(dt);
#ifdef AFTER_HOURS_ENABLE_MCP
    if (MCP_ENABLED) {
        process_screenshot_request_if_needed();
    }
#endif
    render_to_screen();

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

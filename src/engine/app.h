
#pragma once

#include <array>
#include <optional>

#include "../../vendor/afterhours/src/backends/raylib/headless.h"
#include "../../vendor/afterhours/src/frame_rate_stats.h"
#include "files.h"
#include "layer.h"
#include "singleton.h"

struct App {
    struct AppSettings {
        int fps;
        int width;
        int height;
        const char* title;
        raylib::TraceLogLevel logLevel = raylib::LOG_ERROR;
        bool headless = false;
    };

    static void create(const AppSettings& pt);
    [[nodiscard]] static App& get();

    explicit App(const AppSettings&);
    ~App();

    void run();
    void close();

    void note_frame(float dt);
    [[nodiscard]] bool frame_stats_ready() const {
        return frame_stats.ready();
    }
    [[nodiscard]] float avg_fps() const { return frame_stats.fps(); }
    [[nodiscard]] float avg_frame_ms() const {
        return frame_stats.avg_frame_ms();
    }
    [[nodiscard]] float max_frame_ms() const {
        return frame_stats.max_frame_ms();
    }
    [[nodiscard]] std::optional<float> p99_frame_ms() const;

    void loadLayers(const std::vector<Layer*>& layers);

   private:
    void update(float dt);
    void draw(float dt);

    std::array<Layer*, 32> layerstack;
    int max_layer = -1;

    // TODO create a render texture library?
    raylib::RenderTexture2D mainRT;

   public:
    [[nodiscard]] raylib::Texture2D get_main_texture() const {
        return mainRT.texture;
    }

   private:
    bool running = false;
    int width;
    int height;

    afterhours::FrameRateStats frame_stats{1.f};
    std::array<float, 1024> frame_times{};
    int frame_time_count = 0;
    int frame_time_next = 0;
    bool use_headless = false;
    afterhours::graphics::RaylibHeadless headless_backend;

    // Track last known resolution to detect changes
    afterhours::window_manager::Resolution last_resolution = {0, 0};

    App() {}
    // disable copying
    App(const App&);
    App& operator=(const App&);
    static bool created;
    static App instance;

    void draw_all_to_texture(float dt);
    void render_to_screen();

    static void start_post_processing();
    static void end_post_processing();

    /*
    static void start_remove_invisible();
    static void end_remove_invisible();
    */
};

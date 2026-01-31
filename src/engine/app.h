
#pragma once

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
    };

    static void create(const AppSettings& pt);
    [[nodiscard]] static App& get();

    explicit App(const AppSettings&);
    ~App();

    void run();
    void close();

    void loadLayers(const std::vector<Layer*>& layers);

   private:
    void loop(float dt);

    std::array<Layer*, 32> layerstack;
    int max_layer = -1;

    // TODO create a render texture library?
    raylib::RenderTexture2D mainRT;

   public:
    [[nodiscard]] raylib::Texture2D get_main_texture() const { return mainRT.texture; }

   private:

    bool running = false;
    int width;
    int height;

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


#pragma once

#include "event.h"
#include "files.h"
#include "layer.h"
#include "singleton.h"

struct AppSettings {
    int fps;
    int width;
    int height;
    const char* title;
    raylib::TraceLogLevel logLevel = raylib::LOG_ERROR;
};

struct App {
    static void create(const AppSettings& pt);
    [[nodiscard]] static App& get();

    bool running = false;
    LayerStack layerstack;
    int width;
    int height;

    int prev_width = -1;
    int prev_height = -1;

    // TODO create a render texture library?
    raylib::RenderTexture2D mainRT;

    explicit App(const AppSettings&);
    ~App();

    template<size_t N>
    void pushAllLayers(const std::array<Layer*, N>& layers) {
        layerstack.pushAllLayers<N>(layers);
    }
    void pushLayer(Layer* layer);
    void pushOverlay(Layer* layer);
    void onEvent(Event& event);
    bool onWindowResize(WindowResizeEvent event);
    bool onWindowFullscreen(WindowFullscreenEvent& event);
    void processEvent(Event& e);
    void close();
    void run();
    void loop(float dt);

   private:
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

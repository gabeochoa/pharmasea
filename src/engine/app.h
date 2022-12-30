
#pragma once

#include "files.h"
#include "layer.h"
#include "singleton.h"

struct AppSettings {
    int fps;
    int width;
    int height;
    const char* title;
    TraceLogLevel logLevel = LOG_ERROR;
};

SINGLETON_FWD(App)
struct App {
    SINGLETON_PARAM(App, AppSettings)

    bool running = false;
    LayerStack layerstack;
    int width;
    int height;

    // TODO create a render texture library?
    RenderTexture2D mainRT;

    explicit App(const AppSettings&);
    ~App();

    void pushLayer(Layer* layer);
    void pushOverlay(Layer* layer);
    void onEvent(Event& event);
    bool onWindowResize(WindowResizeEvent event);
    void processEvent(Event& e);
    void close();
    void run();
    void loop(float dt);

   private:
    void draw_all_to_texture(float dt);
    void start_post_processing();
    void end_post_processing();
    void render_to_screen();
};

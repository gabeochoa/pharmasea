
#include "app.h"

#include "settings.h"
#include "shader_library.h"

void App::start_post_processing() {
    if (!Settings::get().data.enable_postprocessing) return;
    auto post_processing_shader = ShaderLibrary::get().get("post_processing");
    BeginShaderMode(post_processing_shader);
}

void App::end_post_processing() {
    if (!Settings::get().data.enable_postprocessing) return;
    EndShaderMode();
}

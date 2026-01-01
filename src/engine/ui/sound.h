
#pragma once

#include "../../strings.h"
#include "../sound_library.h"

namespace ui {

namespace sounds {
inline void select() {
    SoundLibrary::get().play(
        strings::sounds::to_name(strings::sounds::SoundId::SELECT));
}
inline void click() {
    SoundLibrary::get().play(
        strings::sounds::to_name(strings::sounds::SoundId::CLICK));
}

}  // namespace sounds

}  // namespace ui

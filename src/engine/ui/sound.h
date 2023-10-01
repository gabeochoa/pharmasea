
#pragma once

#include "../../strings.h"
#include "../sound_library.h"

namespace ui {

namespace sounds {
inline void select() { SoundLibrary::get().play(strings::sounds::SELECT); }
inline void click() { SoundLibrary::get().play(strings::sounds::CLICK); }

}  // namespace sounds

}  // namespace ui

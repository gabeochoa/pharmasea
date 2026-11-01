

#pragma once

#include "bool_state.h"

using CanBeHeld = BoolState<struct HeldTag>;

struct CanBeHeld_HT : public CanBeHeld {};

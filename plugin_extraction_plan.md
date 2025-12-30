# Pharmasea Plugin Extraction Plan

## Overview
Extract reusable features from pharmasea into afterhours plugins. Focus on features that are:
- Reusable across multiple games
- Well-abstracted and not game-specific
- Would save significant developer time
- Follow afterhours plugin patterns

## Status (as of 2025-12-30)

- **Not started in this repo**
  - No plugin extraction work has landed into an `afterhours` codebase here (there is no `vendor/afterhours/` subtree, and no new `afterhours` plugin headers were added under `vendor/`).
  - The listed “Files to Extract” still exist in `src/` as game code, and would need to be moved/copied into the actual Afterhours repository (or added here as an explicit dependency/submodule) to begin extraction.

- **Related refactors that *have* happened (but are not extraction)**
  - Map generation was refactored internally into a pipeline (`src/map_generation/pipeline.*`) with a documented playability spec (`docs/map_playability_spec.md`).
  - Day/night transitions were refactored into dedicated systems (`src/system/afterhours_day_night_transition_systems.cpp`).

## Key Findings: Afterhours Current State

### What Afterhours Already Provides:
- ✅ **Library<T>** template - can be used for any resource type (sounds, music, fonts, etc.)
- ✅ **font_helper.h** - font loading with CJK support (`load_font_from_file_with_codepoints`)
- ✅ **text_utils.h** - CJK text detection utilities
- ✅ **HasLabel** component - for UI text (but stores string directly, no translation)
- ✅ Plugin infrastructure - `developer::Plugin` base, SystemManager registration

### What Afterhours Does NOT Provide:
- ❌ **Translation/i18n system** - no .po support, no language switching, no translation keys
- ❌ **Audio plugin** - no sound/music playback, no volume management, no audio systems
- ❌ **Settings/Save system** - no settings management or serialization
- ❌ **State management** - no state manager with history/callbacks
- ❌ **Files/Resource management** - no platform-specific paths or resource iteration
- ❌ **Pathfinding** - no pathfinding algorithms or systems
- ❌ **Networking** - no multiplayer/networking support

## Priority 1: High-Value Plugins (Extract First)

### 1.1 Networking/Multiplayer Plugin
**Files**: `src/network/`, `src/network/shared.h`, `src/network/client.h`, `src/network/server.h`

**Current State**:
- Full client/server architecture using Steam GameNetworkingSockets
- Entity synchronization across network
- Packet serialization with bitsery
- Reliable/unreliable channels
- Client join/leave handling
- Host/client role management

**Plugin Design**:
- **Components**:
  - `HasNetworkRole` - tracks host/client/none
  - `HasClientId` - unique client identifier
  - `IsNetworkedEntity` - marks entities to sync
  - `NetworkSyncData` - component data to sync
- **Systems**:
  - `NetworkSystem` - handles connection management
  - `EntitySyncSystem` - syncs entities between host/client
  - `PacketProcessSystem` - processes incoming packets
- **API**:
  - `network::init()` - initialize networking
  - `network::start_host(port)` - start as host
  - `network::connect(ip, port)` - connect as client
  - `network::send_packet(packet)` - send custom packet
  - `network::register_packet_handler(type, handler)` - register packet handlers

**Benefits**: Networking is complex and time-consuming to implement. This would save weeks of development for multiplayer games.

**Dependencies**: Steam GameNetworkingSockets (optional, can work without Steam)

**Generalization Notes**:
- Abstract away Steam-specific code behind interface
- Make entity sync configurable (which components to sync)
- Support custom packet types
- Handle network errors gracefully

---

### 1.2 Localization/i18n Plugin
**Files**: `src/strings.h`, `src/translations/`, `src/preload.cpp` (language loading), `src/engine/font_library.h`

**Current State**:
- Translation system using .po files
- Language switching at runtime
- Font loading per language (CJK support)
- Enum-based translation keys
- Parameter substitution in translations

**Afterhours Current State**:
- ✅ Has `HasLabel` component for UI text (stores string directly)
- ✅ Has `font_helper.h` with CJK font loading utilities (`load_font_from_file_with_codepoints`)
- ✅ Has `text_utils.h` with CJK text detection
- ❌ **NO translation/localization system** - no .po support, no language switching, no translation keys

**Plugin Design**:
- **Components**:
  - `HasLocalizedText` - entity with localizable text (extends or replaces `HasLabel` for translated text)
  - `ProvidesTranslation` - translation data provider
  - `ProvidesLanguage` - current language provider
- **Systems**:
  - `TranslationUpdateSystem` - updates text when language changes
  - `LocalizedTextRenderSystem` - renders localized text (integrates with UI rendering)
- **API**:
  - `i18n::load_language(lang_name, po_file)` - load translation file
  - `i18n::set_language(lang_name)` - switch language (triggers updates)
  - `i18n::translate(key, params...)` - translate with parameters
  - `i18n::register_font_loader(lang, font_loader_fn)` - register font per language
  - `i18n::get_font(lang)` - get font for language
  - `i18n::register_on_language_change(callback)` - callback when language changes

**Integration with Afterhours**:
- Use existing `afterhours::font_helper::load_font_from_file_with_codepoints()` for CJK fonts
- Extend `afterhours::ui::HasLabel` or create `HasLocalizedLabel` that wraps it
- Integrate with `afterhours::ui` rendering system
- Use `afterhours::Library<Font>` pattern for font management

**Benefits**: Localization is essential for many games but tedious to implement. Afterhours has font utilities but no translation system - this fills the gap.

**Dependencies**: .po file parser (mo library), magic_enum for enum keys

**Generalization Notes**:
- Support multiple translation backends (.po, JSON, etc.)
- Make font loading optional/configurable (use afterhours font_helper)
- Support pluralization rules
- Handle missing translations gracefully
- Integrate with afterhours UI system (HasLabel, rendering)

---

### 1.3 Audio Plugin
**Files**: `src/engine/sound_library.h`, `src/engine/music_library.h`

**Current State**:
- SoundLibrary and MusicLibrary with volume control
- Random sound matching by prefix
- Volume updates for all sounds/music
- Server-side sound filtering (network awareness)

**Afterhours Current State**:
- ✅ Has `Library<T>` template that can be used for any resource type
- ✅ Example in `library_usage/main.cpp` shows Library usage with Sound type (but just example, not plugin)
- ❌ **NO audio plugin** - no sound/music playback, no volume management, no audio systems
- ❌ No audio device initialization/cleanup
- ❌ No audio systems or components

**Plugin Design**:
- **Components**:
  - `HasSound` - entity that can play sounds
  - `HasMusic` - entity that can play music
  - `ProvidesAudioSettings` - master/music/sound volume
  - `IsPlayingSound` - marks entity currently playing sound
  - `IsPlayingMusic` - marks entity currently playing music
- **Systems**:
  - `SoundPlaySystem` - plays sounds on entities
  - `MusicPlaySystem` - plays music streams
  - `AudioVolumeSystem` - updates volumes when settings change
- **API**:
  - `audio::init()` - initialize audio device
  - `audio::shutdown()` - cleanup audio device
  - `audio::load_sound(name, filename)` - load sound (uses Library<Sound>)
  - `audio::load_music(name, filename)` - load music (uses Library<Music>)
  - `audio::play_sound(name)` - play sound immediately
  - `audio::play_music(name, loop)` - play music stream
  - `audio::stop_music()` - stop current music
  - `audio::set_master_volume(vol)` - set master volume
  - `audio::set_music_volume(vol)` - set music volume
  - `audio::set_sound_volume(vol)` - set sound volume
  - `audio::play_random_match(prefix)` - play random sound matching prefix
  - `audio::update()` - update music streams (call each frame)

**Integration with Afterhours**:
- Use `afterhours::Library<Sound>` and `afterhours::Library<Music>` for resource management
- Follow afterhours plugin pattern (implement `developer::Plugin`)
- Register systems via `SystemManager`

**Benefits**: Audio management is common across games. Afterhours has Library template but no audio plugin - this provides complete audio solution.

**Dependencies**: raylib audio (or abstract to audio backend)

**Generalization Notes**:
- Abstract audio backend (raylib, SDL, etc.) - start with raylib, make backend swappable
- Support 3D positional audio (optional)
- Handle audio device initialization/cleanup
- Support streaming for music
- Use afterhours Library pattern for sound/music storage

---

### 1.4 Settings/Save Plugin
**Files**: `src/engine/settings.h`, `src/engine/settings.cpp`, `src/engine/files.h`, `src/dataclass/settings.h`

**Current State**:
- Settings management with bitsery serialization
- Platform-specific save file paths (PlatformFolders)
- Settings persistence (resolution, volume, language, etc.)
- Settings refresh/update system

**Plugin Design**:
- **Components**:
  - `ProvidesSettings` - settings data provider
  - `HasSettingsData` - entity with settings data
- **Systems**:
  - `SettingsLoadSystem` - loads settings on startup
  - `SettingsSaveSystem` - saves settings on change
- **API**:
  - `settings::init(game_name, settings_file)` - initialize settings
  - `settings::load()` - load from disk
  - `settings::save()` - save to disk
  - `settings::get<T>(key)` - get setting value
  - `settings::set<T>(key, value)` - set setting value
  - `settings::register_on_change(key, callback)` - callback on change
  - `settings::get_save_path()` - get platform-specific save path

**Benefits**: Save/load is needed in most games. Provides platform-specific paths and serialization.

**Dependencies**: bitsery (or abstract serialization), PlatformFolders

**Generalization Notes**:
- Support multiple serialization backends
- Handle settings migration/versioning
- Support multiple save slots
- Validate settings on load

---

## Priority 2: Medium-Value Plugins

### 2.1 State Management Plugin
**Files**: `src/engine/statemanager.h`

**Current State**:
- StateManager2 template with history stack
- State change callbacks
- Previous state tracking
- State validation

**Plugin Design**:
- **Components**:
  - `HasState<T>` - entity with state
  - `ProvidesStateManager<T>` - state manager provider
- **Systems**:
  - `StateTransitionSystem` - handles state transitions
- **API**:
  - `state::create_manager<T>(default_state)` - create state manager
  - `state::set(state)` - set new state
  - `state::get()` - get current state
  - `state::previous()` - get previous state
  - `state::go_back()` - revert to previous state
  - `state::register_on_change(callback)` - callback on state change
  - `state::reset()` - reset to default

**Benefits**: State management is common (menu states, game states, etc.). Provides history and callbacks.

**Generalization Notes**:
- Support multiple state managers per game
- Handle state validation/guards
- Support state machines (not just linear history)

---

### 2.2 Files/Resource Management Plugin
**Files**: `src/engine/files.h`, `src/engine/files.cpp`

**Current State**:
- Platform-specific folder paths
- Resource path resolution
- Resource iteration (for_resources_in_group)
- Settings file path management

**Plugin Design**:
- **Components**:
  - `ProvidesResourcePaths` - resource path provider
- **API**:
  - `files::init(game_name)` - initialize file system
  - `files::get_resource_path(group, name)` - get resource path
  - `files::get_save_path()` - get save directory
  - `files::get_config_path()` - get config directory
  - `files::for_resources_in_group(group, callback)` - iterate resources
  - `files::ensure_directory_exists(path)` - create directory

**Benefits**: Platform-specific paths are tedious. Provides consistent resource management.

**Dependencies**: PlatformFolders, std::filesystem

**Generalization Notes**:
- Support resource packing/archives
- Handle resource hot-reloading (dev mode)
- Support multiple resource roots

---

### 2.3 Pathfinding Plugin
**Files**: `src/engine/path_request_manager.h`, `src/engine/bfs.h`, `src/engine/astar.h`, `src/components/can_pathfind.h`

**Current State**:
- A* and BFS pathfinding algorithms
- Async path request system (separate thread)
- Walkability checking
- Path following component

**Plugin Design**:
- **Components**:
  - `CanPathfind` - entity that can pathfind
  - `HasWalkabilityMap` - provides walkability data
- **Systems**:
  - `PathfindingRequestSystem` - queues path requests
  - `PathfindingResponseSystem` - processes path results
  - `PathFollowingSystem` - follows computed paths
- **API**:
  - `pathfinding::request_path(entity, start, end, callback)` - async path request
  - `pathfinding::find_path_sync(start, end, is_walkable)` - synchronous path
  - `pathfinding::register_walkability_checker(fn)` - register walkability
  - `pathfinding::set_algorithm(algorithm)` - A*, BFS, etc.

**Benefits**: Pathfinding is common in games. Provides async pathfinding with multiple algorithms.

**Generalization Notes**:
- Support different pathfinding algorithms (A*, BFS, Dijkstra, Theta*)
- Support 3D pathfinding
- Handle dynamic obstacles
- Support path smoothing

---

### 2.4 Timer/Trigger Plugin
**Files**: `src/engine/trigger_on_dt.h`

**Current State**:
- Simple timer that triggers after duration
- Resets automatically

**Plugin Design**:
- **Components**:
  - `HasTimer` - entity with timer
  - `HasCooldown` - entity with cooldown
- **Systems**:
  - `TimerUpdateSystem` - updates timers
  - `CooldownUpdateSystem` - updates cooldowns
- **API**:
  - `timer::create(duration)` - create timer
  - `timer::test(dt)` - test if timer triggered
  - `timer::reset()` - reset timer
  - `cooldown::is_ready()` - check if cooldown ready
  - `cooldown::start()` - start cooldown

**Benefits**: Timers/cooldowns are very common. Simple but saves boilerplate.

**Generalization Notes**:
- Support one-shot vs repeating timers
- Support timer callbacks
- Support pause/resume

---

## Priority 3: Lower Priority (Consider Later)

### 3.1 Prefab/Entity Maker Plugin
**Files**: `src/entity_makers.h`, `src/entity_makers.cpp`

**Current State**:
- Entity creation helpers
- Prefab-like entity builders

**Plugin Design**:
- **API**:
  - `prefab::create(name, components...)` - create from prefab
  - `prefab::register(name, builder_fn)` - register prefab
  - `prefab::load_from_json(file)` - load prefabs from JSON

**Benefits**: Reduces entity creation boilerplate.

**Note**: May be too game-specific. Evaluate if patterns are reusable.

---

### 3.2 Wave Function Collapse Plugin
**Files**: `src/wave_collapse.h`, `src/wave_collapse.cpp`

**Current State**:
- WFC algorithm for procedural generation
- Map generation

**Plugin Design**:
- **API**:
  - `wfc::generate(tileset, constraints, size)` - generate map
  - `wfc::register_tile(name, constraints)` - register tile

**Benefits**: Procedural generation is useful but niche.

**Note**: Very specialized. May not be worth extracting unless multiple games need it.

---

## Implementation Strategy

### Phase 1: Extract High-Value Plugins (Use Afterhours Where Possible)
1. **Audio Plugin** - Afterhours has Library template but no audio plugin. Extract pharmasea's audio system.
2. **Settings/Save Plugin** - Frequently needed, no afterhours equivalent
3. **State Management Plugin** - Common pattern, no afterhours equivalent
4. **Files/Resource Plugin** - Foundation for others, no afterhours equivalent

### Phase 2: Extract Complex Plugins
5. **Localization Plugin** - Afterhours has font utilities but no translation system. Extract pharmasea's i18n system and integrate with afterhours font_helper.
6. **Pathfinding Plugin** - Complex but reusable, no afterhours equivalent
7. **Networking Plugin** - Most complex, highest value, no afterhours equivalent

### Phase 3: Evaluate Lower Priority
8. Review Prefab and WFC plugins based on actual need

## Plugin Development Guidelines

### Plugin Structure
- Follow afterhours plugin pattern (see `developer.h`)
- Use `afterhours::developer::Plugin` base
- Register systems via `SystemManager`
- Use components for data, systems for logic

### Integration with Afterhours
- **Use afterhours Library<T>** for resource management (sounds, music, fonts)
- **Use afterhours font_helper** for font loading in i18n plugin
- **Extend afterhours HasLabel** or create HasLocalizedLabel for translation
- **Follow afterhours plugin pattern** - implement `developer::Plugin`, register systems
- **Integrate with afterhours UI** - work with existing UI rendering systems

### Abstraction Level
- Abstract away game-specific details
- Use callbacks/function objects for customization
- Support multiple backends where possible (audio, serialization)
- Make dependencies optional where possible

### Testing
- Create example projects for each plugin
- Test with different game types
- Ensure plugins work together

### Documentation
- Document API clearly
- Provide usage examples
- Document dependencies and requirements

## Files to Extract

**Priority 1**:
- `src/network/` → `vendor/afterhours/src/plugins/network.h`
- `src/strings.h`, `src/translations/` → `vendor/afterhours/src/plugins/i18n.h`
- `src/engine/sound_library.h`, `src/engine/music_library.h` → `vendor/afterhours/src/plugins/audio.h`
- `src/engine/settings.h`, `src/engine/files.h` → `vendor/afterhours/src/plugins/settings.h`

**Priority 2**:
- `src/engine/statemanager.h` → `vendor/afterhours/src/plugins/state_manager.h`
- `src/engine/files.h` → `vendor/afterhours/src/plugins/files.h`
- `src/engine/path_request_manager.h`, `src/engine/bfs.h`, `src/engine/astar.h` → `vendor/afterhours/src/plugins/pathfinding.h`
- `src/engine/trigger_on_dt.h` → `vendor/afterhours/src/plugins/timer.h`

## Dependencies to Consider

- **Steam GameNetworkingSockets** - for networking (optional)
- **PlatformFolders** - for platform-specific paths
- **bitsery** - for serialization (already in afterhours)
- **magic_enum** - for enum-based keys (already in afterhours)
- **.po file parser** - for translations
- **raylib audio** - for audio (or abstract)

## Success Criteria

- Plugin can be used in multiple games without modification
- Plugin follows afterhours patterns and conventions
- Plugin is well-documented with examples
- Plugin has minimal dependencies
- Plugin is tested and stable
- Plugin integrates well with existing afterhours features (Library, font_helper, UI)


# Generic AI Queue Interaction System

**Category:** System Consolidation
**Impact:** ~270 lines saved
**Risk:** Medium
**Complexity:** Medium

## Current State

4 systems with identical queue-based interaction patterns (~427 lines total):
- `AIQueueForRegisterSystem` (60 lines)
- `AIPaySystem` (95 lines)
- `AIBathroomSystem` (145 lines)
- `AIPlayJukeboxSystem` (127 lines)

All follow: find target → join queue → advance in line → interact at front → leave queue → transition state

## Discussion: Is this the right kind of consolidation?

**Questions to answer first:**

1. **How often do we add new queue-based AI behaviors?** If rarely, copy-paste cost is low.

2. **Have there been bugs in queue logic that needed fixing in multiple places?**

3. **Do these 4 systems share truly identical queue logic, or are there subtle differences?** (e.g., does bathroom have special cleanup that pay doesn't?)

4. **Preference: explicit separate systems vs. consolidated with indirection?**

**Arguments for keeping separate:**
- Each system is ~60-145 lines - not huge
- Explicit and self-contained, easier to debug
- No indirection through callbacks
- Can evolve independently if behaviors diverge
- The "shared" logic might not stay shared

**Arguments for consolidation:**
- DRY - single place to fix queue bugs
- Documents the pattern explicitly ("this is a queue interaction")
- Easier to add new queue-based behaviors consistently

## Possible Approaches

### Approach A: Keep separate, extract shared utilities

Don't merge the systems. Instead, extract common helper functions they all call:

```cpp
namespace ai_queue {
    bool try_join_queue(Entity& customer, Entity& target);
    bool advance_in_queue(Entity& customer, HasWaitingQueue& queue);
    void leave_queue(Entity& customer, HasWaitingQueue& queue);
}
```

Systems stay explicit, but queue manipulation is centralized.

### Approach B: Base class with virtual methods (OOP)

```cpp
struct AIQueueSystemBase : public System<IsAIControlled, CanPathfind> {
    virtual OptEntity findTarget(Entity& customer) = 0;
    virtual void onInteraction(Entity& customer, Entity& target) = 0;
    virtual IsAIControlled::State nextState() = 0;

    void process(Entity& customer, float dt) {
        // Shared queue logic calls virtuals
    }
};

struct AIPaySystem : public AIQueueSystemBase {
    OptEntity findTarget(Entity& customer) override { /* ... */ }
    void onInteraction(Entity& customer, Entity& target) override { /* ... */ }
    IsAIControlled::State nextState() override { return State::Leaving; }
};
```

### Approach C: Template with callbacks

```cpp
struct AIQueueInteractionConfig {
    IsAIControlled::State trigger_state;
    IsAIControlled::State next_state;
    std::function<OptEntity(Entity&)> find_target;
    std::function<void(Entity&, Entity&)> on_interaction;
};

struct AIQueueInteractionSystem : public System<IsAIControlled, CanPathfind> {
    AIQueueInteractionConfig config;
    // ...
};
```

### Approach D: Data-driven state machine

Define AI behaviors in config rather than code. Single system interprets config:

```cpp
// In JSON or code-as-data
AIBehavior bathroom_behavior = {
    .trigger_state = "NeedsBathroom",
    .target_type = "Toilet",
    .queue_based = true,
    .on_complete = "transition:Wandering"
};
```

### Approach E: Keep as-is

If the duplication isn't causing actual problems, maybe it's fine. ~427 lines for 4 distinct behaviors is ~107 lines each.

## Recommendation

TBD after discussion. Approach A (extract utilities) might be the sweet spot - reduces duplication without adding indirection complexity.

## Files Affected

- `src/system/ai/ai_queue_for_register_system.h`
- `src/system/ai/ai_pay_system.h`
- `src/system/ai/ai_bathroom_system.h`
- `src/system/ai/ai_play_jukebox_system.h`

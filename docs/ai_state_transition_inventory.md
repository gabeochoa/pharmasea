# AI State Transition Inventory (current behavior)

This doc is step 1 of the staged-transition refactor: it captures the **current** `IsAIControlled::state` transitions and the associated “scratch reset”/target cleanup that happens around them.

Source of truth while drafting: `src/system/ai_system.cpp` and day/night transition systems in `src/system/afterhours_day_night_transition_systems.cpp`.

## State transitions (inside `ai_system.cpp`)

### Wander (including “wander as pause”)

- **QueueForRegister → Wander**
  - **When**: no register available.
  - **How**: `wander_pause(entity, resume=QueueForRegister)` sets `resume_state` and sets `state=Wander`.
  - **Resets**:
    - clears `HasAITargetLocation`
    - `reset_component<HasAIWanderState>`

- **Pay → Wander**
  - **When**: no register available.
  - **How**: `wander_pause(entity, resume=Pay)` sets `resume_state` and sets `state=Wander`.
  - **Resets**:
    - clears `HasAITargetLocation`
    - `reset_component<HasAIWanderState>`

- **Wander → resume_state**
  - **When**: reached target and dwell timer completes.
  - **Resets**:
    - `tgt.pos.reset()` (clears `HasAITargetLocation::pos`)

### Bathroom (interrupt + return)

- **(Any except Bathroom/Drinking/Leave) → Bathroom**
  - **When**: global interrupt if `needs_bathroom_now(entity)` is true.
  - **How**: `enter_bathroom(entity, return_to=current_state)` sets `state=Bathroom` and stores `next_state` in `HasAIBathroomState::next_state`.
  - **Resets**:
    - clears `HasAITargetEntity`
    - clears `HasAITargetLocation`
    - `reset_component<HasAIBathroomState>`

- **Bathroom → Wander**
  - **When**: missing `CanOrderDrink` (safety fallback).
  - **Resets**: none specific beyond state change.

- **Bathroom → bs.next_state**
  - **When**: bladder no longer full (early exit) OR toilet use finishes (normal exit).
  - **Resets**:
    - `reset_component<HasAIBathroomState>`
    - also clears `HasAITargetEntity` during exit path (`tgt.entity.clear()`)

### Register / ordering loop

- **QueueForRegister → AtRegisterWaitForDrink**
  - **When**: reached front of the register line.
  - **Side effects**:
    - speech bubble on, patience enabled

- **AtRegisterWaitForDrink → QueueForRegister**
  - **When**: target register becomes invalid.
  - **Resets**:
    - `tgt.entity.clear()`

- **AtRegisterWaitForDrink → Drinking**
  - **When**: drink served and validated; customer takes drink and leaves line.
  - **Resets**:
    - `tgt.entity.clear()`
    - `reset_component<HasAIQueueState>`
    - clears `HasAITargetLocation` (comment: queue movement used it)
    - `reset_component<HasAIDrinkState>`
    - speech bubble off, patience disabled+reset

### Drinking / pay / jukebox

- **Drinking → Pay**
  - **When**: drink timer completes and customer does not want another.
  - **Resets**:
    - clears `HasAITargetLocation::pos` via `tgt.pos.reset()`

- **Drinking → PlayJukebox**
  - **When**: wants another, has jukebox ability, random decision.
  - **Resets**:
    - clears `HasAITargetLocation::pos` via `tgt.pos.reset()`

- **Drinking → QueueForRegister**
  - **When**: wants another and not jukebox path.
  - **Resets**:
    - clears `HasAITargetLocation::pos` via `tgt.pos.reset()`
    - `set_new_customer_order(...)` also:
      - clears `HasAITargetLocation`
      - `reset_component<HasAIDrinkState>`

- **PlayJukebox → QueueForRegister**
  - **When**: no jukebox found / already last interacted / finished jukebox action.
  - **Resets**:
    - `reset_component<HasAIJukeboxState>` (in some early exits)
    - `tgt.entity.clear()`
    - `reset_component<HasAIJukeboxState>` (normal finish)

- **Pay → Leave**
  - **When**: payment processing finishes.
  - **Resets**:
    - `tgt.entity.clear()`
    - `reset_component<HasAIPayState>`

## External “force leave” transitions (day/night transition)

In `src/system/afterhours_day_night_transition_systems.cpp`:

- **Customer → Leave**
  - **When**: bar is closing / transition (`TellCustomersToLeaveSystem`).
  - **Side effects**:
    - resets `CanPathfind` by removing/re-adding it

## Wander fallback list (today)

States that currently transition to Wander as a fallback:

- `QueueForRegister` → `Wander` (no register available)
- `Pay` → `Wander` (no register available)
- `Bathroom` → `Wander` (missing `CanOrderDrink` safety)


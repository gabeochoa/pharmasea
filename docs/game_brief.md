# Game Brief: Pub Panic!

## One-Sentence Pitch
"Overcooked but you can eventually automate it."

## Core Game Concept
Pub Panic! is a **Systemic Volume Roguelite** where players manage a high-energy urban bar. Success is measured by maximum throughput and meeting the weekly financial targets (The Rent), rather than perfect service.

## The Loop
1. **The Day (Manual Scrambling)**: Players frantically serve drinks. High-skill manual actions (Rhythm Mixing) allow for "overclocking" machines and triggering map-wide Frenzy boosts.
2. **The Night (Strategic Upgrading)**: Use daily profits to buy new furniture, ingredients, and **Automation Modules**.
3.- **The Endless Loop**: Transition from manual labor (Day 1) to a fully automated "Factorio" style machine (Day 30+). The game continues until you can no longer pay the mounting rent.

---

## 🏛️ Core Design Pillars

### 1. Physicality Over UI
- **Diagetic Information**: Minimal screen-space UI. Character attributes like "Bladder Size" or "Patience" are reflected in animations or speech bubbles, not health bars.
- **Physical Interactions**: The "Rhythm Mixing" game is a series of floor tiles that light up in-world. Automations like Squirters or Grabbers are high-detail mechanical models.

### 2. High-Throughput Chaos (The "Back-up" Rule)
- While mechanics are **deterministic** (logic-consistent), complexity brings risk.
- **The Jam**: If a belt isn't emptied fast enough, items don't just "stop"—they begin to pile up or spill onto the floor, creating a slip hazard (Vomit-logic cleanup required).

### 3. Co-op Scaling
- **Dynamic Challenge**: Rent and Customer Spawn Rates scale linearly with player count.
    - `Target Rent = Base Rent * (0.5 + 0.5 * Number of Players)`
    - `Spawn Rate = Base Rate * (0.8 + 0.2 * Number of Players)`

---
## Key Features & Mechanics

### 🏗️ Logistics & Automation (The "Factorio" Layer)
The core of the late-game experience is automating the chaotic manual loop into a high-throughput machine.
- **Conveyor Belts**: Primary transport for items; can be used to route cups from dispensers to filling stations.
- **Filtered Grabbers**: Intelligent arms that extract items based on type (e.g., only grab "Dirty Cups" or "Full Pitchers").
- **Pneumatic Pipes**: High-speed, instant teleportation for specialized high-volume routing.
- **Squirters & Dispensers**: Fixed alcohol/soda cabinets that auto-fill cups when placed underneath or delivered via conveyor.
- **Tractor Beams**: Magnetic extraction for pulling items toward specific "loading zones."
- **Auto-Cleaning**: 
    - **Mop & Holder**: Basic manual spill cleanup.
    - **Roomba (MopBuddy)**: Automated cleaning bot that patrols for spills/vomit.
    - **Bidet**: Bathroom upgrade that increases the number of uses before cleaning is required, reducing overall mess frequency.
- **Handtruck**: Specialized tool for moving multiple boxes or large furniture items simultaneously, critical for during-round layout adjustments.

### 🍸 Bar & Service Mechanics
- **Pouring & Mixing**: 
    - **Manual**: Timer-based barrels (Beer), Spouts (Soda), and Blender (Rhythm-based tapping).
    - **Automated**: Juice Press (auto-juicing) and Beer Wall (self-serve for customers).
- **Rhythm Mixing (Vision)**: A diagetic "Big Piano" or DDR-style minigame area. Players stand in the trigger zone to play a rhythm game (once per round). 
    - **Reward**: High scores trigger map-wide "Overclocking" or significant discounts on the next purchase.
- **Advanced Recipes**:
    - **Pitchers**: Can hold up to 10x the same drink, allowing for group serving.
    - **Flights**: Holds 4 unique liquids for high-end craft beer orders.
    - **Champagne**: Requires a sword-swinging QTE to open; perfect timing yields higher tips.

### 👥 Customer AI & Systems
Customers possess varying attributes that dictate the bar's "vibe" and challenge level:
- **The Loop**: Entrance -> Queue -> Order -> Drink/Wander -> Bathroom (if "Break the Seal" is active) -> Pay.
- **Attributes**: 
    - **Thieves**: Targets bottles and high-value items; identified by specific visuals and countered by **Metal Detectors**.
    - **Loud/Rude**: Giant speech bubbles and faster patience decay.
    - **SpeedWalkers**: 1.5x walk speed, increasing foot traffic chaos.
    - **Chatty Drinkers/Customers**: Double the time taken to order or pay at the register.
- **Bladder System**: Bladder size is hidden initially; "Break the Seal" halves bladder size after the first use, creating a "bathroom rush."

### 📈 Roguelite Upgrades & Progression
- **The Rent**: A weekly (multi-round) mounting financial target. Failure ends the run unless the **Credit Line** upgrade is active.
- **The Locker**: Persistent storage vault allowing players to save high-value items or one drink between rounds.
- **Scaling Upgrades**:
    - **Happy Hour**: 2x customer spawns, 0.75x drink cost. Occurs at mid-day.
    - **Dynamic Prices**: Drink costs scale linearly with the length of the queue.
    - **Membership Cards**: Significant upfront cash injection in exchange for a higher minimum customer floor.
    - **$10k Beer**: 0.5x customer spawn rate, 2x drink cost (Exclusive "Luxury" theme).
    - **Speakeasy**: Tip multiplier is calculated based on the maximum pathfinding distance from entrance to order point.

### 🎭 Seasonal & Themed Modifiers
- **Themed Nights**:
    - **Karaoke**: Customers order the *same* drink as the person before them in line.
    - **Live Music**: Increased re-order frequency, but the band consumes physical floor space and blocks paths.

---

## Aesthetic & Vibe
Urban, chaotic, and energetic. A realistic city bar reflected through a systemic lens. High-volume, high-stress, high-reward.

---

##  Economy & Strategy Math
The game's economy is designed for high-volume systemic scaling.

- **Drink Pricing**: Calculated dynamically based on complexity.
    - `Base Price = (Number of Ingredients * 5) + (Number of Prereqs * 10)`
- **Tipping Logic**: Tips are highly dependent on speed of service.
    - `Tip = CEIL(Drink Price * 0.8 * Patience Percentage)`
- **Speakeasy Multiplier**: If the "Speakeasy" upgrade is active, prices scale based on pathfinding distance.

---

## 📈 Upgrade Manifest (Technical Specs)
All upgrades permanently modify the global `ConfigData` state.

| Upgrade | Direct Effect | Economic/Game Modification |
| :--- | :--- | :--- |
| **Longer Day** | `RoundLength` x2.0 | Doubles time available to earn rent. |
| **Unlock Toilet** | `MaxNumOrders` x2.0 | Unlocks `is_toilet.h` loop; required for "Big Bladders." |
| **Big Bladders** | `PissTimer` x2.0, `BladderSize` x2 | Customers stay in bathroom longer but go less often. |
| **Bidet** | Mess Reduction | Increases uses before cleaning required. |
| **Main Street** | `CustomerSpawn` x2.0 | Massive volume increase; high pressure. |
| **Break the Seal** | `BladderSize` / 2 after 1st visit | Higher bathroom frequency as the night goes on. |
| **Speakeasy** | Tip % scales with distance | Encourages long, winding bar layouts. |
| **Membership Card** | Flat cash upfront | Increases `MinCustomerSpawn` for 3 days. |
| **$10k Beer** | `CustomerSpawn` x0.25, `DrinkCost` x2 | Extreme low-volume, high-margin luxury niche. |
| **Utility Billing** | Machine Cost x0.75 | Machines are 25% cheaper but incur daily operational costs. |
| **Credit Line** | Enable Debt Recovery | Missing rent triggers the **Loan Shark** instead of Game Over. |
| **Automatic Maker** | Automation Tier 3 | Station builds recipes from raw ingredients. |
| **Vacuum Sealer** | Inventory Persistence | Save 1 drink across day transitions. |
| **Priority Register** | Intelligent Routing | Special register that only accepts specific drink types. |
| **VIP Red Carpet** | Target Marketing | Customers on rug pay 3x but only order most complex drinks. |
| **Flash Sale Board** | Targeted Volume | Place a finished drink here to boost that order frequency +50%. |
| **Ingredient Silos** | (Maybe) Bulk Storage | Connected via pipes; removes manual refilling. |
| **Fragrance Diffuser** | (Maybe) Patience Buff | Slower patience decay / Faster bladder fill. |

---

## � Recipe Bible
Drinks are loaded from `drinks.json` and require specific ingredients and equipment prereqs.

- **Standard Pours**:
    - **Cola**: Soda (1 ing) -> $5
    - **Beer**: Beer (1 ing) -> $5
- **Mixed Drinks (Base + Alcohol)**:
    - **Rum & Coke**: Soda + Rum (2 ings) -> $10
    - **Screwdriver**: Orange Juice + Vodka (2 ings + Orange prereq) -> $20
- **Advanced Cocktails (3+ Ingredients)**:
    - **Margarita**: Tequila + Lime Juice + Triple Sec (3 ings + Lime prereq) -> $25
    - **Cosmo**: Vodka + Cranberry Juice + Lime Juice + Triple Sec (4 ings + Lime/Cranberry prereq) -> $40
    - **Mojito**: Rum + Lime Juice + Soda + Simple Syrup (4 ings + Lime prereq) -> $30
- **High-Volume Specialist**:
    - **Beer Pitcher**: Beer (10x volume) -> $50 (Requires Archer Cupboard)
- **Luxury Service**:
    - **Champagne**: Champagne (1 ing) -> $5 (requires saber QTE for higher tips)

---

## 🔒 Feature Completion Checklist
This list tracks the implementation status of all "Locked" features.

### 🏗️ Logistics
- [x] **Conveyor Belts** (`conveys_held_item.h`)
- [x] **Filtered Grabbers** (`indexer.h`)
- [x] **Pneumatic Pipes** (`is_pnumatic_pipe.h`)
- [x] **Squirters** (`is_squirter.h`)
- [ ] **Automatic Drink Maker** (Recipe-fed station)
- [ ] **Vacuum Sealer** (Persistence slot)
- [ ] Tractor Beams (Magnetic loading zones)
- [ ] Auto-Cleaning Suite
    - [x] **Roomba (MopBuddy)** (`ai_clean_vomit.h`)
    - [ ] Bidet (Mess-reduction capacity upgrade)
- [x] **Handtruck** (`can_hold_handtruck.h`)

### 🍸 Bar & Service
- [ ] Rhythm Mixing System (Diagetic Trigger + "Overclock")
- [ ] Advanced Recipe Support (Flights, Sword-Saber QTE)
- [ ] **Red Carpet / VIP Area** (Most expensive drink orders only)
- [ ] **Priority Register** (Specific drink-type filtering)
- [ ] **Flash Sale Chalkboard** (Drink-priority display)
- [x] **Recipe Book System** (`recipe_library.h`)
- [x] **Machine Suite** (Juice Press, Soda Fountain)

### 👥 AI & Systems
- [x] **Core Customer AI** (Drink -> Wander -> Bathroom -> Pay)
- [x] **Attribute System** (Loud, Rude, Drunk, SpeedWalker)
- [ ] **Thief AI** (Item Snatching logic)
- [ ] **Bladder System** (Break the Seal hidden state)
- [ ] **Metal Detectors** (Thief detection furniture)
- [ ] **Fire Code Logic** (Max capacity / wait-outside queue)
- [ ] **Fragrance Diffuser** (Patience/Bladder scaling system)

### 📈 Upgrades & Modifiers
- [ ] **Scaling Logic** (Happy Hour, Dynamic Pricing)
- [x] **Themed Nights** (Live Music / `ai_play_jukebox.h`)
- [ ] Karaoke (Order mimicry loop)
- [x] **Persistence** (`is_progression_manager.h` / The Locker)

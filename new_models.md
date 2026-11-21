Ramped up on Pharmasea too — same KayKit audit, very different answer, because this game is modern where Cartographer is Age-of-Exploration.

**Pub Panic!:** multiplayer co-op "Overcooked but you can eventually automate it" — run a high-energy urban bar, serve drinks manually by day, buy furniture/upgrades/automation (conveyors, grabbers, pipes, squirters, MopBuddy) by night, pay mounting weekly rent. Customers queue → order → drink/wander → bathroom → pay, with patience/bladder/vomit systems. Planned: tea/coffee, pitchers, champagne-saber QTE, frozen drinks, VIP/red carpet, Try This display, live music, themed nights, DDR piano.

## Baseline

- **KayKit already in:** Dungeon bottles only (the 8 alcohol types in `models.json`) + 4 mini characters (`character_bear/dog/duck/rogue`). Everything else is Kenney `.obj`, much of it joke placeholders.
- **Placeholder models KayKit can actually replace:** `guitar` = lollipop → Mixed Bag `guitar_A/B`; `hand_truck` = chair → Mixed Bag `toolcart`; `mop_buddy` = carved pumpkin → Mystery 4 `Robot_One/Two`; tables/chairs/cupboard/register → Restaurant/Furniture/Dungeon (see below).
- **KayKit cannot fix:** automation machines — no conveyor, grabber arm, pneumatic pipe, or squirter exists in any pack. Those stay Kenney/custom. Same for drink glasses (Kenney `sodaGlass` reused for ~10 drinks), toilet, and mop.
- **Pipeline:** furniture goes `gltf/glb → scripts/convert_gltf_to_obj.py → resources/models/converted/*.obj → models.json` entry (`library_name`, scale, offset). Caveat: that converter exports **first mesh only** — verify multi-part models (decorated tables, etc.) before trusting them. Characters load as `.gltf` directly, and there is **no skeletal animation** — new characters would be static like the current ones.

## Definitely add

**Restaurant Bits — the core pack for this game (225 files)**
- Bar/kitchen machines: `fridge_A/B`, `oven`, `stove_*`, `icecream_machine` (soda/ice machine candidates), `pizza_oven`
- Service: `kitchentable_*`, `table_round_*`, `chair_A/B`, `chair_stool`, `menu` (Try This / Flash Sale display!), `wall_orderwindow`, `wall_doorway`, `wall_window_*`, `pillar_*`, `door_A/B`
- Ingredients/storage: `crate_*` (buns, carrots, cheese, dough, ham, lettuce, mushrooms, onions, pepperoni, potatoes, steak, tomatoes), `jar_*`, `pot_A/B`, `pan_A/B`, `plate`, `bowl`, `cuttingboard`, `rollingpin`, `knife`, `spoon`, `dishrack`
- Drinks/food: `food_burger`, `food_dinner`, `food_stew`, `food_pizza_*`, `food_icecream_*`, `food_vegetableburger`

**Furniture Bits — lounge / VIP / storage**
- `couch`, `armchair` (+ pillows), `table_low/medium`, `chair_A/B/C`, `chair_stool`
- `shelf_A/B_*`, `cabinet_small/medium`, `desk`, `desk_large`, `chair_desk_*`, `lamp_desk/standing/table`
- `rug_rectangle/oval_*` (VIP area flooring — red-carpet substitute), `pictureframe_*`, `mug_A/B`, `cup`, `cup_pencils`, `book_set`
- Skip: `monitor`, `keyboard`, `mouse`, `gameconsole_handheld` (unless the POS/register idea wants them)

**Dungeon — tavern bar core**
- `bar_straight_*`, `bartop_A/B_*`, `barrel_*`, `keg`, `barrel_small_stack`
- `table_long/medium/round_*`, `stool`, `bench`, `chair`, `chest_*`, `bookcase_*`, `shelf_*`
- `candle_*`, `plate_*`, `bottle_*` (already the alcohol model source), `stairs_*`, wall/floor sets

**Mixed Bag 1**
- `guitar_A/B` (replaces lollipop guitar), `slushy_machine_*` + `slushy_*` (frozen drinks), `arcademachine_A/B` (pub arcade), `gumball_machine`, `toolcart` / `toolcart_tall` (replaces chair hand truck), `cup`, `cups_stacked`, `fire_extinguisher`, `umbrella_*`, `waterbottle_*`

**Board Game Bits — pub table games**
- `chess_*` full set, `checker_*`, `card_*` (full deck), `chip_*`, `D6_*`, `D20/D8/D4`, `coin_*`, `board_A/B`

**City Builder Bits — outside the pub**
- `building_A–H` (+ `withoutBase`), `road_*`, `park_road_*`, `park_wall_*`, `streetlight`, `streetlight_old_*`, `car_*`, `dumpster`, `firehydrant`, `trash_*`, `bench`, `tree_*` — street front, delivery zone, Big City / Small Town themes. Modern = correct here.

## Maybe add

- **Characters (.glb) — customer/player variety, static only.** Adventurers (Knight, Mage, Ranger, Rogue, Druid, Barbarian), Mystery 5 (Witch, Vampire, Black Knight, Hiker, Helpers, Caveman), Mystery 6 (Farmers, Cleric, Lorekeeper, Hoarder, Plant Warrior), Mystery 4 (Survivalist, Paladin, Ninja, Space Ranger). Best fits: themed-night customers and Regulars (one per drink). Need `.glb → .gltf`/OBJ conversion + `character_models` registry entries. Skeletons → Halloween night only.
- **Holiday / Halloween — themed nights only.** Holiday: `christmas_tree`, `present_*`, `snowman_*`, `train_*`, `wreath`, `hot_chocolate`. Halloween: `pumpkin_*`, `coffin`, `scarecrow`, `grave_*`, `candy_*`, `shrine`, `wagon`.
- **Fantasy Weapons** — `sword_*` as the champagne-saber QTE prop + wall dressing. No other combat use.
- **Resource Bits — stocking/delivery dressing.** `Containers_Crate_*`, `Food_Barrel/Basket/Crate_*`, `Wood_Log/Plank_*`, `Stone_Brick*`, `Textiles_*`, `Gems_Chest/Sack`. Skip fuel jerrycans, money bills, plastic pallets. (Note: Generic Stocking is on the probably-not list, so this is décor, not a system.)
- **RPG Tools — small picks.** `blueprint` (planning), `journal_closed/open` (recipe book prop), `key_*` / `lock_*` (Locker), `tongs`, `knife`, `scissors`. Skip the workshop tools.
- **Prototype Bits** — `Primitive_*` greybox for prototyping new furniture before final art.

## Consider add

- **Forest Nature** — outdoor beer garden only: `Tree_*`, `Bush_*`, `Rock_*`, `Grass_*`, `Hill_*`. No outdoor area in the current design, so this is speculative.
- **Medieval Hexagon** — only if a medieval-tavern theme night happens; otherwise style-clashes with the urban bar. Its `boat`/`ship`/`castle` have no use here.
- **Character Animations** — no skeletal-animation support, so `Rig_*` files are unusable until that system exists.
- **Platformer / Block Bits / Space Base** — skip. No platforming, voxel, or sci-fi theme; Platformer `signage_*` is the only marginal pick (store/menu signs).

**Skip:** Space Base, Platformer, Block Bits as final art; `Theft + Metal Detectors` and Generic Stocking stay cut per `docs/ideas/probably-not.md` — don't import assets for cut systems.

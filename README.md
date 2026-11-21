# pharmasea (Pub Panic!)

Code name: **PharmaSea**. In-game title string: **Pub Panic!** (`src/strings.h`).

## Build

- **macOS / Linux**: `make` (repo root `makefile`, lowercase)
  - Output binary: `./pharmasea.exe`
- **Windows**: open `WinPharmaSea/PharmaSea.sln` in Visual Studio

## Save files

The game uses `sago/platform_folders` and stores data under the OS “Save Games” folder:

- **Settings**: `<SaveGamesFolder1>/pharmasea/settings.bin`
- **Save slots**: `<SaveGamesFolder1>/pharmasea/saves/slot_XX.bin`

If you’re unsure what your platform resolves to, run the game once and check the startup logs (it prints `intro: path=...` and `intro: game_folder=...`).

libraries used:
- raylib
- Raylib Operator Overloads https://github.com/ProfJski/RaylibOpOverloads
    - Added a #define that removes all the raygui stuff 
    - Made every function inline since i globally include and the linker not happy with so many definitions (odr?)
- Raylib Extras https://github.com/JeffM2501/raylibExtras 
- AppData stuff https://github.com/sago007/PlatformFolders
- Steam's GameNetworkingSockets https://github.com/ValveSoftware/GameNetworkingSockets
- HTTPRequest https://github.com/elnormous/HTTPRequest
- fmt https://github.com/fmtlib/fmt
- tl::expected https://github.com/TartanLlama/expected
- magic enum https://github.com/Neargye/magic_enum.git
- tracy https://github.com/wolfpld/tracy
- backward https://github.com/bombela/backward-cpp
- argh (for dev flags) https://github.com/adishavit/argh
- mo for translations http://number-none.com/blow/code/mo_file/index.html
- nlomann json for JSON reading / writing https://github.com/nlohmann/json
- reasings (some easing functions) https://github.com/raylib-extras/reasings
- PCG Random (faster more repreducible random) https://www.pcg-random.org/ 
- save file and network serialization https://github.com/eyalz800/zpp_bits 

Models from 
- https://www.kenney.nl/assets?q=3d 
- https://kaylousberg.itch.io/kay-kit-mini-game-variety-pack 


Font
- https://m.blog.naver.com/bibimseol/222303267518 


Save Files get stored to: 
- Mac: "/Users/<user>/Library/Application Support/pharmasea/settings.bin"
- Windows: My Documents / My Games / Pharmasea/settings.bin


Info about the todo chart
- full raw data is in todo.md and managed by obsidian
- hide the "backlog" here but trust me theres lots more todo 
- only show the most recent 10 done items
- to regenerate the table below run `python3 scripts/kanban.py` from the repo root (it rewrites this README between `## TODOs` and `## End TODO`)

## TODOs  

|design decisions|broke|
|----------------|-----|
|hard to tell that a new machine/stockpile has been spawned in after you get an upgrade|Client player cant change settings because menu::State is being overriden by host|
|Simple Syrup doesnt dissapear after one use and its kinda the only one that does that…|Automatically teleport new players when joining InRound / Planning etc|
|During planning its hard to know what each machine it, not obvious|default language is reverse which is confusing|
|Need a recipe book|PS4 Controller touchpad causing “mouse camera rotation”<br>clicking the touchpad & analog stick in the opposite direction cam rotates that way|
|revamp the names|- bug where you cant place the table next to the register<br>- => (i’ve disable bounds checking on placement for now)|
|penalty if you make too much extra? waste too much ingredients|at round 3 the people got stuck in line as if there was an invis person at the front|
|highlight spots on the map where this thing can go|BUG: Hide pause buttons from non-host since they dont really do anything anyway|
|Text doesnt rotate based on the camera|BUG: When host opens settings, probably dont bring everyone else too|

## End TODO 


## Changelog

if youd like to try a specific version, the easiest way is to use gitblame to find the commit the changelog line was added
and then just checkout that hash (sorry im not doing releases / tags atm) 

I am publishing some releases on itch, you can find the url in the code or build the game and click the itch.io icon. 
if you need the password join the discord and say hi!


### alpha_0.24.09.06

Its possible this version is spectaularly buggy since I havent tested that much over the network....
maybe skip this one and try the next one if you can :) 

design changes
- Customer Spawner now spawns all customers by midday, so you have more time to serve them 
- change so that you can only clean the toilet when its not in use
- drinks can now be prepped in the daytime (make sure to buy some extra tables to store them)
- make it so that the register is only spawned outside when you are in tutorial mode 
- You now move furniture with the 'C' key or Triangle/Y on gamepad
- Upgrades and Store no longer require all players to complete action 
- You now have to walk to the store and upgrade areas 
- No longer need to drop the cup into the beer machine to fill it up 

impact
- you will now get an estimate for how much you might make for a given day
- Store now has a locker to save items day over day 
- Customer Spawner now has a progress bar 
- Customers will now wander where theres no space in the line

bug fix 
- Beer machine should be less flakey now
- Settings page X and B buttons were reversed 
- Store correctly validates cart contents now
- Customers will now check to see if they can path to the register before choosing it 
- Spawner will no longer spawn all customers instantly during first round
- Fix bug where username would think it wasnt locked in when already locked 
- Store spending animation should be more reliable 
- Fix issue where UI was always drawn at 720p 

better eng
- Add better instructions for getting raylib4.5
- most code will now run at 60fps on the backend 
- only show validation errors when standing on trigger area 
- add support for new types of trigger areas 
- rewrite timer to split reason from timer 
- remove most game states to simplify system 
- bind -> lambda 
- by default ignore entities in the store when running a query 
- pathfinding should be much much faster now (+ its own thread) 
- compilation speedups 
- new randomizer engine with unittest to guarantee order
- less dynamic allocations and more constexpr 
- less layer code and more in rendering system
- new translation system that supports formatting
- better performance by running the map validation 2400x less often 
- add script to validate that we have the right polymorphs setup 


### alpha_0.24.05.11

design changes

impact 
- Added a tutorial for the first two rounds (toggle it in the lobby) 
- Controls now have icons instead of the underlying name (see settings page or tutorial)
- better mouse hover graphics 
- customers will now show a "dollar" icon when paying 
- shop
    - increase number of items in shop from 5 to 10
    - add way to reroll, starts at 50 coins and goes up by 25 each use
- toilet 
    - needs to be cleaned at end of day 
    - has a queue and those waiting in line to the bathroom will eventually go on the floor if they wait too long 
    - customer patience will increase when they move up in line 

bug fix 
- fix bug where customers always thought they were second in line 
- hide names for walls when in planning phase 
- fix a crash that would happen when you close the game
- fix issue where only UI keys were visible in the settings

better eng
- switch from bind to lambda to save 3 seconds on full build :) (120.3s -> 117.30s on m1 air) 
- Query builder upgrades and new features 
- move all components to unique_ptr instead of raw points
- added the ablity to draw ghost blocks
- better dylib commands 
- more color utils 
- eliminate a class of auto conversion make_entity bugs 
- onboard onto sonar cloud: https://sonarcloud.io/summary/new_code?id=gabeochoa_pharmasea 


### alpha_0.24.04.25

New Upgrades
- Jukebox: Customers will pay money to use and keeps their patience from draining (no actual music yet tho) 
- Cant Event Tell: The more customers drink the less they care about the recipe's accuracy 

impact
- Menu UI fully redesigned (#thanks @steven for the design feedback) 
- new icons for existing Upgrades that were using the default before 
- customers now take time to pay (though no ui to show yet) 
- customers will now walk somewhere to drink their drink (instead of standing at 0,0)
- customers will now show a "need to pee" icon when they gotta go 
- players now spawn outside near the delivery zone instead of at 0,0 
- furniture names are now shown during design mode (more obvious what mode you are in and better tell what anything is) 
- New map generator (should crash less often but i cant promise no inifinite loops :$) 


bug fix
- Players should get stuck in dropped furniture way less often (might i say solved?) 
- fix issue where adding syrup to a drink would add salt and mint instead 
- fix a crash that happened sometimes when adding solid fruits to a drink 
- Progress bars will now dissapear if the drink was removed before complete
- Fast Forward Box can no longer be used as a table 

better eng
- translations support for trigger areas
- add more validation for missing items from model test 
- deleted unused code & ran some fixes to avoid undefined behavior 
- speed up entity overlapping check by 1000x
- Rewrote AI system to be much more ergonomic and easier to read/develop for 
- More debug ui for ai system 
- Address a ton of floating todos that were mostly tech debt (still 630 in the codebase though) 


### alpha_0.24.04.06

Known Issues
- (Store) New machines spawn on top of each other
- (UI) Controls are not remappable in UI 
- (Game) Map generator always makes basically the same map
- (Game) No way to know if you made the wrong drink 
- (Game) Can place items where it collides with your body


New Upgrades 
- Speakeasy: People will pay more the hard to get to register
- Main Street: Twice as many customers 
- HappyHour: More customers in the start of the day but cheaper drinks
- Pitcher: Unlocks a way to make many of one drink at the same time
- MeAndTheBoys: Customers can order a pitcher of 10 beers
- Mocktails: Can leave out one alcohol from drinks
- HeavyHanded: Less profit & more vomit, but customers will order more
- PottyProtocol: Customers will try to vomit in the toilets before the floor
- SippyCups: Customers order less and take longer to drink
- DownTheHatch: Customers drink faster and order more 

impact / design changes 
- animation for when money is deposited
- champage now shows a star rating based on how well you played the minigame (more stars more tip) 
- upgrades are now shown in the pause menu (hover to see more info) 
- customers:
    - can now tip based on their remaining patience
    - will now find the closest toilet first 
    - now pay at the register after finishing their drink

bug fixes
- Less crashes when adding ingredients to drinks
- Fixed issue where all platforms used Mac keymap, Windows users can now paste!
- Fixed entity duplication for entities spawned from upgrades
- Fixed bug in random number generation (was incorrectly [a+32k, b+32k] instead of [a,b])

better eng
- revamped translation system (part 1) 
- simpler upgrade system for faster development
- better ergnomics for writing queries
- add support for sorting entity queries
- faster overlapping entities check 
- faster typechecking for everything (str -> enum)
- add ability for upgrades to lower values (w/ division)


### alpha_0.23.11.22

Known Issues
- (Store) New machines spawn on top of each other
- (UI) some screens are gonna be ugly for a while
- (UI) Controls are not remappable in UI 
- (Game) Map generator always makes basically the same map
- (Game) No way to know if you made the wrong drink 
- (Game) Can place items where it collides with your body


impact / design changes 
- Upgrade - Pitcher: make more than one serving of a drink at the same time
- New Drink - Champagne: serves three people but bottle has minigame to open 

bug fixes
- Customer finished peeing should no longer crash
- Whenever you unlock a new machine through an upgrade it should now spawn in the next store 
- Upgrades that lengthen the day now actually work (they did nothing before) 
- Customers should now correctly order drinks unlocked through upgrades 
- Progress bar should no longer show red when pouring a drink
- Beer now correctly counts as alcohol (ie contributes to vomit amount) 
- Roomba now only leaves his house if theres some vomit (before he'd just sit at 0,0)
- Customers should now face where they are walking

better eng
- unittests for apply and remove upgrades
- add drinks as possible pre-reqs for upgrades
- add a way to disable upgrades in config 
- new Entity Query utils which should be more readable than entityhelper
- Rewrite of AI system 


### alpha_0.23.11.14

Known Issues
- (Store) New machines spawn on top of each other
- (UI) some screens are gonna be ugly for a while
- (UI) Controls are not remappable in UI 
- (Game) Map generator always makes basically the same map
- (Game) No way to know if you made the wrong drink 
- (Game) Can place items where it collides with your body


impact / design changes 
- Add Beer!
    - Place the cup in the barrel and press the work button to fill
- Added Bathroom!
    - Customers will now use the toilet sometimes
    - Toilets will need to be cleaned after being used three times 
    - They are auto cleaned overnight
- Added non drink upgrades
    - These will either unlock new machines, types of customers, or change backend numbers
    - Check config/round_upgrades for more info or ideas.md for some in progress ones 
    - They will show up in the progression menu every once in a while 
- Changed trashcan model to be a trashcan now that toilet is being used as a toilet
- Lobby rearragement so you can see everything on spawn 
- We now try to predict the drink so its easier to know if you got the recipe right 
- Display fruit name above fruit 
- Display the drink model along side the icon for customer speech bubble

bug fixes
- Fixed issue where sometimes trash would incorrectly think an item was in there
- All existing drinks should have models now
- Customers should stand in a better spot when waiting for their order

better eng 
- Add a room for placing one of every model in the game
- Better constexpr contrainers
- More h to cpp splitting 
- Logs should be more readable on windows now (we actually use fmt sometimes) 
- Start cleaning up Job.cpp file and moving things to system as we can
- Components can now reach their parent entity if needed
- Write upgrade function and move all parameters into round_settings on sophie


### alpha_0.23.11.02

Known Issues
- (Store) New machines spawn on top of each other
- (UI) some screens are gonna be ugly for a while
- (UI) Controls are not remappable in UI 
- (Game) Map generator always makes basically the same map
- (Game) No way to know if you made the wrong drink 
- (Game) Can place items where it collides with your body


impact / design
- Store will now generate items smarter than before (it was just random before)
- you can now move my face! (lots of people asked for this) 
- max framerate is now 240fps

bug fixes
- now pick up / drop on client works, fix mutex bug for client 
- fixes a windows crash that happens when your monitors resolution is not in our supported resolutions 
- fixes a bug where every day customers would vomit more than the previous day
- dont allow putting drink with ingredients back in the cupboard 
- you can no longer put ingredients in cup while its in cupboard 
- last used IP should now save even when a crash happens
- Drop Preview box should no longer rotate when in non-snap camera mode
- Drop preview should no longer show red if you overlay on the same spot you picked it up from 
- Fix bug where you can hand customers drinks directly (you can still hand to the mop) 
- disable map randomizer since it crashes the game often 
- Fix crash when disconnecting from a host and reconnecting 

better eng 
- change from singleton to static pointer for now because when processing the intial resolution update this-> is nullptr and somehow this fixes that 
- lots of network changes to better support leaving and rejoining


### alpha_0.23.10.20

Known Issues
- (Store) New machines spawn on top of each other
- (UI) some screens are gonna be ugly for a while
- (UI) Controls are not remappable in UI 
- (Game) Map generator always makes basically the same map
- (Game) No way to know if you made the wrong drink 
- (Game) Can place items where it collides with your body


impact / design changes
- make patience 30s per ingredient (and also count prereqs to add time for conversions)
    - For example, Margarita went from 60s to 120s
- increase time between vomits
- Walking on vomit slows you down 
- Starting Map
    - Now only one register 
    - Start with Trash can 
    - Remove mop 
- Settings 
    - Added reset-to-default button for settings 
    - Added ability to use alt+enter to toggle fullscreen (or through settings) s/o @Mora
    - Save and Exit button 
- Swap selected recipe with B button xbox / o ps3 instead of dpad for controller
- Always render price in store and move price forward so it conflicts less 
- Show position of invalid trash item 
- Default language is now english 

bug fixes
- move reasons higher and only show one at a time 
- Fix visual issue when two players both were holding soda spouts
- Fix crash when dropping empty cup onto filtered conveyer
- Block player from throwing away trash can or cups
- Pausing no longer deletes all items in the store 
- Make sure to unlock fruits when unlocking the juices
- Fix issue where the soda machine was triggering the "round end error" 
- No longer count players as "furniture" in spawn area
- Handle out of bounds crash when viewing Settings Keymap 
- Change name of "Pill Dispenser" to "Fruit Basket"
- Change name of "Medicine Cabinet" to "Alcohol Cabinet"
- Fix issue where picking up lemon would swap to lime and vice versa 

better eng 
- No longer count colors.h in line count 
- Lots new entity helper functions 
- Sophie validation into its own file 
- Fix overflow with IngredientBitset



### alpha_0.23.10.16b

Known Issues
- (Store) New machines spawn on top of each other
- (UI) some screens are gonna be ugly for a while
- (UI) Controls are not remappable in UI 
- (UI) No reset-to-default state for controls
- (Game) Map generator always makes basically the same map
- (Game) No way to know if you made the wrong drink 
- (Game) Can place items where it collides with your body

impact
- Store 
    - You now can see your balance and how much is in your cart 
    - Added machine names and prices so you know how much things cost and what they do (more work to be done here) 
    - Move the "free" machines into the store so you know what you are getting for your new recipe
    - Spawn 5 random machines when entering the store 
    - Added error messages when you cant leave store for reasons 
- Drink prices are now based on complexity instead of just being 10 coins
- You can now mop with your empty hands but its really slow 
- Customers now use one of the random customer models 
- Added round validation for when you are throwing out something you probably need 


bug fixes
- Fix crash for non-host player (sophie problems) 
- Rooma guy no longer should get stuck at customer leave spot
- Fixed issue where sometimes floor marker would only mark one item 
- Fix crash when stealing from store
- Fix issue where sometimes you get charged for free machines 
- Fix visual glitch when purchased item containers would be missing the item until the first time you grab one
- Fix issue where deleting a machine holding an item would leave a floating item around

better eng 
- more reliable pathfinding system (ezpz bfs instead of astar) 
- more util functions for EntityHelper 


### alpha_0.23.10.16

Known Issues
- (Progression) New machines spawn on top of each other
- (UI) some screens are gonna be ugly for a while
- (UI) Controls are not remappable in UI 
- (UI) No reset-to-default state for controls
- (Game) Roomba guy doesnt like to clean vomit at the customer de-spawn point..
- (Game) Map generator always makes basically the same map
- (Game) No way to know if you made the wrong drink 
- (Game) Can place items where it collides with your body
- (Game) Trash Area can delete things you likely need for future rounds 

impact 
- Customers will now pay & tip 
- You now have a bank balance 
- Added Store (doesnt cost money yet and always spawns the same two things)
- In new recipe selection, we now show the ingredients 
- New Furniture
    - Ice machine 
    - Single Alcohol 
- Added preview for where the furniture will drop 
- The first customer each day will want the most recently unlocked drink

bug fix:
- Fixed issue where clients might get stuck when picking up things 
- Fixed issue where trash overlapped with walls on big map 

better eng:
- Added mode (key: I) to disable checking ingredients, allows serving an empty cup
- Made FastFwd and Customers faster while in debug mode
- Added NamedEntities for faster sophie fetching (server only) 
- Helper function for get all entities in range
- Better floor marker code 
- more flexible transition system in preparation for the store

### alpha_0.23.10.01

Known Issues
- (Progression) New machines spawn on top of each other
- (UI) some screens are gonna be ugly for a while
- (UI) Controls are not remappable in UI 
- (UI) No reset-to-default state for controls
- (Game) Roomba guy doesnt like to clean vomit at the customer de-spawn point..
- (Game) Map generator always makes basically the same map
- (Game) No way to know if you made the wrong drink 
- (Game) Can place items where it collides with your body
- (Game) Trash Area can delete things you likely need for future rounds 

impact
- Sounds
    - Sound Effects for adding most ingredients
    - Sound for picking up and dropping items 
    - Sound for moving around the UI / clicking buttons
    - Blender Sound 
    - Sound FX slider in settings (to turn off new sounds if you dont want them)  
    - Add in Game music though its kinda annoying 
- Furniture
    - Added a holder for the mop guy so you can move his spawn point 
- For customers with low patience, we now change the color of the progress bar and the UI cards when running out of time
- Ability to lock the camera to the default view if youd like 
- Floor Markers
    - Add Trash Area where you can delete items (doesnt validate anything at the moment)
        - Items in the trash area will have a trash icon 
    - Added area for where new machines spawn
    - Warning for when you forget items in the spawn area 
- We now show how many customers will be coming in the next round 
- Better recipe book so you can still play while reading (though its kinda hard with controller) 
- Customer progress bars will look much nicer now


bug fixes
- Crashes
    - fixed stealing drinks back from customers
    - disabled register path rendering 
- Game Breaking
    - Fixed issue where controllers couldnt navigate settings page when in pause menu
    - Fixed issue where progression transition would break if you were in debug mode
    - Fixed issue where SimpleSyrup wasnt spawning 
    - Added controller controls for recipe book 
- Annoying 
    - Fixed issue where some items were not getting deleted at the end of day 
    - Fixed issue where customers were never vomitting 
    - Made it easier to use gamepad to navigate dropdown (though can still be better) 
    - Better spawn points for new players when game is already in progress 
 
better eng 
- Add warning when text size is below accessibility guidelines 
- Lots of includes changes which should help reduce recompiles
- Automatically detect new types of item containers (reduces class of easy to make bugs) 
- Better file splitting 
- Lots of constness
- Reduce lots of shared_ptr uses
- Cleanup some unused files
- Convert some typedefs to using 
- Change some couts to log_infos
- Add warning when playing sounds on server (they will only play for host)_ 

### alpha_0.23.09.09

Known Issues
- (Progression) New machines spawn on top of each other
- (UI) some screens are gonna be ugly for a while
- (UI) Controls are not remappable in UI 
- (UI) No reset-to-default state for controls
- (Game) Roomba guy doesnt like to clean vomit at the customer de-spawn point..
- (Game) Map generator always makes basically the same map
- (Game) cant pick up roomba in planning 
- (Game) No way to know if you made the wrong drink 
- (Game) Can place items where it collides with your body

impact
- Customers now have patience
    - Longer timers for more complicated drinks 
- New Drinks!
    - Long Island Iced Tea and Mai Tai 
- New Fruits!
    - Pineapple and Pinapple Juice 
- You can now view recipes for your unlocked drinks by holding Tab / Gamepad-Top-Button
- We now hide the mouse when holding down to move camera (Viewer Suggestion :) )
- Configure the analog stick deadzone in json now 
- Can now place and pick up the little mop boy 
- Better progress bar UI element
- Can walk around with arrow keys now 
- Added a sound when people vomit 
- Moved Round Timer and made it bigger 
- Added a new validation that will stop Round transitioning when you have overlapping furniture 
 
bug fixes
- End of Round Timer text should get cut off less than before with new font 
- Much better collisions, You should get stuck on edges much less now 
- Fix crash where rotating objects more than 360 could crash game 
- Added more controller mappings from SDL 
- Fixed issue where item placement validation wasnt working (soda wand could go anywhere) 
- hide progress bars when not needed 
- Fix issue where you could have two dropdowns open at once 
- Better validation for when register is not pathable 
- tab between options works again 
- better arrow key movement for dropdowns 
- register validation should be way more accurate now
- Toasts now will fade out 

better eng 
- Fix some wwarnings
- Add mouse input events 
- update debug info to look a little better 
- move ui into gamedebug layer instead of system/
- add new component for marking entities needed to update at end of day 
- move ping and version to top right 
- add simple clang-tidy setup 
- add simple color blindness simulator shader 
- bad but better ui code splitup 
- add ui to tell when no clip is on 
- move toasts to bottom right 
- extract progress bar into helper render function 
- Updated Windows VC Proj Filters
- Made it possible to compile with an older windows compiler, avoiding an issue with a compiler bug in platform Windows toolset v143
- Added debug walkable ui, needs its own controls eventually 
- Added debug render info for waiting spots on register 
- Added easings library

### alpha_0.23.08.25

Known Issues
- (All) movement sucks today with analog stick
- (Progression) New machines spawn on top of each other
- (UI) some screens are gonna be ugly for a while
- (Game) Roomba guy doesnt like to clean vomit at the customer de-spawn point..
- (UI) Controls are not remappable in UI 
- (UI) No reset-to-default state for controls

impact
- Map generator now uses WaveFunctionCollapse
    - This should result in some less boxy maps than before
    - If you want to mess around with it you can edit map_generator_input.json
- More fonts and other language support
    - We got a new font that hopefully fits better with the games playful style :)
    - Due to differences in font and string width, some text might be cut off until you restart the game :$
- new little roomba guy (hes underneath the pumpkin
    - Moves slow but cleans up vomit without any help :) 
- New alcohol based customer names!
- Added controls view in the settings page
    - No longer do you have to guess the controls
    - Controls are not currently remappable in game but you can update the keymap.json manually if you need

bug fixes
- Fixed this issue; see impact for more info: (Lobby) Seed changer disabled for now 
- Fixed issue where clients couldnt change settings once in game 

better eng 
- Categorized some of the todos 
- Popup window to our UI component library 
- Font Library to prep for multi-language 
- Lobby etc are now Perma-entities so we can reenable seed selection 


### alpha_0.23.09.02

Known Issues
- (All) movement sucks today with analog stick
- (Progression) New machines spawn on top of each other
- (UI) some screens are gonna be ugly for a while
- (UI) Controls are not remappable in UI 
- (UI) No reset-to-default state for controls
- (Game) Roomba guy doesnt like to clean vomit at the customer de-spawn point..
- (Game) Map generator always makes basically the same map

impact
- New Fruits (and associated juices)!
    - Lime / Coconut / Cranberries / Oranges
- New Drinks!
    - Screwdriver and Moscow Mule (These suggestions came from viewers like you!) 
- Press "space" as the host on the seed block to enter your own seed
- You can now face any direction to your hearts content (it might be harder to play now tho)
- Added ability to change UI theme in the settings menu 
    - you can also make your own in config/settings.json
- new secondary UI color for default theme 
- Character Switcher now has a label so its more clear what it does

bug fixes
- Map generator should not spawn anything inside the walls

better eng 
- UI Library
    - Added scrollable views
- Lots of pointers removed and/or migrated to OptEntity framework 
- Moved entity helper code to cpp
- Restrict valid resolutions to 16x9 for now

### alpha_0.23.08.25

Known Issues
- (All) movement sucks today with analog stick
- (Progression) New machines spawn on top of each other
- (UI) some screens are gonna be ugly for a while
- (Game) Roomba guy doesnt like to clean vomit at the customer de-spawn point..
- (UI) Controls are not remappable in UI 
- (UI) No reset-to-default state for controls

impact
- Map generator now uses WaveFunctionCollapse
    - This should result in some less boxy maps than before
    - If you want to mess around with it you can edit map_generator_input.json
- More fonts and other language support
    - We got a new font that hopefully fits better with the games playful style :)
    - Due to differences in font and string width, some text might be cut off until you restart the game :$
- new little roomba guy (hes underneath the pumpkin
    - Moves slow but cleans up vomit without any help :) 
- New alcohol based customer names!
- Added controls view in the settings page
    - No longer do you have to guess the controls
    - Controls are not currently remappable in game but you can update the keymap.json manually if you need

bug fixes
- Fixed this issue; see impact for more info: (Lobby) Seed changer disabled for now 
- Fixed issue where clients couldnt change settings once in game 

better eng 
- Categorized some of the todos 
- Popup window to our UI component library 
- Font Library to prep for multi-language 
- Lobby etc are now Perma-entities so we can reenable seed selection 


### alpha_0.23.08.19

Known Issues
- (All) movement sucks today with analog stick
- (Progression) New machines spawn on top of each other
- (Lobby) Seed changer disabled for now 
- (UI) some screens are gonna be ugly for a while

impact
- we finally got a release build check it out on itch
    - add link to itch.io in main menu
- we now have cards at the top to show what drinks to make (only shows person at the front of the reg)

bug fixes
- Finally fixed this one: (UI) The first option in a dropdown isnt selectable 
- update trashcan model render location 
- dont allow simple syrup to get deleted by toilet 
- turn off place collision checking for now until its fixed 
- clear pathing cache when picking up furniture 
- Create all directories needed to store the save file 
- spawn simple syup when needed for progression

better eng 
- New UI library 
- Move theme loading to json config file
- remove heldby and migrate to entitytype 
- add util function to check if an ingredient is enabled 


### alpha_0.23.08.14

Known Issues
- (All) movement sucks today with analog stick
- (UI) The first option in a dropdown isnt selectable 
- (Progression) New machines spawn on top of each other
- (Lobby) Seed changer disabled for now 

impact
- More drink options!
- Add Progression into the game
    - After each round you get teleported to the progression area,
    - you can choose which of two drinks to add
    - if the new drink requires new ingredients we'll spawn them outside for you
- New Default Map
    - Much simpler default map which makes progression worth it
- Floor Triggers now respect selected language

bug fixes
- Fix issue where i was using |= instead of .set() which caused weird progression issues 
- Fix issue where spawners werent resetting 

better eng 
- add util function for checking if any entity of that type exists 
- add util function for running a function on each enabled bit in a bitset 
- allow teleporation to only certain locations for now 
- Refactor trigger system so all code lives in system manager
- add typename to error messages for library 

### alpha_0.23.08.08

Known Issues
- (All) movement sucks today with analog stick
- (UI) The first option in a dropdown isnt selectable 

impact
- Seed Changer now shows the current seed on the block
- Added fast-forward block to move the day faster if you find it too easy
- Character now does local movement prediction so it should feel smoother
- Added placeholder model for wall (not used yet) 
- Added hand cursor for in menu (no idea how it should work for controller yet though) 

bug fixes
- Hide timer when in lobby 
- Stop conveyers from grabbing items from people (again)
- Bring back loading text waving
- Stop generating entities for empty space

better eng 
- Switch from strings for entities to enums, entire game should run faster 
- Lots of file changes 
- Start storing the ingredients we haven unlocked 
- Bitwise operations are now constexpr

### alpha_0.23.08.05

Known Issues
- (All) movement sucks today with analog stick
- (UI) The first option in a dropdown isnt selectable 

impact
- Added Seed Changer! 
    - The generated map is really simple right now so use the default one, but it works
    - Also shows a minimap with a visual of the generated map
    - We also validate certain things about the map (like spawner / timer / path to register) 
- Added more rope between soda stream 

bug fixes
- Furniture highlighting works again! 
- Increase network bandwith which reduce lag (but is a bandaid) 

better eng 
- Merged lobby and game worlds so the code is a ton easier 
- Split a server/game into cpp
- Added basic logging for windows 

### alpha_0.23.07.30

Known Issues
- (All) movement sucks today with analog stick
- (UI) The first option in a dropdown isnt selectable 

impact
- Add new Furniture 
    - Mop Holder and Mop to clean up Customer Vomit
- Furniture Changes
- New Ingredients 
- Add components for handling progression across rounds 
    - Customers now only generate drinks that are enabled for that round

bug fixes
- fix a bunch of grab/drop issues which should make this a ton more reliable 
- Entity Cleanup is better performing on windows
- Spawners now reset at the end of the round 
- fix issue where placing into a container was sometimes producing duplicate objects 

better eng 
- const all the things (plus other cppcheck issues)
- cpp a ton of code 
- new EntityID type for just extra info
- move a ton of shared_ptr > entity&
- add bitset utils


### alpha_0.23.07.29

Known Issues
- (All) movement sucks today with analog stick
- (All) grabbing / dropping is pretty bad and doesnt work as expected 
- (IR) Conveyer belt model is now bread cause it kept crashing on windows 
- (UI) The first option in a dropdown isnt selectable 


Game Renamed!
- pub panic for now :) 

impact 
- Add new Furniture 
    - cup spawner 
    - trash can 
    - squirter (finds alcohol nearby and squirts it into the cup)
    - filtered grabber (name + subtype) 
    - Pneumatic Pipe (teleporter) 
- Furniture Changes
    - pill dispenser => fruit basket 
- New Ingredients
    - add bitters and the other liquires? liquoors? 
- customers can order multuple times now 
- add Day Counter
- Menu now has button to link to discord invite 

bug fix
- customer takes their drink now (also hide drinks that are not makeable yet) 
- handle if you put something back into an indexer that doesnt match the current index 
- fix issue where item containers could have duplicated items 
- conveyers now respect canhold filters 
- fix issue where items without subtypes crashed the game 
- fix subtype rendering for filters 
- fix issue where countdown seconds were each two seconds (walkable cache) 
- fix issue where item containers with limits werent generating more in future rounds 
- invalidate walkable cache when player moves furniture around 
- block player from dropping furniture ontop of other furniture 
- reset customer order count when changing rounds 
- delete the customers when ending the round 

better eng
- allow rendering underlying enum name in subtype debug info 
- remove bag / pill bottle / pill 
- no more reference shared pointers 
- change from filter function to data store 
- disable network if in gabe mode 
- add debug info for filters 
- no longer need to update the entire model object 
- add back support for dynamic model loading 
- new model info => model info :) 
- more model loading 
- move basic model rendering to json config 
- Migrate all drinks and recipes to json
- add button in pause menu to releoad configs, good for model scale/position changes 
- add function to open urls 
- add image_button() to ui library 
- extract search range for teleporter into global 
- add better json loading utils in preload 
- move one off textures to config file 
- lot less logs on startup 
- update to local player list in case we consider doing that later 
- remove all uses of global player 


### alpha_0.23.07.21

Known Issues
- (All) movement sucks today with analog stick
- (All) grabbing / dropping is pretty bad and doesnt work as expected 
- (IR) Conveyer belt model is now bread cause it kept crashing on windows 
- (UI) The first option in a dropdown isnt selectable 


**Pivoting from Pharmacy to Bar** 
- fully pivoting from medicine to _medicine_ 

impact 
- Added 10 drinks and 21 ingredients 
- Add placeholder images for each drink order
- Add blender that can whizz up fruits
- change medicine cabinet to be the alcohol storage
- add recipes for some popular drinks
- dynamic model based on ingredients
- add soda machine with spout
- enforce can_be_held when picking up / dropping
- add a check for when theres no path to register

bug fix
- fix bug where entity cleanup wasnt working in all cases 
- turn off audio engine only if audio is enabled 
- remove all memory leaks so the game should run much smoother over time (~5k different leaks)
- (All) you can get the controls into a backwards state.. Small camera formula refactor fix.

BE
- Convert all Items into Entities so now we only have one update render path 
- debug UI for ingredients to make it easier to understand what the current cup holds
- util::in_range function
- util function for getting ingredient from index
- turn off serialization for jobs and some other things that we dont need rendering info for
- only run sophie checks when time is out (speed)
- renamed entities:: => furniture::
- util for cleaning up by entity id
- update grab/drop to better support can hold checks
- add support for max-generated items for item containers
- change item held by to suport flags
- inital basic animation loading, tho untested
- add python script to extract i18n strings from strings.h to mo and reverse them
- move example map to configjson
- move entity makers into another file so we can include entity.h places without issue
- better const stuff across the code base (lots in rendering code) 
- add some notes about detecting streaming software
- made the json parsing for the config settings safe if json is improperly formatted.
- Added JSON library, config for log_level, and globally reading LOG_LEVEL from config file. Defaults to LEVEL_INFO if not supplied.
- removed useless #pragma

### alpha_0.23.07.08

Known Issues
- (All) movement sucks today with analog stick
- (All) grabbing / dropping is pretty bad and doesnt work as expected 
- (All) you can get the controls into a backwards state.. 
- (IR) Conveyer belt model is now bread cause it kept crashing on windows 
- (UI) The first option in a dropdown isnt selectable 

impact 
- Can change language and it updates across the game! (Setting saves across plays)
- Show who is connected to the game in top right with ping!
- When merging into an item container, the item now goes into your hand if it can wrap what you are holding
- Added a round timer that looks like a Sun rise. Store will open and close. When store is closed, customers wont come in, but any already there will stay around.
- Before switching rounds (open / closed) we now show a countdown (5secs) in case you are still doing something. It wont countdown if theres a reason why the round shouldnt end (for example people are still in the store) 

bug fix
- move name forward a bit to conflict less with models 
- rotate conveyers to be correctly aligned for test map
- grabber now grabs from non conveyers 
- Fix CheckHeldFurniture so that it correctly checks players and not just entities

BE
- add loop-through-files api 
- add warning to dropdown for when options is empty 
- be rename canhold update to be more obvious 
- move player info into the system manager 


### alpha_0.23.07.04

Known Issues
- (All) movement sucks today with analog stick
- (All) grabbing / dropping is pretty bad and doesnt work as expected 
- (All) you can get the controls into a backwards state.. 
- (IR) Conveyer belt model is now bread cause it kept crashing on windows 

impact 
- Spawner now spawns people! 
- Add ability to support multiple registers and find the best one 
- Add Day/Night cycle with sunrise ui and countdown timers
- Show a reason for when the gamemode isnt switching 

bug fix
- grabber now grabs from non conveyers 

BE
- debug mode turns on noclip now
- run tests only first and then the game 
- move render ui into rendering_system 
- rename canhold update to be more obvious 
- move player info into the system manager 
- add timer component 
- add util functions to know if a register is full 
- send inputs less often so that it works better for 60fps 
- send half as much data on every input frame :) 
- add leak tester command; also sign code i guess 
- add util function to find all entities with component 


### alpha_0.23.07.03

Known Issues
- (All) movement sucks today with analog stick
- (All) grabbing / dropping is really bad and doesnt work as expected 
- (IR) timer running out doesnt do anything 
- (IR) Conveyer belt model is now bread cause it keeps crashing on windows 

impact / gameplay changes
- customers now check for if the bottle contains pills 
- change controls based on camera direction 
- character models are now updated over the internet so you can see who your friends are dressed as 
- added new character model (rogue) 
- turn on/off speech bubble when customer is at front of line / leaving
- you can now pick up pills with the pill bottle
- added pill selector crate (hold work button to switch)
- cooler looking default map 
- Created trigger-area so you can actually load into the game without the devtools! (Also shows number of players needed) 
bug fixes
- fixed issue with pathfinding that was causing noclips
- you cant pick up walls anymore 
- fixed issue where dead items were being rendered still
- brought back furniture highlights!
better eng 
- pathfinding unittests
- speed up the gameupdate tick by 40x with this one weird trick (dont compile used logs)
- i think i may have solved the component desync problem for good (tm) 
- add dev flags for disabling sound and models to speed up game start during dev

### alpha_0.23.06.29

Finally finished the ECS rewrite!! 

What works:
- (Lobby) character switcher
- (Planning) can move furniture around
- (InRound) timer counts down 
- (IR) customers spawn and find a job
- (IR) customers correctly wait at register for their prescription 
- (IR) customers correctly leave when they get it 
- (IR) customers mostly look where theyre going

What doesnt:
- (IR) customers dont check whats inside the pill bottle
- (IR) timer running out doesnt do anything 
- (IR) customers still have trouble generating valid paths 
- (All) movement sucks today with analog stick
- (All) grabbing / dropping is really bad and doesnt work as expected 

bug fixes
- theres kinda tons, but im sure i also broke a bunch
impact 
- add ping overlay so you know what your ping is
better eng 
- update raylib to 4.5
- organize the code a bit better into folders (ie move engine stuff to engine/)
- no more inheritance for entity
- added unittest for basic ecs stuff
- privatize any non-const functionality in components 
- rename M_ASSERT to VALIDATE so its more obvious what it does/means
- added information on how to get lldb to work 

### alpha_0.23.02.06

better eng
- Add tons of components and systems to handle them
soon
- finish migration to ent component system 


### week of 1 30 (no new version) 

bug fixes
- fix issue where master volume setting wasnt being respected 
- fix issue where pause screen showed game instead of lobby 
- fix bug in pause layer state manager 
- only allow putting things back in the supply if they arent holding something else inside 
impact 
- add character switcher to lobby 
- add character switcher furniture 
- send users to lobby on join 
- lower time to wait before kicking someone who disconnected/crashed
- detect host disconnect and return to main menu 
- add different toast background color based on the message type 
- add round with timer and UI element 
- janky render item inside other item 
- make it so you can drop_merge items 
- add barrel holding pill bottles 
better eng 
- allow host to render server view for debugging
- add new debug keys 
- move all library loading logging into Library<> so we can turn it into trace -> info as needed 
- remove back from pauselayer now that we can use go_back
- rename entity update methods to be more accurate to gamestate logic 
- add new state manager system with separate states for menu and game 
- add basic tracing support and add more trace points 
- make library impls private 
- add player models and better model library api 
- allow scaling items over all axes 
- quicker missing-value path for globals register.get_default 
- new job system that is hopefully more readable and more resilient 
soon 
- :) 



### week of 1 30 (no new version)

bug fixes
- Fix bug where sounds loaded after settings load would use wrong volume setting 
- Fix file loading for windows 
- Fix issue where we called cleanup on null sharedptr 
- dont allow bags to go into bags 
impact 
- Add new music (supermarket, menu)
- Bunch of pa announcement sounds 
- make the key holding feel nicer 
- make people a little smaller 
- add the ability to do work on a furniture during game hours 
- hook up announcment messages to toasts 
- remove player when the disconnect :) 
- hopefully better feeling backspace 
- Add ugly button to load last used api (stored in settings file) 
- box model now responds to planning vs game 
- add a 'pill bottle' item 
better eng
- Add Log library (remove most couts) 
- Add nodiscard to everything 
- Add magic_enum library 
- Add expected library (though not using it as much as we should) 
- Onboard all File uses into File:: api 
- Support for focus ring to take another color 
- better sound play api 
- add for-each in folder to files api 
- Cleanup UIContext a bit 
- Extract components into its own file" 
- Show how many things are in the library on close
- Ability for sound library to play a random sound given a prefix 
- Add menu system in settingslayer which loads and prepares us for keybinding settings 
- Add basic trie header
- add held_down debouncer for UI for better backspace 
- introduce delay for small packets to remove overhead and reduce map update speed by 2x 
- add message queue so we can separate process and deserialize later on 
- increase packet buffer size to 1mb 
- Move set clipboard to ext 
- Move raylib into a namespace, and expose through graphics.h 
soon
- more models :) 

### alpha_0.23.01.16

bug fixes
- Change item position for table so empty bag is easier to see 
- dont allow bags to go into bags 
- fix bug where sounds loaded after settings load would use wrong volume setting (though might still be happening)
- fix a todo for stopping paste when itll go over max length 

impact 
- Add new player models (no more red square)
- Add Round timer UI 
- Render items held inside other items (kinda janky tho) 
- Add pills and pill bottles (with Supply as barrel) 
- Make it so you can drop merge items (holding pill bottle drop onto bag)
- Box model now responds to planning vs game (it opens :D )
- Add logic so customers only want to pick up a bag containing a pill bottle 
- Add toast system to show all the network annoucements that we've had logs for 
- Remove player from game when they disconnect
- Add "last used ip" button for quality of life
- new Music for menu and in game
- Add a bunch of PA announcment sounds
- add validation for ip address input (2 weeks ago) <Gabe Ochoa>
- Added "hold to backspace" for textfiends and should feel decent

better eng
- Better Model Library api
- Can now "do work" on some furniture
- add ability for sound library to play a random sound given a prefix 
- add expected and magic_enum libraries 
- add basic tracing support through Tracy (much better than PROFILE())
- base class for an item container 
- new job system that is hopefully more readable and more resilient 
- smaller font cache 
- add util::join and util::words_in_string for later wrap use 
- extract font sizer to its own file 
- add toast framework 
- move raylib into a namespace, and expose through graphics.h 
- add basic trie datastructure
- introduce delay for small packets to remove overhead and reduce map update speed by 2x 
- add message queue so we can separate process and deserialize later on 
- increase packet buffer size to 1mb 
- UIContext cleanup its not better but it is more readable 
- add print statement on close to show how many things are in the library 
- add for-each in folder to files api 
- better sound play api 
- move controller loading to File::get() api 

soon
- Add ability to save and load chosen player model 
- disable pathfinding cache for now due to too many segfaults... 
- add menu system in settingslayer which loads and prepares us for keybinding settings 
- internal server cleanup though more to do here 

### alpha_0.22.12.26

Last update of the year

impact
- none cause its code freeze time, see ya in january 

better eng 
- start engine extraction so we can use this for other games in the future :) 
- fix bug where settings screen would show the wrong resolution on startup
- add more default resolutions 
- fixed undefined behavior in network/client.h
- fixed bug with map serialization 
- deleted world.h you will be missed. 

soon
- scrolling? 

### alpha_0.22.12.05

impact 
- Conveyer! and Grabber Conveyer!
    - You can now grab items and transport them to customers without touching items
- Fix issue with layer order so Pause Menu is ontop of all game items
- Resolution Switcher in settings page! (But hard to use :$)
- Basic Symptoms and Ailments with speed and stagger modifiers
- Bag-Box furniture, to get empty bags from (new model for empty bags)
- Pill Bottle model, but not in game yet
- Post-Processing shaders written by AI (+ can disable in settings)

better eng 
- Added dropdowns (but you cant scroll yet) 
- Switched to tracking todos in Obsidian with a kanban.py script to populate readme
- Add Ideas about "Bar"s to ideas.md cause idk anything about pharmacies
- Lots of updates to UI rendering code to remove deprecated rendering
- Unloading support for Library destroy-cleanup
- Switched from our own operator overloads to using a library (though i had to mess with it a bit)
- Faster compiling on mac through .o separation (though still lots causing recompilation at the moment since game.cpp includes everything) 
- Menu UI modernization updates (though many more to do) 
- Remove need to pass mouse information into UI context

soon 
- Scrolling UI elements
- Able to have different post processing shaders for game / menu



### alpha_0.22.11.21

impact 
- Conveyer works and delivers to Register
- Streamer Safe box visualization and settings 

better eng
- Inputs are enum'd so we can better track usage 
- Add helper macros for UI Size expectations
- Better menu system to allow easier go_back functionality
- better getMatchingEntity
- 

soon
- Conveyer Grabber 


### alpha_0.22.11.14

impact 
- Map now send across network
- Map now respects seed sent in 
- Version checking for player join 
- Friends can now pick up items and you can see! 

better eng
- Serialize entity, map, furniture, item (unblocks syncing across network) 
- Add planning and game modes (and pausedplanning) 
- more constexpr for globals
- add announcement types to annoucements (eventually do popups?) 
- better default tab location on Network screen
- Fix issue where tabbed item could be invisible 
- Move server into its own thread 

soon
- Conveyer
- Add settings page to pause menu ? 

### alpha_0.22.11.07

impact 
- Show IP address of host (hidden by default, has toggle button) 
- Add copy button for ip 
- Add ability to edit username from any screen showing it
- Add settings.bin file for storing settings across sessions 
- ^Skip username selector when already setup 
- Add pause menu (continue / quit) 
- Fix bug where "exit" button would just crash game to close instead of using close() functionality (so now saves work on exit button) 
- Fix crash when switching from host to peer after hosting

better eng
- Slightly better focus ring selection, still need much more work on network screen 
- Tables spawn with items on them since items cant be on the ground
- Add debug ui layer and move most debug info to it (Turn it on / off with '\')
- Add defer which you can use to force cleanup at end of scope (see defer.h for usage)
- Add profiler to see avg ms spent in function

soon
- Conveyer
- Add settings page to pause menu ? 

### alpha_0.22.10.31

impact 
- Added the ability to paste into textfields
- Multiplayer on both platforms ! (though you need to port forward :770)
- Multiplayer movement is now sync'd across the network

better eng
- We made the decision to switch from yojimbo to GameNetworkSockets.
- create Library<T> base class for easier library management later
- Add UI context ownership for widgets for better UI code 
- Made `EntityHelper::getMatchingEntityInFront` feel much better.

soon
- Sync more info across network

### alpha_0.22.10.25
impact 
- Spent most of the week doing network programming with enet. 
    - Got multiplayer movement working (check screenshots folder)
    - Had too many issues on windows, need to migrate over to a new library
- Character names generator (Thanks Alice for the lists)
- Textfield ui component 
- Timer object that can be treated as continuous or as n-shot timer
- Music and Sounds!
    - Customer order sound effect 
    - Gameplay placeholder music WAHWAH
    - Placeholder roblox oof sound when running into customers
    - Volume slider now actually controls master volume
- Table that you can use to place items down
- Prevent Person types from getting stuck in each other. (might need some more work) 
- Model for Cash Register (though its a bit dark cause no textures)
- Job System for customers !
    - They will now come in and line up for their RX, 
    - once they get one at the register they'll take it and go

better engineering 
- Gabe got a windows machine setup to help create builds long term 
- Include libs directly in vendor folder for easier first time setup
- Added better UI memory management so we can use composable functions for design / branching
- Added working headers for ailments/medicines/names (functionality coming soon?) 
- Cached all isWalkable checks 

soon 
- switching to yojimbo
- System for highlighting selected items / furniture (working sometimes but definitely not done)
- autolayout (launched but still missing dropdown ui element)
- Had windows issues with Sago library, blocked until we can figure out windows.h
- Working on a stagger walk 

### alpha_0.22.10.17

impact
- added Controller movement for camera / menus
- New ailment icons and bag model 
- Switched from manually checking entities for pathfinding to using a navmesh w/ caching
- Added Register furniture and customers now wait in line 
- Remove all non theme'd ui color usages
- Can now pick up and rotate furniture 

better engineering
- All loading happens in preloader now 
- Rotational facing logic rehaul + diagonal walking 
- Better font caching 
- Reduced lots of warnings
- Better testing infrastructure
- Slowed down the customer and made picking up items easier

soon
- Tests for entity fetching 
- System for highlighting selected items / furniture
- Migration to autolayout 99% complete (missing dropdown ui element) 
- Had windows issues with Sago library, blocked until we can figure out windows.h

### alpha_0.22.10.10

- nicer buttons and colors across all UI (unified theme :) )
- new font!!
- added texture loading with an image file 
- cleaned up a bunch of code (maybe making folders next? wdyt)
- added a new customer person that has a name above their head (see video) 
- added a texture library 
- Speech Bubble on Customer (though with billboard cant place anything on it yet)
- Started work on a UI autolayout-er
- Added a tests/ folder with a way for all tests to run on startup

pre-history
- added a library for finding the right savegame folder (like AppData etc) cause its different on every platform. 
- added loading / writing settings files (uncomment #define WRITE_FILES to test it out) 
- added a focus ring so its more obvious what you are tabbed into on ui


# pharmasea

libraries used:
- raylib
- Raylib Operator Overloads https://github.com/ProfJski/RaylibOpOverloads
    - Added a #define that removes all the raygui stuff 
    - Made every function inline since i globally include and the linker not happy with so many definitions (odr?)
- AppData stuff https://github.com/sago007/PlatformFolders
- Steam's GameNetworkingSockets https://github.com/ValveSoftware/GameNetworkingSockets
- HTTPRequest https://github.com/elnormous/HTTPRequest
- fmt https://github.com/fmtlib/fmt
- tl::expected https://github.com/TartanLlama/expected
- magic enum https://github.com/Neargye/magic_enum.git
- tracy https://github.com/wolfpld/tracy
- backward https://github.com/bombela/backward-cpp
- argh (for dev flags) https://github.com/adishavit/argh


Models from 
- https://www.kenney.nl/assets?q=3d 
- https://kaylousberg.itch.io/kay-kit-mini-game-variety-pack 



Info about the todo chart
- full raw data is in todo.md and managed by obsidian
- hide the "backlog" here but trust me theres lots more todo 
- only show the most recent 10 done items
- to regerate the table below run python kanban.py

## TODOs  

|needed to play once|in progress|blocked|done|
|-------------------|-----------|-------|----|
| |diagetic UI to lobby screen|dropdown needs scrollbar when subwindow goes offscreen|pill bottle barrel|
| |placeholder bottle pill filler|Support for "windows" or "modals" in ui framework|Magic Enum? https://github.com/Neargye/magic_enum|
| |Ability to do different shaders between gaming and menu|highlight furniture under selection|change UI from in world to render-to-texture|
| |Need a build system to output producuction build| |Add ailments for customers|
| | | |bag box to get paper bags from|
| | | |Put all things that have to be loaded in a single place|
| | | |resolution switcher to settings page|
| | | |Disallow target cube from going outside of bounds|
| | | |Refactor Library to be more generic|
| | | |add diagonal facing direction|

## End TODO 


## Changelog

if youd like to try a specific version, the easiest way is to use gitblame to find the commit the changelog line was added
and then just checkout that hash (sorry im not doing releases / tags atm) 

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
- move player info into the system manager (8 hours ago) <Gabe Ochoa>
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


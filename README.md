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



Info about the todo chart
- full raw data is in todo.md and managed by obsidian
- hide the "backlog" here but trust me theres lots more todo 
- only show the most recent 10 done items
- to regerate the table below run python kanban.py

## TODOs  

|needed to play once|in progress|blocked|done|
|-------------------|-----------|-------|----|
|Disallow target cube from going outside of bounds|Support for "windows" or "modals" in ui framework|highlight furniture under selection|change UI from in world to render-to-texture|
|In pause menu, remap key bindings in layer for arrows keys to choose options|dropdown needs scrollbar when subwindow goes offscreen| |Add ailments for customers|
|BUG: if customer runs into someone you both get stuck|pill bottle barrel| |bag box to get paper bags from|
|Better settings page|placeholder bottle pill filler| |Put all things that have to be loaded in a single place|
|WARN: need a way to warn that UI elements are offscreen "purpling"|Ability to do different shaders between gaming and menu| |resolution switcher to settings page|
|BUG: Hide pause buttons from non-host since they dont really do anything anyway| | |Refactor Library to be more generic|
|BUG: When host opens settings, probably dont bring everyone else too| | |add diagonal facing direction|
|button hover state color change| | |Add easy-to-use UI buttons|
| | | |Better state management for AI|
| | | |Allow an Entity to pick up / drop an Item|

## End TODO 


## Changelog

if youd like to try a specific version, the easiest way is to use gitblame to find the commit the changelog line was added
and then just checkout that hash (sorry im not doing releases / tags atm) 

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


# pharmasea

libraries used:
- raylib
- AppData stuff https://github.com/sago007/PlatformFolders
- Steam's GameNetworkingSockets https://github.com/ValveSoftware/GameNetworkingSockets
- HTTPRequest https://github.com/elnormous/HTTPRequest
- fmt https://github.com/fmtlib/fmt

(we only show the most recent 10 done items, but full raw data is in todo.md) 
(to regerate this run python kanban.py)

## TODOs 

|Backlog|In Progress|Done|
|-------|-----------|----|
|Create Doors|resolution switcher to settings page|Allow an Entity to pick up / drop an Item|
|Create Nav mesh for "walkability"| |Fix A|
|Disallow target cube from going outside of bounds| |Allow for an Item to be walkable or not (influences Entity movement)|
|Add some way for entities to subscribe to certain keys so we can more easily keep track of what keys are being requested over lifetime| |Add new target entity that moves via arrows keys / alternative keys|
|Add a pause menu with textual options| |Paint front-facing indicator (another cube, different color face, some shape)|
|In pause menu, remap key bindings in layer for arrows keys to choose options| |When Entities change direction, change the front-facing indicator to face the direction they're moving|
|Add easy-to-use UI buttons| |When Entities change direction, change the location of any held item to face the direction they're moving|
|Add system for exporting resources to code for easier binary packaging ([see branch packager](https://web.archive.org/web/20210923054249/https://veridisquot.net/singlefilegames.html))| |add diagonal facing direction|
|BUG: if customer runs into someone you both get stuck| | |
|highlight furniture under selection| | |
|Better state management for AI| | |
|Fix corner walls| | |
|Better settings page| | |
|Put all things that have to be loaded in a single place| | |
|Upgrade Astar to ThetaStar (worth doing?)| | |
|Need a build system to output producuction build| | |
|Refactor Library to be more generic| | |
|support for tile sheets| | |
|diagetic UI to lobby screen| | |
|bag box to get paper bags from| | |
|pill bottle barrel| | |
|placeholder bottle pill filler| | |

## End TODO 

Game Ideas
- Pharmacy
    - AI players need status conditions ("Ailments")
        - Ailments should have an poor OTC solution and good Rx solution
        - Poor solutions mean people are more likely to come back later in the day
            - Returning people with differing appearance for differentiation
            - Can't take an OTC solution again
        - Good solutions mean people are good for the day 
    - Small / Medium / Large pill bottle
    - How do upgrades work?
        - Options: Deterministic Skill Tree or RNG + money(Like Plateup)
    - Weigh station
        - Auto-weigher upgrade? 
    - Label maker
        - automatically labels held bottle
    - Generic medication stocking
        - Able to load up a whole shelf at a time
        - Grab box from pallet and drop on shelf to refil 
        - Each pallet holds 10 refills
        - How to get a new pallet
    - Cash Register
        - multiple lines per register
        - People at register dont lose patience
            - Or maybe lose patience when not floating still and regenerate a little bit on snapping
    - Conveyer belts or tractor beam
    - Phone will ring from doctor with prescription information 
        - (Include key bindings for picking up the phone)
        - QTEs for writing down the right information
        - Upgrade to a fax machine that automatically takes info with single press
    - Phone will ring from clients with questions about whether or not their prescription is ready
        - Webserver upgrade that you scan a completed rx on to mark "is ready for pickup"
        - Second hand Upgrade Allow to put people on hold?
    - Cleaning tools after use, otherwise risk contamination
    - Clients will have to have distinct names
    - Rerolls have chance for really rare things (sloth in SAPs)
    - Difficulty modifiers 
        - SpeedWalkers: Customers walk 1.5x as fast
        - Hypocondriacs: Customers always get more than once prescription
        - SelfMedicated: Customers heavily prefer OTC medicine
        - Chatty Doctors: Doctors spend twice as long on the phone 
        - Doctor Writing: Fax machine output harder to read
        - Bad Insurance Rates: make half as much money 
        - Curious customers: Customers ask questions while paying 
            - Decrease patience of other customers in the same line (implicitly)
        - Chatty Customers: Customers take twice as long to pay
            - Need some fish jokes
        - Mummblers: Customer names are harder to read 
        - Bad Fins: You swim slow? Get tired more easily? Have to rest? 
        - Gimme a pack of reds: Sell cigarettes (more money) but get more customers (more sick) 
        - It fell off a truck: sell Rx on the side, Double Income but risk getting caught
        - While I'm here: 
            - Adds food / snacks / drinks to sell 
            - Customers who are waiting a while will pick up more (causing cashier step to take longer) 
        - We have 4 seasons here: Adds weather which increases mess, but less people come in on those days
        - Vax Shot: More customers but only have to stab them (quick compared to rx) 
            - Global Pandemic: Adds more sick people & doubles the amount of vax patients
        - Beep Beep: Adds a metal detector that slows down thieves
            - Mall RoboCop: Adds an AI robot that tries to catch thieves
    - Different game modes
        - Big Box: Acquired by a franchise. 
            - More customers
            - Have to show advertising (takes up space) 
            - Have to pay x% of income to them at end of day 
            - Replace names with shapes 
        - Drive through
            - Only serve through one window 
        - Hospital
            - More sick customers
    - Customers have attributes?
        - Loud customers (giant speech bubble) 
        - Rude customers () 
        - Sick customers (more likely to have other customers return back later for more Rx)
        - Poor customers (cant pay but need lifesaving medicine)
        - Undercover cop (only if you take the double income, if you sell to them you lose immediately) 
        - Thieves (easily identifiable and have to grab them before they leave) 
    - Allow players to exchange held items?

## Changelog

if youd like to try a specific version, the easiest way is to use gitblame to find the commit the changelog line was added
and then just checkout that hash (sorry im not doing releases / tags atm) 

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


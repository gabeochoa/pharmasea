# pharmasea


libraries used: 
- raylib
- AppData stuff https://github.com/sago007/PlatformFolders


Core engine TODOs:
1. ~~Allow an Entity to pick up / drop an Item~~
2. ~~Fix A* algorithm~~
3. ~~Allow for an Item to be walkable or not (influences Entity movement)~~
4. Create Doors
5. Create Nav mesh for "walkability"
6. ~~Add new target entity that moves via arrows keys / alternative keys~~
7. Disallow target cube from going outside of bounds
8. Add some way for entities to subscribe to certain keys 
    so we can more easily keep track of what keys are being requested over lifetime
9. ~~Paint front-facing indicator (another cube, different color face, some shape)~~
10. ~~When Entities change direction, change the front-facing indicator to face the direction they're moving~~
11. ~~When Entities change direction, change the location of any held item to face the direction they're moving~~
12. Add a pause menu with textual options
13. In pause menu, remap key bindings in layer for arrows keys to choose options
14. Add easy-to-use UI buttons
15. Add system for exporting resources to code for easier binary packaging (see branch packager)
    https://web.archive.org/web/20210923054249/https://veridisquot.net/singlefilegames.html
16. BUG: if customer runs into someone you both get stuck

Gameplay Ideas
- Pharmacy
    - AI players need status conditions ("Ailments")
    - Small / Medium / Large perscription size
    - Label Dosage on the bag / pill bottle?
    - Generic medication stocking
    - Cashier? (Part of job?)
    - Auto-weigher? / Conveyer belts or tractor beam
    - Phone will ring from doctor with perscription information (Include key bindings for picking up the phone)
    - Phone will ring from clients with questions about whether or not their perscription is ready
    - Allow to put people on hold?
    - Cleaning tools after use, otherwise risk contamination
    - Clients will have to have distinct names
    - Difficulty modifiers at end of day (Like Plateup's)?
        - SpeedWalkers: Customers walk 1.5x as fast
        - Hypocondriacs: Customers always get more than once prescription
        - SelfMedicated: Customers heavily prefer OTC medicine
        - Chatty Doctors: Doctors spend twice as long on the phone 
    -

Changelog

alpha_0.22.10.10
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


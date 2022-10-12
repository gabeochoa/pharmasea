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
        - Ailments should have an poor OTC solution and good Rx solution
        - Poor solutions mean people are more likely to come back later in the day
        - Good solutions mean people are good for the day 
    - Small / Medium / Large perscription size
    - Label Dosage on the bag / pill bottle?
    - Generic medication stocking
    - Cashier? (Part of job?)
    - Auto-weigher? / Conveyer belts or tractor beam
    - Phone will ring from doctor with prescription information 
        - (Include key bindings for picking up the phone)
        - QTEs for writing down the right information
        - Upgrade to a fax machine that automatically takes info with single press
    - Phone will ring from clients with questions about whether or not their prescription is ready
        - Webserver upgrade that you scan a completed rx on to mark "is ready for pickup"
        - Second hand Upgrade Allow to put people on hold?
    - Cleaning tools after use, otherwise risk contamination
    - Clients will have to have distinct names
    - Difficulty modifiers 
        - SpeedWalkers: Customers walk 1.5x as fast
        - Hypocondriacs: Customers always get more than once prescription
        - SelfMedicated: Customers heavily prefer OTC medicine
        - Chatty Doctors: Doctors spend twice as long on the phone 
        - Doctor Writing: Fax machine output harder to read
        - Bad Insurance Rates: make half as much money 
        - Curious customers: Customers ask questions while paying 
        - Chatty Customers: Customers take twice as long to pay
        - Mummblers: Customer names are harder to read 
        - Bad Knees: You walk slow? Get tired more easily? Have to sit down? 
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
    - Customers have attributes?
        - Loud customers (giant speech bubble) 
        - Rude customers () 
        - Sick customers (more likely to have other customers return back later for more Rx)
        - Poor customers (cant pay but need lifesaving medicine)
        - Undercover cop (only if you take the double income, if you sell to them you lose immediately) 
        - Thieves (easily identifiable and have to grab them before they leave) 

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


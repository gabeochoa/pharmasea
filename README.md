# pharmasea


libraries used: 
- raylib


Core engine TODOs:
1. ~~Allow an Entity to pick up / drop an Item~~
2. ~~Fix A* algorithm~~
3. ~~Allow for an Item to be walkable or not (influences Entity movement)~~
4. Create Doors
5. Create Nav mesh for "walkability"
6. ~~ Add new target entity that moves via arrows keys / alternative keys ~~
7. Disallow target cube from going outside of bounds
8. Add some way for entities to subscribe to certain keys 
    so we can more easily keep track of what keys are being requested over lifetime
9. ~~Paint front-facing indicator (another cube, different color face, some shape)~~
10. ~~When Entities change direction, change the front-facing indicator to face the direction they're moving~~
11. ~~When Entities change direction, change the location of any held item to face the direction they're moving~~
12. Add a pause menu with textual options
13. In pause menu, remap key bindings in layer for arrows keys to choose options
14. Add easy-to-use UI buttons

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
    -

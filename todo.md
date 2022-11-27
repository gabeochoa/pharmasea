


## Backlog

* Create Doors
* Create Nav mesh for "walkability"
* Disallow target cube from going outside of bounds
* Add some way for entities to subscribe to certain keys so we can more easily keep track of what keys are being requested over lifetime
* Add a pause menu with textual options
* In pause menu, remap key bindings in layer for arrows keys to choose options
* Add easy-to-use UI buttons
* Add system for exporting resources to code for easier binary packaging ([see branch packager](https://web.archive.org/web/20210923054249/https://veridisquot.net/singlefilegames.html))
* BUG: if customer runs into someone you both get stuck
* highlight furniture under selection 
* Better state management for AI 
* Fix corner walls 
* Better settings page 
* Put all things that have to be loaded in a single place
* Upgrade Astar to ThetaStar (worth doing?) 
* Need a build system to output producuction build
* Refactor Library to be more generic
* support for tile sheets
* diagetic UI to lobby screen 
* bag box to get paper bags from 
* pill bottle barrel 
* placeholder bottle pill filler


## In Progress

* resolution switcher to settings page 

## Done

* Allow an Entity to pick up / drop an Item
* Fix A* algorithm
* Allow for an Item to be walkable or not (influences Entity movement)
* Add new target entity that moves via arrows keys / alternative keys
* Paint front-facing indicator (another cube, different color face, some shape)
* When Entities change direction, change the front-facing indicator to face the direction they're moving
* When Entities change direction, change the location of any held item to face the direction they're moving
* add diagonal facing direction

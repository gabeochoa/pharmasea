---

kanban-plugin: basic

---

## backlog

- [ ] Create Nav mesh for "walkability"
- [ ] Add some way for entities to subscribe to certain keys so we can more easily keep track of what keys are being requested over lifetime
- [ ] Fix corner walls
- [ ] Upgrade Astar to ThetaStar (worth doing?)
- [ ] support for tile sheets
- [ ] Ailments have OTC solution
- [ ] Ailments have Rx solution
- [ ] Customers who took OTC come back again
- [ ] Different sizes of pill bottle? (S/M/L)
- [ ] Consider using https://github.com/raysan5/rres for resources
- [ ] Particle system?
- [ ] Spritesheet animator
- [ ] unrelated: proggy clean font: https://www.dafont.com/proggy-clean.font


## want for mvp

- [ ] diagetic UI to lobby screen
- [ ] Need a build system to output producuction build
- [ ] Add system for exporting resources to code for easier binary packaging ([see branch packager](https://web.archive.org/web/20210923054249/https://veridisquot.net/singlefilegames.html))
- [ ] Add a pause menu with textual options
- [ ] Create Doors
- [ ] Add "powered by raylib" intro card (and other intro cards) like cat v roomba: https://github.com/raysan5/raylib-games/tree/master/cat_vs_roomba/src
- [ ] Better settings page


## needed to play once

- [ ] In pause menu, remap key bindings in layer for arrows keys to choose options
- [ ] BUG: if customer runs into someone you both get stuck
- [ ] WARN: need a way to warn that UI elements are offscreen "purpling"
- [ ] BUG: Hide pause buttons from non-host since they dont really do anything anyway
- [ ] BUG: When host opens settings, probably dont bring everyone else too
- [ ] button hover state color change


## in progress

- [ ] Support for "windows" or "modals" in ui framework
- [ ] dropdown needs scrollbar when subwindow goes offscreen
- [ ] pill bottle barrel
- [ ] placeholder bottle pill filler
- [ ] Ability to do different shaders between gaming and menu


## blocked

- [ ] highlight furniture under selection


## done

**Complete**
- [x] Magic Enum? https://github.com/Neargye/magic_enum
- [x] change UI from in world to render-to-texture
- [x] Add ailments for customers
- [x] bag box to get paper bags from
- [x] Put all things that have to be loaded in a single place
- [x] resolution switcher to settings page
- [x] Disallow target cube from going outside of bounds
- [x] Refactor Library to be more generic
- [x] add diagonal facing direction
- [x] Add easy-to-use UI buttons
- [x] Better state management for AI
- [x] Allow an Entity to pick up / drop an Item
- [x] Fix A\* algorithm
- [x] Allow for an Item to be walkable or not (influences Entity movement)
- [x] Add new target entity that moves via arrows keys / alternative keys
- [x] Paint front-facing indicator (another cube, different color face, some shape)
- [x] When Entities change direction, change the front-facing indicator to face the direction they're moving
- [x] When Entities change direction, change the location of any held item to face the direction they're moving




%% kanban:settings
```
{"kanban-plugin":"basic"}
```
%%
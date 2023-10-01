---

kanban-plugin: basic

---

## backlog

- [ ] WARN: need a way to warn that UI elements are offscreen "purpling"
- [ ] Add "powered by raylib" intro card (and other intro cards) like cat v roomba: https://github.com/raysan5/raylib-games/tree/master/cat_vs_roomba/src
- [ ] Create Doors
- [ ] Add a pause menu with textual options
- [ ] Add system for exporting resources to code for easier binary packaging ([see branch packager](https://web.archive.org/web/20210923054249/https://veridisquot.net/singlefilegames.html))
- [ ] Create Nav mesh for "walkability"
- [ ] Add some way for entities to subscribe to certain keys so we can more easily keep track of what keys are being requested over lifetime
- [ ] Fix corner walls
- [ ] Upgrade Astar to ThetaStar (worth doing?)
- [ ] support for tile sheets
- [ ] Consider using https://github.com/raysan5/rres for resources
- [ ] Particle system?
- [ ] Spritesheet animator
- [ ] consider switching to https://github.com/graphitemaster/0xABAD1DEA for all of our static globals
- [ ] Look into if its worth using fmod for sound: https://www.fmod.com/
- [ ] Investigate Fiber jobs: http://gdcvault.com/play/1022186/Parallelizing-the-Naughty-Dog-Engine
- [ ] Create / Use a flatmap/flatset for better cache locality on smaller data sets
- [ ] Should we be using arena allocators?


## infra changes

- [ ] collision for player to change to cylinder
- [ ] pressing two movement at the same time while moving camera sometimes feels weird
- [ ] In pause menu, remap key bindings in layer for arrows keys to choose options


## no repro

- [ ] cosmopolitan model is invisible


## design decisions

- [ ] hard to tell that a new machine/stockpile has been spawned in after you get an upgrade
- [ ] Simple Syrup doesnt dissapear after one use and its kinda the only one that does that…
- [ ] During planning its hard to know what each machine it, not obvious
- [ ] penalty if you make too much extra? waste too much ingredients
- [ ] highlight spots on the map where this thing can go
- [ ] need preview for where item will go
- [ ] Text doesnt rotate based on the camera
- [ ] Not clear you can cycle through alcohols
- [ ] guys keep coming back to register. eventually need to add money system or something
- [ ] Should alcohol have to be put back? should it be like the soda / simple syrup
- [ ] add practice mode to learn recipe


## broke

- [ ] default language is reverse which is confusing
- [ ] PS4 Controller touchpad causing “mouse camera rotation”<br>clicking the touchpad & analog stick in the opposite direction cam rotates that way
- [ ] - bug where you cant place the table next to the register<br>- => (i’ve disable bounds checking on placement for now)
- [ ] at round 3 the people got stuck in line as if there was an invis person at the front
- [ ] BUG: Hide pause buttons from non-host since they dont really do anything anyway
- [ ] mojito model is big square
- [ ] having pathfinding crashes when rendering the waiting queue
- [ ] controls dont work for gamepad in settings during game
- [ ] lime doesnt want to go into drink when cup is in register


## complete

**Complete**
- [x] Tell the player how many customers are coming this round
- [x] vomit is broken not working
- [x] Client player cant change settings because menu::State is being overriden by host
- [x] if you take the drink back from the customer you crash
- [x] Automatically teleport new players when joining InRound / Planning etc




%% kanban:settings
```
{"kanban-plugin":"basic"}
```
%%
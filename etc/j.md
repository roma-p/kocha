# 22/12/22 -------------------------------------------------------------------

architecture um up: 


### structs

- game: 
    + defined in game_types but where created? 
    + contains: 
        * app_config structure (not pointer)
        * state structure (pointer)
        * init/update/render/resize.

- application_config
    + defined in application.h
    + contains
        * window position 
        * winddow size
        * window name
    + contained by game struct.

- application_state
    + defined in application.c
    + contains: 
        * status : running / suspended.
        * other data i don't know.
        * all the main struct I guess? 
            - game struct.
            - platform struct
    + staticly defined in application.


### structs

- entry.h : contains main of game. 
    + "create game"         -> from entry.h BUT implementation in game src. / uses game struct.
    + "create application"  -> from aaplication.h 
    + application run       -> from application.h
- application.h / application.c: 
    + "application create"
        * store game in the application structure
        * start platform 
        * init game using func in game struct.
    + "application run" : game loop ! 
        * using game -> update and game -> render


### SUM UP 

app_state (static)
    -> platform
    -> game
        -> main_callbacks 
        -> app_config

main: 
    -> define game inst. 
    -> call "create game" from game exec code source. (will config app_config) 
    -> call app_create
	-> store game_inst to app. 
	-> start platform
	-> init game with user defined func in game inst.

# 16/12/22 -------------------------------------------------------------------

-> log macro: K before it.

platform_macos.m
-> renamed to c (and patched) on livestream from 5 september. 
-> .c created on 17 august.
    -> video of 20 august: no macos.c (at least at 10mins)
    -> added out of stream... no exploantions, best I can do is copy paste stuff.

missing glfw / glm (brew it)

# 15/12/22 -------------------------------------------------------------------

custom makefiles made from various pages... 
minimal requirements are: 
    - 2 makefiles: 1 for engine and 1 for app (unlike How to setup Vulkan)
    - MacOs support but code ready to add support for linux / windows.
    - No extra build.sh build_all.sh
    - compile / link etc... command of Kohi are not ok.


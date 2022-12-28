# 28/12/22 -------------------------------------------------------------------

##vid14: 
-----------------

vulkan layers : validation mlayers, used for debugging. None by default. 
extension and layers: const char**.  -> use of darray.

string.h -> OK 
-> add string_equals (for now wrap strcmp, but later own implementation).

required_extensions: 
    -> VK_KHR_SURFACE_EXTENSION_NAME: used to draw surfaces.
    -> add feature to get platform specific extensions.
	platform_get_required_extension_names(&required_extensions)
    -> if debug -> VK_EXT_DEGU_UTILS_EXTENSION_NAME.
    -> if debug: print all required extensiosn.
    
new file vulkan_platform.h to host get_required_extension_names
!! func has triple pointers !!! 
    -> string is a characeter array, so one *
    -> an array of string is **
    -> a pointer of an array of string is ***
implementation of func is in platform_macos / platform_win_32 etc...
    ... but what if we do'nt use VULKAN?  
in specific implementation -> needs VK_KHR_win32_surface or platform specific extensions.

TODO:

    - missing extension for macos surface (neither macos / metal are found...)
    - print all extensions as debug -> find out where debug is defined.
    - creates_info apple specific to deport in vulkan_platform  / macos platform too.
    - forcing metal or macos surface -> -9





-----------------

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


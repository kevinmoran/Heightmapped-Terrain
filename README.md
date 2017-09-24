# Heightmapped-Terrain
This is my heightmapped terrain viewer and editor, written in C-style C++ (I've seen it called [Nominal C++](http://archive.is/2016.08.07-162105/https://namandixit.github.io/blog/nominal-c++/), [Orthodox C++](https://gist.github.com/bkaradzic/2e39896bc7d8c34e042b), C++--).

[Demo video on Youtube](https://youtu.be/FGm2a1pxgsE)

I started it as a personal side-project to practice game/tool programming. It is very much a work in progress; there's isn't a proper UI yet, everything is done with hotkeys, a good few features are missing. Improved usability is on the roadmap!

[![Screenshot of an example level](https://github.com/kevinmoran/Heightmapped-Terrain/blob/master/Screenshots/HeightmappedTerrainScreenshot1.png)](https://youtu.be/FGm2a1pxgsE)

## Building ## 
This project is built using a Makefile. Run `make` with MinGW installed to build on Windows. It should also compile on Mac (again just run `make`), but I don't currently have an OSX machine so I can't regularly check.

## Features ##

* Loads heightmap from a .pgm image file
* Can add/remove height from terrain by clicking left/right mouse
* Can run around on the terrain as a cube
* Can save edited height data to the .pgm file
* Can render wireframe of level
* Can reload shaders while program is running
* DebugDrawing functions to visualise points and vectors in an Immediate-Mode fashion
* An extremely lightweight OpenGL function loader based on [Apoorva Joshi](https://twitter.com/ApoorvaJ)'s used in [Papaya](https://github.com/ApoorvaJ/Papaya) (Link to his article is at the top of gl_lite.h
* A custom single-header Wavefront .obj mesh loader I wrote (load_obj.h)
  
## Dependencies ##

The wonderful [GLFW library](http://glfw.org).

## Controls ##
Here are the current controls for the editor. These are definitely not final and not very user-friendly at the moment, sorry!

### Edit Mode ###
| Key           | Action        | 
| ------------- |:-------------:| 
| `W`,`A`,`S`,`D`| Move Camera Forward, Left, Backward and Right |
| `Q`, `E`    | Lower/Raise Camera |
| `Arrow Keys` / `Move Mouse` | Turn Camera |
|`M`            | Toggle Mouse/Arrow controls for Camera |
|`Left Click`, `Right Click` | Raise/Lower terrain |
| `Number keys`    | Choose brush (paint, flatten) |

### Play Mode ###

| Key           | Action        | 
| ------------- |:-------------:| 
| `W`,`A`,`S`,`D`| Move Player Forward, Left, Backward and Right |
|`Spacebar`            | Jump |


### General (Both Modes) ###
| Key           | Action        | 
| ------------- |:-------------:| 
| `Ctrl`+`S`    | Save edited terrain to .pgm file |
| `Ctrl`+`R`    | Reload shaders |
| `Ctrl`+`F`    | Toggle Fullscreen |
| `Tab`    | Toggle Edit/Play Mode |
| `/` | Toggle Wireframe view
| `Esc`    | Exit program |


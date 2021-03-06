# Defunct
**The Voxelius as I know and love it is now defunct, the development will continue later in a separate repository.**  
**The source branch `master` compiles but is highly unstable and the code is pretty bloated.**  
**The source branch `rewrite` compiles and works but lacks multiplayer support but the code is much more readable.**

# Voxelius
Voxelius is a work-in-progress FOSS multiplayer voxel-based (actually it will be block-based like Minecraft) game written in C++17.  
My goal is to resemble as much of generic Minecraft gameplay as possible but with some core changes like cubic chunks.  
> Voxelius's multiplayer design is heavily inspired by [OpenBuilder](https://github.com/Hopson97/open-builder) although still different and more Minecraft-like.

## Features
* ECS-based design (some things are managed outside of the ECS because it simply helps to not bloat the code).
* True Modern OpenGL (4.6) so no more binding buffers to write things to them.
* Threaded greedy meshing based on voxel faces.
* Possibly multiplayer support with a Minecraft-like protocol

## Work in progress
You can contribute if you want and I would appreciate that :)  
The current "milestone" is to achieve a correct client-server system and actually draw some chunks sent by the server.

## Screenshots
#### The first ever screenshot
This was in really early stage of development where there was still no greedy meshing.  
![](rd1.png)
#### Pre-network code
![](game/screenshots/2021-09-28_15.15.26.jpg)

# psobb_patches

Patches for Phantasy Star Online: Blue Burst (2004) PC Game.

These patches are designed to work with the [MTethVer12513](https://github.com/anzz1/TethVer12513_Multi/releases/latest) (1.25.13) multilingual client.

To install, simply extract [psobb_patches.zip](https://github.com/anzz1/psobb_patches/releases/latest/download/psobb_patches.zip) archive to the game folder.

## BetterSleep
Significantly lowers CPU usage by replacing the frame limiter's busy loop with a smart sleep algorithm using the [preciseleep](https://github.com/anzz1/precisesleep) technique.

## MoreSaveSlots
Increases the number of save slots from 4 to 12. Configurable to up to 127 slots by changing the [SLOT_COUNT](https://github.com/anzz1/psobb_patches/blob/master/psobb_moresaveslots/dllmain.c#L14) definition and recompiling.
Requires a compatible server such as [newserv](https://github.com/fuzziqersoftware/newserv).

Credits to [fuzziqersoftware](https://github.com/fuzziqersoftware) for the patch.

## WideScreen
Runs the game in borderless fullscreen mode for improved compatibility with modern systems and multiple displays, allows the use of widescreen resolutions, and upgrades DirectX 8 to DirectX 9 for improved performance and shader support.

Credits to [tofuman](https://github.com/tofuman0) for the offsets and [crosire](https://github.com/crosire) for the d3d8to9 project used as the base.

Several post-processing effects are implemented via shaders to improve graphical fidelity.  
These effects can be toggled by editing `widescreen.cfg`.

- **MSAA** (Multisampling Anti-Aliasing)  
Reduces aliasing, smoothing jagged edges.

- **SMAA** (Subpixel Morphological Anti-Aliasing)  
Further reduces aliasing at a subpixel level. More information in the [original research](https://www.iryoku.com/smaa/).

- **SSAO** (Screen Space Ambient Occlusion)  
Improves shadows by occluding ambient light according to the scene geometry.

- **Cel Shading**  
Improves the contrast of models and textures by emphasizing dark lines.

- **Depth of Field**  
Simulates a natural look by making far-away objects appear subtly out-of-focus while in contrast closer objects appear sharper.

- **High Dynamic Range Tone Mapping**  
Adjusts the color range of the scene to darken blacks and brighten whites, to more accurately replicate the deeper colors of a [CRT](https://en.wikipedia.org/wiki/Cathode-ray_tube) display for which the game's art was originally designed for. Colors will appear more vivid and less washed out on a modern LCD display. If you are using a CRT display, you might want to turn this feature off.


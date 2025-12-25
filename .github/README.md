<p align="center">
  <img src="https://github.com/FriskTheFallenHuman/D3ModdingKit/blob/master/.github/branding/logo.png?raw=true" alt="The logo, what else do you expected?"/>
</p>

[![](https://dcbadge.limes.pink/api/server/https://discord.gg/fEehA3hCTg?style=flat)](https://discord.gg/fEehA3hCTg) [![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0) ![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=flat&logo=c%2B%2B&logoColor=white) ![Windows](https://img.shields.io/badge/Windows-0078D6?style=flat&logo=windows&logoColor=white) ![Linux](https://img.shields.io/badge/Linux-FCC624?flat&logo=linux&logoColor=black) ![macOS](https://img.shields.io/badge/mac%20os-000000?style=flat&logo=macos&logoColor=F0F0F0) ![GitHub Repo stars](https://img.shields.io/github/stars/FriskTheFallenHuman/D3ModdingKit?style=flat&logo=Github)

# About

Doom 3 Modding Kit is basically a kit oriented for modders and people that wants to make total conversion mods.



# CHANGES

Compared to the original _dhewm 3_, the changes of _D3Modding Kit_ worth mentioning are:

- fhDoom:
  - Refactor and UI Cleanup of DoomEdit.
  - Added con_size cvar to scale the console.
  - Removal of death code in various places.
  - Dmap timming stats.
- IcedTech / Darkligth:
  - Add MatBuild for mass texture creation.
  - Restore TypeInfoGen (Mostly for standolone games)
- StormEngine2
  - Support for "standard" cubemap side images.
- TEKUUM-D3 / RBDOOM3-BFG:
  - DoomEdit:
    - Window Snapping.
    - Mousewheel Support for cam window.
    - Tool window uses smaller title bar (more screen space for editing)
    - Cam Win Clip Point Manipulation.
    - Rotate Around Selection.
    - New Brush creation in Cam window.
    - Center on Selection.
    - Closing inspector dialog simply hides it.
    - Modified camera control.
    - BSP doesn't hide the Inspector's console.
    - Added Camera Background Color Selection.
    - Selection Centers views on entity.
    - Restore Command List windows.
  - Added Background music support for maps.
  - Added More Console Information.
  - Ambient Music for maps.
- The Dark Mod / Hardcorps
  - Re-implemented water system
- IcedTech3:
  - IceTech3: Fixed Particle Editor crashing when creating a new particle.
  - Improved Multiplayer experience by merging some feedback code.
  - brush_t renamed to idEditorBrush and entity_t idEditorEntity.
  - Brushes will now be a single idRenderModel rather then each side of a brush also having its own render model.
  - Varius Changes for DoomEdit:
    * Added Proper Camera look for camera preview.
    * Surface properties are now accessed from the M key.
    * Camera movement is now based on delta time, and controled by radiant_cameraMoveSpeed.
    * Added grid lines.
  - Fixed a performance were idInteraction would allocate and link for objects that weren't in the light views frustum.
  - Created native versions of all AI_events and then new script functions to call the new native events.
  - Moved Weapon DoomScript native functions so they can be called in native and just in doomscript.
  - Added AnimatedEntity.cpp/h moved that from Entity.cpp/h
  - idAnimState moved to AnimState.cpp/h
  - Seperated idEntity doomscript functions so they can be called in native.
  - Seperated DoomScript from idActor so it can be used with native.
  - Re-Added MayaImport:
    * Supports Maya SDK 2019 up to 2025 (2019 Below is untested)
- com.n0n3m4.diii4a:
  - Quake 4's map base .reverb config files for maps + default .efx for maps that lacks it.
- Original Changes:
  - Support for source engine skyboxes base on StormEngine2 Changes.
  - fixed noSpecular flag for lights Base on a dhewn3 rejected PR.
  - Enable Common Controls theme engine for the tools.
  - D3XP And Game dlls are in one folder.
  - WIP Template base for TC Games.
  - Update The Sys Console to use different colors and font.
  - Set gui_mediumFontLimit and gui_smallFontLimit to "0"
  - Added Missing Material Editor models.
  - Added .guide Support from Quake 4.
  - Refactored MFC Tools.
  - Backported fixes from d3xp and merge missed changes in d3xp/ from base/
  - Backported Water bullet impact from RoE ( see Examples/ folder for the water particle)
  - Made the player be in thirdperson when they die in multiplayer.
  - Merge g_showPlayerShadow to r_shadows (Set r_shadows 2 to see it).
  - Added decl_warn_duplicates so warnings about duplicated entries are hide.
  - Removed TakeNotes system.
  - Added doxygen support.
  - Improved GUIEditor base on the idtech4sdk project.
  - Restored "mphud" keyvalue in player.def
  - Remove unused game over gui file.
  - Added blender model I/O scripts.
- Blood Mod:
  - Blood mod has been integrated into the code base, its base uppon 1.8, its a simplified version since we mainly use stock assets
    - It does not have all of the custom kv but has the essentials that basically upgrades the gib system.
- Doom 3 Redux && Arl's Improvements Mods:
  - Added HD hud assets to be paired with the widescreen .guis

See [Changelog](./CHANGELOG.md) for the original dhewn3 changelog.

## Configuration

See [CONFIGURATION.md](./CONFIGURATION.md) for dhewm3-specific configuration, especially for using gamepads.

## Compiling

Additionally to the [Compiling instructions](./COMPILING.md) we use vcpkg for the handling of the dhewn3 dependencies.

## License

The GNU License can be see here [GNU License](./LICENSE.md) the same goes for the original [DOOM 3 License](./LICENSE_DOOM3.md).

## Special thanks

Special thanks to everybody who did previous help on the making of Dhewm3 and D3Modding Kit:

<p align="center">
  <a href="https://github.com/FriskTheFallenHuman/D3ModdingKit/graphs/contributors">
    <img src="https://contrib.rocks/image?repo=FriskTheFallenHuman/D3ModdingKit" />
  </a>
</p>

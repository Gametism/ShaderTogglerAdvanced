ShaderToggler Advanced

ReShade 5.1+ add-on to toggle game shaders on/off in groups via hotkeys. Configure groups directly in the ReShade overlay.

Note: This toggles game shaders (pixel/vertex/compute), not ReShade effects.

Supported builds
64-bit (x64) – actively maintained (default).

32-bit (Win32) – optional legacy build (if present in Releases/Actions artifacts).

Requirements
ReShade with add-on support (unsigned build).

Get it from https://reshade.me → Download → grey button “with Add-on support”.

ReShade 5.1+ (newer is fine).

Installation
Download ShaderToggler.addon for your game’s bitness:

x64 games: ShaderToggler.addon goes next to the game’s Win64 .exe.

Win32 games: ShaderToggler.addon next to the Win32 .exe (legacy build only).

Place it in the same folder as the ReShade DLL used by the game.

DirectX/OpenGL: usually the game folder with ReShade64.dll (x64) or ReShade.dll (x86).

Vulkan: the ReShade loader can be elsewhere; put the .addon next to the game exe (and install ReShade there too).

Launch the game → open ReShade → Add-ons tab → you should see ShaderToggler.

Unreal Engine often has the real exe under GameName\Binaries\Win64\GameName-Win64-Shipping.exe. Put the .addon (and ReShade) there.

Quick start
Open ReShade → Add-ons → Shader Toggler.

Click New to create a Toggle Group (defaults: name Default, hotkey Caps Lock).

Click Edit to change the name and hotkey (modifiers Alt/Ctrl/Shift supported).

Click Change Shaders to start shader hunting for that group.

Shader hunting controls
After a short frame-gather (configurable), browse active shaders:

Pixel: Numpad 1 / Numpad 2 (mark/unmark with Numpad 3)

Vertex: Numpad 4 / Numpad 5 (mark/unmark with Numpad 6)

Compute: Numpad 7 / Numpad 8 (mark/unmark with Numpad 9)

Hold Ctrl + the browse keys to jump through shaders already in the group.

Test the group at any time by pressing its hotkey.

Click Done when finished.

Saving
Click Save Toggle Group(s) to write ShaderToggler.ini next to the .addon.

The “Is active at startup” option is saved immediately when you tick it (no extra save needed).

Features
Toggle groups with per-group hotkeys.

“Active at startup” (per group) — persists across restarts.

Inline group reordering – ReShade overlay → Group Order section → drag & drop; order auto-saves.

Works across D3D9/10/11/12, OpenGL, Vulkan via ReShade’s add-on API abstraction.

Troubleshooting
Add-on tab doesn’t show ShaderToggler: You’re not using the unsigned (add-on) ReShade build.

Hotkeys don’t toggle anything: Ensure shader hunting added shaders to the group; confirm events are firing by trying another group/hotkey.

Nothing persists: Check write permissions in the game folder; ShaderToggler.ini should be created/updated on save (and immediately when toggling “Active at startup”).

Vulkan title: Confirm the .addon and ReShade are next to the actual executable being launched.

Notes
This add-on toggles game shaders, not ReShade effects.

For Unreal/other multi-exe games, place files next to the shipping exe under Binaries/Win64 (or Win32).

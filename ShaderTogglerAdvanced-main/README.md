# ShaderToggler Advanced

ShaderToggler Advanced is a ReShade 5.1+ add-on for toggling **game shaders** on and off in groups with a single hotkey.  
It is designed for **game shaders only**, not ReShade effects.

Originally created by **Frans "Otis_Inf" Bouma**.  
Modified and expanded by **Sven "Gametism" Königsmann**.

This version adds:
- a modernized UI
- active-at-startup support for x86
- group duplication
- group reordering
- improved hotkey handling
- accelerated shader browsing while holding keys
- controller button support
- hold-based activation modes
- auto-hide (timed) toggle groups
- inverted auto-hide behavior
- multiple timed trigger keys with configurable trigger modes
- a global hotkey modifier system for all defined hotkeys
- enhanced shader and HUD detection behavior for DX9, DX11, DX12, Vulkan, DXVK, and RenoDX
- sorting functions
- **timed suppression keys for timed groups**

---

## How to use

Place `ShaderToggler.addon64` or `ShaderToggler.addon` in the same folder as the **game executable**.  
In most cases, this is also the folder where the ReShade DLL is located.

For **Vulkan** games, the ReShade DLL may be located elsewhere depending on the setup.

For some **Unreal Engine** games, there may be two executables:
- one in the root game folder
- one deeper inside a path like  
  `GameName\Binaries\Win64\GameName-Win64-Shipping.exe`

In those cases, ShaderToggler has to be placed in the **actual executable folder**, for example:

`GameName\Binaries\Win64`

ReShade must also be installed in that same folder.

Make sure you are using the **ReShade version with Add-on support**.

When the game starts, open the ReShade overlay and go to the **Add-ons** tab.  
You should see the **Shader Toggler** panel and its controls there.

---

## Creating a toggle group

To create a new shader toggle group:

1. Open the ReShade overlay
2. Go to the **Shader Toggler** section
3. Click **New**

A new group is created with:
- the default name `Default`
- the default hotkey `Caps Lock`

To change these:
- click **Edit**
- rename the group
- assign a different hotkey

Hotkeys do **not** need to be unique.  
Group names also do **not** need to be unique.

A hotkey can be:
- a keyboard key
- a mouse button
- a supported controller button
- optionally combined with `Alt`, `Ctrl`, or `Shift` for keyboard bindings

Each group can also be set to:
- **Active at startup**
- **Hold mode**
- **Auto-hide mode (timed)**

You can also:
- duplicate groups
- reorder groups with drag and drop
- save all groups to the config file

---

## Global hotkey modifier

ShaderToggler Advanced includes a **global hotkey modifier** setting that can be applied to all defined keyboard and mouse hotkeys at runtime.

Available options:
- `None`
- `Ctrl`
- `Alt`
- `Shift`
- `Ctrl + Alt`
- `Ctrl + Shift`
- `Alt + Shift`
- `Ctrl + Alt + Shift`

This is useful if:
- you want safer default hotkeys
- you want to reduce accidental triggers
- you do not want to manually rebind every group after updates

### How it works
The selected modifier is applied **at runtime only**.

That means:
- your stored hotkeys stay exactly the same in the config
- the modifier is simply required on top of the existing binding
- no manual rebinding is needed

### Example
If a group hotkey is:
- `Caps Lock`

and the global modifier is set to:
- `Ctrl`

then the effective hotkey becomes:
- `Ctrl + Caps Lock`

If a hotkey is already:
- `Alt + F1`

and the global modifier is:
- `Ctrl`

then the effective hotkey becomes:
- `Ctrl + Alt + F1`

### Important note
The global modifier applies to:
- keyboard hotkeys
- mouse hotkeys

It does **not** change:
- gamepad button bindings

---

## Activation modes

ShaderToggler Advanced supports three main ways to control a group:

### 1. Toggle mode (default)
- Press key → ON
- Press again → OFF

This is the classic behavior.

---

### 2. Hold mode
- Group is active **only while holding the key**

Optional:
- **Invert hold** → active by default, turns OFF while holding

---

### 3. Auto-hide mode (Timed Mode)
Timed mode temporarily changes a group state when one of its trigger keys is used.

You can configure:
- hide delay / show time
- minimum visible time
- fade-out linger
- inverted auto-hide behavior
- multiple timed trigger keys
- trigger mode per trigger key
- timed suppression keys
- suppression release delay

---

## Timed mode trigger behavior

Each timed trigger key can use one of these modes:

### On press
The timed state starts only when the key is pressed.

### While held
The timed state keeps refreshing while the key is held.

### Press + hold
The timed state starts on press and also keeps refreshing while the key remains held.

---

## Timed suppression keys

Timed suppression keys allow you to temporarily block timed activation behavior for a specific group.

This feature is designed for situations where:
- a timed trigger is reused for multiple gameplay actions
- some actions should reveal or hide the HUD/effect
- other actions should temporarily suppress the timed behavior

Timed suppression only affects:
- timed groups
- auto-hide behavior
- timed visibility logic

It does **not** affect:
- normal toggle mode
- hold mode
- manual activation
- other groups

Each group now handles suppression independently.

That means:
- suppression keys only affect the group where they are configured
- suppression no longer cancels unrelated timed groups

---

## Suppression release delay

Modern controllers — especially analog triggers — often release slightly slower than face buttons.

Example:
- `RT + Y`
- you release both at the same time
- `Y` releases immediately
- `RT` may still report as pressed for a few milliseconds

Without protection, this could accidentally trigger timed behavior immediately after releasing the suppression button.

ShaderToggler Advanced now includes:
- an internal suppression release delay system

### What it does
When a suppression key is released:
- suppression remains active briefly
- timed triggers stay blocked during that short grace period

This prevents:
- accidental HUD flickering
- unwanted auto-show triggers
- trigger/button desync problems
- unreliable suppression on controllers

The delay is intentionally short and automatic.

No manual setup is required.

---

## Practical example

### Goal
Hide a combat HUD effect while holding:
- `RT`

But prevent that behavior during:
- `RT + Y`
- `RT + B`
- `RT + X`
- `RT + A`

### Setup
Timed trigger:
- `RT`

Suppression keys:
- `A`
- `B`
- `X`
- `Y`

### Result
- holding `RT` alone:
  - timed behavior activates normally

- using:
  - `RT + Y`
  - `RT + B`
  - `RT + X`
  - `RT + A`

  suppression activates and blocks the timed behavior.

When releasing both buttons:
- suppression briefly lingers
- accidental trigger activation is prevented

This makes controller behavior significantly more reliable.

---

## Marking shaders

To mark shaders successfully, make sure the element you want to hide is currently visible.

Then click **Hunt Shaders** on the group you want to configure.

ShaderToggler Advanced uses an enhanced shader hunting system that combines:
- traditional shader hash hunting
- internal draw fingerprint analysis
- improved HUD draw detection

This improves detection accuracy for:
- HUD elements
- interaction prompts
- UI backgrounds
- layered UI widgets
- DX12 HUD systems
- Vulkan HUD systems
- Unreal Engine HUD rendering

Especially for difficult modern games where:
- many UI elements share the same shader
- shaders are heavily reused
- draw calls are batched aggressively

The enhanced detection system works automatically in the background during normal shader hunting.

---

## Shader hunting hotkeys

### Pixel shaders
- `Numpad 1` = previous pixel shader
- `Numpad 2` = next pixel shader
- `Numpad 3` = mark / unmark current pixel shader

### Vertex shaders
- `Numpad 4` = previous vertex shader
- `Numpad 5` = next vertex shader
- `Numpad 6` = mark / unmark current vertex shader

### Compute shaders
- `Numpad 7` = previous compute shader
- `Numpad 8` = next compute shader
- `Numpad 9` = mark / unmark current compute shader

---

## Marked shader browsing

### Pixel shaders
- `Ctrl + Numpad 1 / 2`
  = previous / next marked pixel shader

### Vertex shaders
- `Ctrl + Numpad 4 / 5`
  = previous / next marked vertex shader

### Compute shaders
- `Ctrl + Numpad 7 / 8`
  = previous / next marked compute shader

---

## Accelerated browsing

Holding down browsing keys will automatically accelerate scrolling speed over time.

This works for:
- shader browsing
- marked shader browsing

This makes large shader hunts significantly faster.

---

## Testing a group

To test a group while hunting:
- press the hotkey assigned to that group

If the selected shaders are correct, the targeted game elements should disappear when the group is active.

When you are finished hunting shaders for a group, click **Done**.

---

## Saving your groups

To keep your groups for the next session, click:
- **Save all Toggle Groups**

This writes the configuration to:

`ShaderToggler.ini`

The file is stored in the same folder as the add-on.

---

## Notes

- ShaderToggler Advanced is for game shader toggling, not ReShade effect toggling
- The UI has been modernized while keeping the original workflow familiar
- Controller button mapping support is available
- Timed mode supports inverted behavior for intuitive HUD auto-show setups
- Multiple timed trigger keys let one group react to keyboard, mouse, and controller input simultaneously
- Timed suppression keys now operate independently per group
- Internal suppression release delay improves controller reliability

---

## Credits

**Original ShaderToggler:**  
Frans "Otis_Inf" Bouma

**ShaderToggler Advanced modifications:**  
Sven "Gametism" Königsmann
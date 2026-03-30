# ShaderToggler Advanced

ShaderToggler Advanced is a ReShade 5.1+ add-on for toggling **game shaders** on and off in groups with a single hotkey.
It is designed for **game shaders only**, not ReShade effects.

Originally created by **Frans "Otis_Inf" Bouma**.
Modified and expanded by **Sven "Gametism" Königsmann**.

This version adds:

* a modernized UI
* active-at-startup support for x86
* group duplication
* group reordering
* improved hotkey handling
* accelerated shader browsing while holding keys
* controller button support
* **hold-based activation modes**
* **auto-hide (timed) toggle groups**

---

## How to use

Place `ShaderToggler.addon64` or `ShaderToggler.addon` in the same folder as the **game executable**.
In most cases, this is also the folder where the ReShade DLL is located.

For **Vulkan** games, the ReShade DLL may be located elsewhere depending on the setup.

For some **Unreal Engine** games, there may be two executables:

* one in the root game folder
* one deeper inside a path like
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

* the default name `Default`
* the default hotkey `Caps Lock`

To change these:

* click **Edit**
* rename the group
* assign a different hotkey

Hotkeys do **not** need to be unique.
Group names also do **not** need to be unique.

A hotkey can be:

* a keyboard key
* a mouse button
* a supported controller button
* optionally combined with `Alt`, `Ctrl`, or `Shift` for keyboard bindings

Each group can also be set to:

* **Active at startup**
* **Hold mode**
* **Auto-hide mode (timed)**

You can also:

* duplicate groups
* reorder groups with drag and drop
* save all groups to the config file

---

## Activation modes

ShaderToggler Advanced now supports three different ways to control a group:

### 1. Toggle mode (default)

* Press key → ON
* Press again → OFF

This is the classic behavior.

---

### 2. Hold mode

* Group is active **only while holding the key**

Optional:

* **Invert hold** → active by default, turns OFF while holding

This is useful for:

* ADS overlays
* temporary HUD visibility
* quick comparison setups

---

### 3. Auto-hide mode (Timed Mode)

* Press key → group activates
* After a delay → group automatically deactivates

You can configure:

* **Hide delay (100 ms – 10 seconds)**

This enables:

* **HUD auto-hide behavior**
* "show UI on input" systems like modern games

Example use case:

* Press attack → HUD appears
* After 1.5 seconds → HUD disappears automatically

---

## Important limitation (Why no fade-out?)

A real **visual fade-out is NOT possible** with ShaderToggler.

**Reason:**

* ShaderToggler works by **blocking draw calls at the pipeline level**
* When a shader is disabled → it is **not rendered at all**
* There is **no access to opacity, blending, or animation states**

So the system can only do:

* **Fully visible (draw call allowed)**
* **Fully hidden (draw call blocked)**

It cannot:

* interpolate alpha
* animate transitions
* gradually fade elements

A fade-out would require:

* modifying shader code
* injecting custom blending logic
* or post-processing the specific UI elements

Which is outside the scope of this add-on.

The new **Auto-hide mode** is designed as a practical alternative:

* instant show
* delayed hide
* mimics modern game UI behavior without needing shader edits

---

## Marking shaders

To mark shaders successfully, make sure the element you want to hide is currently visible, for example:

* HUD elements
* menus
* overlays
* visual effects

Then click **Hunt Shaders** on the group you want to configure.

This starts the shader hunting phase for that group.

During this phase, ShaderToggler shows an overlay at the top of the screen with shader information.
The overlay opacity can be adjusted in the settings, including setting it to **fully invisible** if needed.

Before browsing shaders, the add-on first collects active shaders for a configurable number of frames.
This reduces the number of shaders you have to go through and makes hunting more practical.

---

## Shader hunting hotkeys

### Pixel shaders

* `Numpad 1` = previous pixel shader
* `Numpad 2` = next pixel shader
* `Numpad 3` = mark / unmark current pixel shader

### Vertex shaders

* `Numpad 4` = previous vertex shader
* `Numpad 5` = next vertex shader
* `Numpad 6` = mark / unmark current vertex shader

### Compute shaders

* `Numpad 7` = previous compute shader
* `Numpad 8` = next compute shader
* `Numpad 9` = mark / unmark current compute shader

### Marked shader browsing

* `Ctrl + Numpad 1 / 2` = previous / next marked pixel shader
* `Ctrl + Numpad 4 / 5` = previous / next marked vertex shader
* `Ctrl + Numpad 7 / 8` = previous / next marked compute shader

### Accelerated holding

Holding down browsing keys will automatically speed up scrolling over time.

---

## Testing a group

To test a group while hunting:

* press the hotkey assigned to that group

If the selected shaders are correct, the targeted game elements should disappear when the group is active.

When a group is active, the UI shows it as **active**.

When you are finished hunting shaders for a group, click **Done**.

---

## Saving your groups

To keep your groups for the next session, click:

* **Save all Toggle Groups**

This writes the configuration to:

`ShaderToggler.ini`

The file is stored in the same folder as the add-on.

---

## Notes

* ShaderToggler Advanced is for **game shader toggling**, not ReShade effect toggling
* The UI has been modernized while keeping the original workflow familiar
* Controller button mapping support is available
* Auto-hide mode enables modern UI behavior without modifying shaders

---

## Credits

**Original ShaderToggler:**
Frans "Otis_Inf" Bouma

**ShaderToggler Advanced modifications:**
Sven "Gametism" Königsmann

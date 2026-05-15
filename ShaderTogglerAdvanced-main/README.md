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
- improved shader detection behavior for DXVK and RenoDX (DX9)
- sorting functions
- **timed suppression keys for timed groups**
- **draw-call fingerprint based blocking**

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

**Example:**  
You want to manually hide a minimap:
- press key once → minimap disappears
- press again → minimap returns

---

### 2. Hold mode
- Group is active **only while holding the key**

Optional:
- **Invert hold** → active by default, turns OFF while holding

This is useful for:
- ADS overlays
- temporary HUD visibility
- quick comparison setups

**Example (normal hold):**
- hold key → scoped overlay hidden
- release key → scoped overlay returns

**Example (invert hold):**
- overlay is normally hidden
- hold key → overlay becomes visible only while held

---

### 3. Auto-hide mode (Timed Mode)
Timed mode temporarily changes a group state when one of its trigger keys is used.

You can configure:
- **Hide delay / show time**
- **Minimum visible time**
- **Fade-out linger**
- **Invert auto-hide behavior**
- multiple trigger keys
- trigger mode per trigger key
- timed suppression keys

---

## Timed mode trigger behavior

Each timed trigger key can use one of these modes:

### On press
The timed state starts only when the key is pressed.

**Example:**  
Press attack once → HUD appears briefly → then hides again

### While held
The timed state keeps refreshing while the key is held.

**Example:**  
Hold aim button → timer keeps refreshing as long as the button is held

### Press + hold
The timed state starts on press and also keeps refreshing while the key remains held.

**Example:**  
Press and keep holding a gamepad trigger → HUD stays visible longer and only hides after release + delay

---

## Understanding how timed mode actually works

This part is very important.

ShaderToggler does **not** “show” or “hide” HUD elements directly in the usual sense.  
It only controls whether the selected game shaders are **allowed to render** or **blocked**.

So for a hunted HUD group:

- **Group ACTIVE** = selected shaders are blocked = HUD hidden
- **Group INACTIVE** = selected shaders are allowed = HUD visible

Because of that, timed mode may appear inverted at first depending on how your group is set up.

---

## Normal auto-hide mode

### Behavior
When triggered:
- the group temporarily becomes **active**
- after the timer ends, it returns to **inactive**

This is useful when:
- `active = hidden` is **not** what you want for the resting state
- or when the group should only block something briefly

**Example:**
- HUD is visible
- press trigger
- HUD disappears
- after delay, HUD returns

This means your group is behaving like a temporary **hide group**.

---

## Inverted auto-hide behavior

### Behavior
When triggered:
- the group temporarily becomes **inactive**
- after the timer ends, it returns to **active**

This is the most useful mode for modern auto-hide HUD behavior.

### Typical setup
For a HUD hide group:
- hunt the HUD shaders normally
- confirm that `group active = HUD hidden`
- enable:
  - **Active at startup**
  - **Auto-hide mode**
  - **Invert auto-hide behavior**

### Result
- game starts → group is active → HUD hidden
- press trigger → group turns inactive temporarily → HUD appears
- timer ends → group turns active again → HUD hides

---

## Practical timed mode examples

### Example 1: Modern auto-hide HUD
**Goal:**
- HUD hidden by default
- attack input reveals HUD briefly
- HUD hides again after a delay

**Setup:**
- hunt HUD shaders
- make sure `group active = HUD hidden`
- enable **Active at startup**
- enable **Auto-hide mode**
- enable **Invert auto-hide behavior**
- add attack key as timed trigger
- choose delay, for example `1500 ms`

**Result:**
- HUD hidden normally
- press attack → HUD appears
- after 1.5 seconds → HUD disappears again

---

### Example 2: Mouse and controller support together
**Goal:**
- show the HUD whether the player uses mouse or controller

**Setup:**
- same as above
- add multiple timed triggers:
  - `Left Mouse Button`
  - `Gamepad RT`

**Result:**
- either trigger reveals the HUD
- timer refreshes when either input is used

---

### Example 3: Keep HUD visible during continuous combat
**Goal:**
- HUD stays visible while continuously attacking

**Setup:**
- use **While held** or **Press + hold** for the trigger
- set a short delay, for example `500 ms`

**Result:**
- repeated or held input keeps the HUD visible
- once input stops, HUD hides after the delay

---

### Example 4: Temporary tooltip / pickup prompt suppression
**Goal:**
- briefly hide a specific overlay when a key is pressed

**Setup:**
- group represents the overlay you want to suppress
- use normal timed mode
- do **not** invert it

**Result:**
- press trigger → overlay disappears briefly
- after the timer ends → overlay returns

---

## Timed suppression keys

Timed suppression keys allow you to temporarily prevent a timed group from activating while specific buttons or keys are pressed.

This feature is designed mainly for:
- combat abilities
- alternate attacks
- contextual actions
- special moves
- situations where the normal timed trigger should temporarily be ignored

Timed suppression only affects:
- **Auto-hide mode / Timed mode groups**

It does **not** affect:
- normal toggle mode
- hold mode

---

## Why this feature exists

Some games reuse the same trigger button for:
- normal gameplay actions
- special abilities
- contextual interactions

Without suppression keys, this can create problems.

### Example
You use:
- `RT` as the timed trigger to reveal a hidden HUD or disable a heavy visual effect

But the game also uses:
- `RT + Y`
- `RT + B`
- `RT + X`
- `RT + A`

for elemental abilities or special attacks.

In that situation, the timed trigger would normally activate even during those abilities.

That can cause:
- important VFX to disappear
- combat readability problems
- unwanted HUD behavior
- incorrect temporary shader blocking

Timed suppression keys solve this.

---

## How timed suppression works

When any suppression key is currently held:
- timed activation is blocked
- existing timed visibility is cancelled immediately
- fade-out linger timers are reset
- the group returns to its resting state

The suppression keys only affect timed behavior.

The group itself still exists normally and can still:
- toggle manually
- use hold mode
- use startup behavior
- use normal shader blocking

---

## Practical example: Crimson Desert

### Goal
Disable the heavy RT combat visual effect during normal attacks, but keep elemental skill effects visible.

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
  - heavy combat effect gets disabled

- using:
  - `RT + Y`
  - `RT + B`
  - `RT + X`
  - `RT + A`

  suppression activates and prevents the timed group from triggering.

This allows:
- elemental powers
- skill effects
- special attacks

to remain fully visible.

---

## Additional use cases

### Prevent HUD reveal during menus
Suppress HUD auto-show while:
- inventory button
- map button
- radial menu button

is held.

### Prevent effect removal during ultimate abilities
Keep cinematic VFX active only during:
- super attacks
- finishers
- special states

### Context-sensitive UI behavior
Prevent temporary shader blocking during:
- lockpicking
- dialogue wheels
- interaction prompts
- photo mode

---

## Important behavior notes

Suppression keys:
- are checked continuously
- work instantly
- override timed triggers
- can use keyboard, mouse, or controller buttons

If multiple suppression keys are defined:
- pressing **any** of them activates suppression

Timed suppression is evaluated before timed activation logic.

That means suppression always has priority over timed triggers.

---

## Timed mode options explained

### Hide delay / show time
This is the main timed duration.

- in normal timed mode: how long the group stays temporarily **active**
- in inverted timed mode: how long the group stays temporarily **inactive**

For HUD auto-show behavior, this is effectively **how long the HUD stays visible**.

### Minimum visible time
Prevents the temporary timed state from ending too quickly.

This is useful when:
- fast taps would otherwise make the HUD flash too briefly
- you want a more readable UI reveal

**Example:**  
You tap attack very quickly, but still want the HUD to remain visible for at least `300 ms`.

### Fade-out linger
This is **not a real visual fade**.  
It is simply an additional linger stage before the group returns to its resting state.

It helps make the transition feel less abrupt in timing, but it does **not** animate alpha or transparency.

---

## Important limitation (Why no real fade-out?)

A real **visual fade-out is NOT possible** with ShaderToggler.

**Reason:**
- ShaderToggler works by **blocking draw calls at the pipeline level**
- When a shader is disabled, it is **not rendered at all**
- There is **no access to opacity, blending, or animation states**

So the system can only do:
- **Fully visible** (draw call allowed)
- **Fully hidden** (draw call blocked)

It cannot:
- interpolate alpha
- animate transitions
- gradually fade elements

A real fade-out would require:
- modifying shader code
- injecting custom blending logic
- or post-processing those exact UI elements

Which is outside the scope of this add-on.

### What “Fade-out linger” actually means
It does **not** fade the image visually.  
It only adds a short extra timed phase before returning to the resting state.

So it behaves more like:
- visible
- linger a bit longer
- hidden

Not:
- 100% opacity
- 75%
- 50%
- 25%
- 0%

---

## Marking shaders

To mark shaders successfully, make sure the element you want to hide is currently visible, for example:
- HUD elements
- menus
- overlays
- visual effects

Then click **Hunt Shaders** on the group you want to configure.

This starts the shader hunting phase for that group.

During this phase, ShaderToggler shows an overlay at the top of the screen with shader information.  
The overlay opacity can be adjusted in the settings, including setting it to **fully invisible** if needed.

Before browsing shaders, the add-on first collects active shaders for a configurable number of frames.  
This reduces the number of shaders you have to go through and makes hunting more practical.

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

### Marked shader browsing
- `Ctrl + Numpad 1 / 2` = previous / next marked pixel shader
- `Ctrl + Numpad 4 / 5` = previous / next marked vertex shader
- `Ctrl + Numpad 7 / 8` = previous / next marked compute shader

### Accelerated holding
Holding down browsing keys will automatically speed up scrolling over time.

---

## Testing a group

To test a group while hunting:
- press the hotkey assigned to that group

If the selected shaders are correct, the targeted game elements should disappear when the group is active.

When a group is active, the UI shows it as **active**.

When you are finished hunting shaders for a group, click **Done**.

---

## Saving your groups

To keep your groups for the next session, click:
- **Save all Toggle Groups**

This writes the configuration to:

`ShaderToggler.ini`

The file is stored in the same folder as the add-on.

---

## Fingerprint hunting

Fingerprint hunting is designed mainly for:

HUD elements
prompts
text overlays
interaction icons
UI backgrounds
difficult modern interfaces
How it works

Start fingerprint hunting from a toggle group.

The system collects visible draw-call fingerprints from recent frames and lets you cycle through them individually.

When cycling:

- matching UI elements disappear temporarily
- allowing you to identify the correct fingerprint

Once identified:

mark the fingerprint
it becomes part of the current toggle group

Marked fingerprints are highlighted in yellow in the overlay.

Fingerprint hunting controls

### Fingerprint browsing

Numpad 0 = previous fingerprint
Numpad Decimal = next fingerprint
Numpad + = mark / unmark current fingerprint

### Marked fingerprint browsing

Ctrl + Numpad 0 / Numpad Decimal = previous / next marked fingerprint

Accelerated holding

Holding fingerprint browsing keys speeds up scrolling automatically over time.

This makes large HUD fingerprint sets much faster to navigate.

---

## Notes

- ShaderToggler Advanced is for **game shader toggling**, not ReShade effect toggling
- The UI has been modernized while keeping the original workflow familiar
- Controller button mapping support is available
- Timed mode now supports inverted behavior for more intuitive HUD auto-show setups
- Multiple timed trigger keys let one group react to keyboard, mouse, and controller input at the same time
- Timed suppression keys allow contextual blocking of timed activation behavior
- The global hotkey modifier lets users make all defined keyboard and mouse hotkeys safer without rebinding them individually
- Timed suppression keys allow contextual blocking of timed activation behavior

---

## Credits

**Original ShaderToggler:**  
Frans "Otis_Inf" Bouma

**ShaderToggler Advanced modifications:**  
Sven "Gametism" Königsmann
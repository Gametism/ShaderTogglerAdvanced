# ShaderToggler Advanced

**Version 1.5.0.0**

ShaderToggler Advanced is a ReShade add-on for toggling game shaders and selected draw calls in groups. Use it to hide HUD elements or visual effects, assign keyboard, mouse or controller hotkeys, and build automatic visibility rules.

Originally created by **Frans "Otis_Inf" Bouma**. Modified and expanded by **Sven "Gametism" Königsmann**.

## Contents

- [Installation](#installation-and-updating), [quick start](#quick-start-which-tool-should-i-use), [interface](#compact-interface) and [sizing](#interface-size-and-readability).
- [Creating groups](#creating-and-editing-groups), [shader hunting](#shader-hunting) and [Smart disable](#smart-disable).
- [Refining shared effects](#refine-separate-effects-that-share-shaders) and [standalone Effect Finder](#standalone-effect-finder).
- [Merging](#merging-groups), [ordering](#reordering-and-sorting), [hotkeys](#hotkeys-and-controller-labels), [activation modes](#activation-modes), [timed controls](#timed-triggers-suppression-and-linger) and [global suspension](#suspend-all-toggle-groups).
- [Saving](#saving-and-compatibility), [diagnostics](#diagnostic-logging), [troubleshooting](#troubleshooting), [licensing](#credits-and-licensing).

## Features

- Pixel, vertex and compute shader hunting, with accelerated browsing.
- Original shader matching enabled by default for compatibility with existing toggles.
- **Smart disable** with suggested methods, colour previews and saved choices for DirectX 9, 10, 11, 12, Vulkan and OpenGL.
- **Refine** to narrow an existing toggle to individual calls when a shader controls several effects.
- Paused-scene refinement and a standalone **Effect Finder** with paused or guided live captures.
- Saved precise filters that follow the group's normal hotkey and activation settings.
- Group duplication and merging, including refined groups.
- Direct drag-and-drop ordering in the main list, plus automatic sorting.
- A compact interface with brighter text, outlined buttons, clear state badges, search and filters.
- Automatic sizing for high-resolution displays and manual sizes from 100% to 250%.
- Keyboard, mouse and supported controller hotkeys; Xbox and PlayStation button labels.
- Toggle, hold, inverted hold, timed and inverted timed activation.
- Multiple timed triggers, suppression keys and suppression linger.
- Active at Startup, including an optional startup duration, on x86 and x64.
- Global hotkey modifiers and Suspend All Toggle Groups.
- Per-group notices and diagnostic logging.

ShaderToggler Advanced controls **game shaders**, not ReShade post-processing effects. Available operations depend on the game, graphics API and captured rendering state.

## Installation and updating

Use **ReShade 5.1 or newer with full Add-on support**, installed for the game.

| Game executable | Add-on file | Compiler supplied with the source package |
| --- | --- | --- |
| 64-bit | **ShaderTogglerAdvanced.addon64** | **Runtime/x64/dxcompiler.dll** |
| 32-bit | **ShaderTogglerAdvanced.addon** | **Runtime/x86/dxcompiler.dll** |

1. Close the game.
2. Place the matching add-on in the folder containing the actual game executable.
3. For DXIL Smart disable support, also place the matching **dxcompiler.dll** there. The build script supplies it beside each built add-on.
4. Keep your existing **ShaderToggler.ini**.
5. Start the game, open ReShade and select **Add-ons → Shader Toggler Advanced**.

When updating from the original ShaderToggler, remove its old add-on binary so only one version loads. The configuration filename remains **ShaderToggler.ini**; it has not changed to ShaderTogglerAdvanced.ini.

Some Unreal Engine games launch an executable inside **GameName/Binaries/Win64**, rather than the executable in the root folder. Use the folder containing the running game executable. Vulkan's ReShade loader location can differ from a normal DirectX installation.

If the game already supplies its own **dxcompiler.dll**, keep that file and check whether it works before replacing a game dependency. A separate **dxil.dll** is not required for this add-on's bundled compiler path. Include **DXC-LICENSE.txt** when distributing the supplied compiler.

### Configuration and log files

Both files are written beside the game executable:

| File | Purpose |
| --- | --- |
| **ShaderToggler.ini** | Groups, hotkeys, activation settings, saved Smart disable methods, refined filters and interface preferences |
| **ShaderTogglerAdvanced.log** | Diagnostics for the current run; overwritten on the next launch |

Existing ShaderToggler configurations remain supported. Back up the INI before making large changes or testing a new setup.

## Quick start: which tool should I use?

| Situation | Tool |
| --- | --- |
| Find a shader that hides a HUD element or effect | **Hunt** |
| Ordinary shader disabling leaves bloom burn-in, stale output or another unwanted result | **Smart disable** |
| A toggle removes the trail but also removes fire, smoke or other wanted effects | **Refine** |
| Search for an effect without an existing shader group | **Advanced tools → Effect Finder** |
| Combine several working refined groups under one hotkey | **Merge** |

Smart disable changes **how a selected pixel shader is disabled**. Refine changes **which matching calls a group disables**. A colour replacement alone does not separate effects that share a shader.

## Compact interface

The add-on has four tabs:

| Tab | Contents |
| --- | --- |
| **Groups** | Group list, creation, editing, merging, sorting, search and filters |
| **Settings** | Hunting preferences, controller labels, global modifiers and suspension controls |
| **Advanced tools** | Standalone Effect Finder |
| **Help** | Hunting shortcuts and diagnostic information |

**Save all** and **Size** remain at the top. Active hunting, Smart disable and refinement controls appear above the tabs so they remain accessible.

Each group occupies one compact row:

- A dotted drag handle.
- A coloured state badge.
- The group name, with **[!]** when it has a notice.
- Its hotkey and shader/filter counts when space permits.
- **Hunt**, **Refine**, **Edit** and **...** actions.

In a narrow panel, additional actions remain available under **...**. Long names are shortened visually; hover the name to see its full text, notice, hotkey and counts. Click the name or **Edit** to open its settings inline.

**S** means whole-shader rules. **F** means saved precise effect filters. A combined count indicates that the group contains both.

### What ON, OFF and SUS mean

| Badge | Meaning |
| --- | --- |
| Green **ON** | The group is active: its selected shaders or matching calls are disabled using the applicable method |
| Slate **OFF** | The group is inactive: its rules allow those shaders or calls |
| Amber **SUS** | All groups are temporarily suspended |

For a typical HUD-hiding group, **ON means the HUD is hidden**. It does not mean the HUD itself is switched on.

### Search and filters

Search matches group names, notices and hotkeys. **Filter** can show all groups, refined groups or active groups. Groups currently being edited, hunted or used as the active finder destination remain visible.

Switching tabs cancels a pending hotkey capture. It does not end an active hunting or finder session.

## Interface size and readability

Use **Size** to choose **Auto**, **100%**, **125%**, **150%**, **175%**, **200%** or **250%**.

Auto enlarges small fonts towards these targets:

| Output resolution | Auto target |
| --- | --- |
| 1080p | 16 px |
| 1440p | 22 px |
| 4K | 32 px |

Auto uses the output's shorter dimension, keeps an already larger ReShade font and limits enlargement to 250%. Manual percentages multiply the current ReShade font size. The menu shows the **actually applied percentage and pixel size**.

Text, controls and hunting/finder overlays scale together. Brighter text, outlined buttons and clear hover states improve readability while preserving tight row spacing. Secondary columns hide when necessary to keep names and actions usable.

The choice saves automatically as **UIScalePercent** in the INI's **[General]** section; **0** selects Auto. It applies on the next overlay update. The add-on restores ReShade's font and style after drawing its interface.

## Creating and editing groups

1. Open **Groups** and click **New**.
2. Give the group a name.
3. Assign its hotkey.
4. Use **Hunt** to select the shaders it should control.
5. Finish hunting and click **Save all**.

New groups start as **Default**, with **Caps Lock** as their default hotkey. Group names and hotkeys do not have to be unique. Groups sharing a hotkey can activate together.

Use the hotkey field in **Edit** to capture a binding and confirm it with **OK**. Keyboard bindings can include Ctrl, Alt or Shift. Mouse bindings support Left, Right, Middle, Mouse 4 and Mouse 5. Supported controller buttons can also be assigned.

For mouse capture, follow the UI prompt: release the opening click, move away from the field, then press the mouse button to bind.

**Save group** saves and closes the inline editor. **Save all** saves the complete configuration. Hover **Save all** to see the last save result.

### Notices

Add an optional **Notice** in the group editor to document a game-specific caveat without making the name excessively long. For example:

    Name=DOF
    Notice=Also affects some cutscenes.

The group row displays **[!]**, and the notice appears in its tooltip.

### Duplication

Choose **... → Duplicate** to copy a group, including its saved precise filters. Check the duplicate's hotkey and activation settings before using it alongside the original.

## Shader hunting

Make the target HUD element or effect visible, then click **Hunt** on the group. The add-on first collects shaders for the configured number of frames.

Under **Settings → Hunting and controls**:

- **Frames to collect** controls the collection period. Increase it for effects that appear only occasionally.
- **Overlay opacity** controls the hunting status overlay, including making it invisible.
- **Use original shader matching** preserves the original add-on's matching behaviour.

After collection, browse and mark the shaders that affect your target:

| Shader type | Previous | Next | Mark / unmark |
| --- | --- | --- | --- |
| Pixel | Numpad 1 | Numpad 2 | Numpad 3 |
| Vertex | Numpad 4 | Numpad 5 | Numpad 6 |
| Compute | Numpad 7 | Numpad 8 | Numpad 9 |

Hold **Ctrl** with the previous/next shortcuts to browse only marked shaders. Holding a browsing key accelerates movement through the list.

Use the group's assigned hotkey to test its selection. Click **Done hunting**, or the row's **Done** action, to finish and save the selected shaders.

### Original shader matching

**Use original shader matching** is enabled by default, including when loading an existing INI.

It preserves last-known shader matching across passes, which can be necessary for older groups to find and disable the same content as before. A group containing a compute shader can affect later graphics calls through that remembered match. Direct compute dispatches continue running in this mode.

Disabling it enables stricter stage matching and direct compute suppression. This can change the behaviour of existing groups. Refine and the Effect Finder maintain their own precise matching rules; Smart disable still needs a verified current pixel shader for colour replacement.

The preference saves as **OriginalShaderMatching** in **[General]**. Leave it enabled when reproducing a working original ShaderToggler setup.

## Smart disable

Simply skipping a pass can leave old contents in a render buffer. This can cause effects such as bloom burn-in or stale lighting. Smart disable can instead run a compatible colour-replacement pass.

### Guided workflow

1. Start **Hunt** and wait for collection to finish.
2. Select the relevant **pixel shader** with Numpad 1 / 2.
3. Click **Smart disable**.
4. Inspect the result and use **Show original for comparison** when needed.
5. Click **Looks good** to mark the shader and save the method, or **Try another method** to try the next option.
6. Finish with **Done hunting**.

Available methods include:

- Transparent black.
- Opaque black.
- White.
- Mid-gray.
- Ordinary draw skipping.

Under **Advanced colour**, choose custom RGBA values and click **Preview custom colour**. **Cancel preview** returns to the previously saved method, or ordinary hunting if no method was saved. Changing the selected shader or finishing hunting discards an unconfirmed preview.

The initial suggestion uses sampled blending information. It still needs visual confirmation: the add-on cannot infer the meaning of every game's render buffers.

For native DirectX 9 **Syndicate.exe**, the known bloom shader **4E71379D** has an automatic transparent-black replacement. A manually saved method takes priority.

### Saved methods

Methods are saved by **graphics API and pixel shader hash** in ShaderToggler.ini. Groups using the same shader on the same API share its saved method.

The group still controls when that method applies through its hotkey, startup, hold, timed and suspension settings.

### API support

| API | Colour replacement support and requirements |
| --- | --- |
| **DirectX 9** | Pixel shader models 2.0 and 3.0; one supported floating-point colour output/target; no external shader compiler |
| **DirectX 10** | One supported colour output/target; uses the system d3dcompiler_47.dll |
| **DirectX 11** | Feature level 10.0 or higher, an immediate context and one supported colour output/target; uses d3dcompiler_47.dll |
| **DirectX 12** | Captured compatible graphics pipeline; one to eight supported floating-point colour outputs, including sparse slots; DXBC uses d3dcompiler_47.dll and DXIL uses dxcompiler.dll |
| **Vulkan** | Captured compatible graphics pipeline, one known colour target and a supported fragment output; supported dynamic-rendering setups are included; no compiler DLL |
| **OpenGL** | OpenGL 4.3 or newer, captured GLSL source for a linked graphics program and one supported colour output/target; compilation uses the graphics driver |

When ReShade reports **Vulkan** for a DXVK game, the Vulkan backend is used.

DX12 can use a verified pipeline/output layout when target-binding callbacks were not observed. Known missing targets, invalid bindings and ended passes still prevent replacement. Multiple outputs may hold different kinds of data, so check the whole image before accepting a colour.

Smart colour replacement targets pixel/fragment shaders. Compute, mesh, depth-output and other incompatible passes are not covered by this colour-replacement path. Other restrictions include writable shader resources, unsupported output types or layouts, incomplete pipeline information, DX11 deferred contexts, unsupported Vulkan pipeline libraries or shader objects, and OpenGL separate-program or binary-only configurations.

If a colour replacement cannot be applied, the original pass is retained and the UI/log explains why. **Try another method** also offers ordinary skipping.

Blending, depth/alpha tests, write masks and draw coverage still affect the result. There is no universal neutral colour, and Smart disable does not clear an entire render target or provide texture passthrough.

## Refine: separate effects that share shaders

Use **Refine** when a working group hides both the unwanted effect and something you want to keep—for example, weapon trails together with fire and smoke.

### Recommended paused-scene workflow

1. Turn the broad source group **OFF** so all relevant effects are visible.
2. Freeze the unwanted effect and wanted fire/smoke using the game's pause menu. Hide the menu/HUD with your usual separate group if needed.
3. Click **Refine** on the source group.
4. The add-on creates an empty **[group name] - Refined** group, selects the source automatically and starts the paused workflow.
5. Close ReShade. After two seconds to prepare and a two-second scan, browse candidates while the scene stays paused.
6. Use **Numpad 1 / 2** to browse and **Numpad 0** to compare.
7. When only the unwanted effect disappears, press **Numpad 3** to keep the filter and finish.
8. Assign the refined group a hotkey, enable it and test again after unpausing.

No idle capture or manual source selection is needed when starting with **Refine**. It uses paused mode without changing the saved default mode for standalone Finder sessions.

The new group contains confirmed precise filters. It does not inherit the source's broad shader rules, hotkey or startup settings. Keep the original broad group disabled while testing, including its **Active at Startup** option.

### Several shaders or filters

A source group can contain more than one shader. Put the relevant shaders in the source group before starting Refine; the scan follows that group's selection.

If more than one filter is needed, use **Manual controls and details → Keep and test more**, or create several working refined groups and merge them.

Keep refined groups focused on their precise filters. Adding broad whole-shader rules through Hunt can bring back the unwanted side effects those filters were intended to avoid.

### Can the original group be deleted?

Yes. Saved refined filters are independent of the original group and do not require it to remain present or enabled. Keep it only if you want it as a reference or for another refinement session.

### What refinement can distinguish

Filters match recorded draw/dispatch characteristics, such as shader combinations and call dimensions. DX12 can also include captured pipeline settings. Refinement can follow the remembered shader condition used by an original toggle.

Effects with identical captured signatures can remain linked. Texture-specific and per-object resource filters are not implemented. A game update, scene, weapon, animation or graphics-setting change can affect which calls match.

Refined filters skip matching calls; they do not apply Smart disable's colour replacement. Check the result during normal gameplay as well as in the paused test scene.

## Standalone Effect Finder

The Finder remains available in **Advanced tools** for cases where you do not already have a useful shader group.

Choose a **Finder mode**, then click **Find effect in new group**. To use a particular destination, expand **Use an existing destination**, select it and click **Find effect**.

### Paused scene

Freeze the scene with the game, then start the finder. After the two-second preparation and two-second scan, it previews one candidate at a time. You can stay paused while browsing.

Under **HUD visibility and shader selection**:

- **Look for shaders from** restricts the scan to an existing group's shaders or filters.
- **Keep these groups enabled while finding** preserves selected HUD/menu-hiding groups, including during original comparison.

The source and destination are excluded from kept groups. These choices apply to the finder session; they do not rewrite the groups' normal activation settings. Changing them clears the scan and preview, so use **Scan paused scene** again.

The game must keep drawing the scene while paused. A cached pause image contains no fresh individual draw calls for the finder to change. The add-on does not pause the game itself or force it to redraw.

### Live comparison

1. Start **Live comparison** and close ReShade.
2. Follow the three-second countdown, then remain idle for eight seconds.
3. After four seconds to prepare, repeat the unwanted effect for eight seconds.
4. Keep the scene comparable and wanted effects visible during both captures.
5. Answer the automatic tests until an individual candidate can be confirmed.

The finder first tests graphics batches of up to eight candidates and narrows successful batches. It saves only a confirmed individual filter, not a whole batch.

Candidates unique to the action capture rank first. Candidates seen in both captures can still qualify when their rate per frame increased substantially. This is a comparison of observed calls, not automatic recognition of a named effect.

### Finder controls

Enable **Num Lock**. These shortcuts apply while the finder is active and ReShade is not capturing keyboard input:

| Key | Paused / individual browsing | Live batch tests |
| --- | --- | --- |
| **Numpad 1** | Previous candidate | Undo the previous answer |
| **Numpad 2** | Next candidate | Effect still visible / try another |
| **Numpad 3** | Keep the current filter and finish | Effect gone / narrow the batch; confirm when one candidate remains |
| **Numpad 0** | Original / preview comparison | Original / preview comparison |
| **Numpad 5** | Stop without saving the current unconfirmed preview | Stop without saving the current unconfirmed preview |

Equivalent buttons are available in ReShade. Holding a key answers only once. Confirmation requires a matching call and an active preview; return from original comparison before confirming.

**Manual controls and details** provides individual browsing, separate captures, quick-test controls and **Keep and test more**. Previously kept filters remain saved if you later stop without accepting another preview.

During live finding, normal groups are temporarily bypassed on that device. Paused mode allows selected groups to remain applied. Normal group operation resumes when the session ends. Finder shortcuts do not also activate the add-on's ordinary group/hunting controls, but the game can still react to keys bound in its own controls.

### Compute candidates and limits

Graphics candidates are used by default. Compute candidates are recorded but excluded from browsing unless **Include compute candidates (advanced)** is enabled for the current session. Compute previews are always individual, and the option resets for a new session. Skipping compute can remove data required by later rendering and may destabilize a game.

Keep original shader matching enabled when following an old compute-based toggle into graphics calls; that does not require enabling compute previews.

Current limits:

- 32,768 unique signatures per capture.
- 256 ranked candidates.
- 64 saved filters per group.

The UI reports incomplete captures or shortened candidate lists. Saved filters appear under **Edit → Saved effect filters** and can be removed there. They are stored in **[GTGroupN_EffectFilters]** in ShaderToggler.ini; older version 1 rules remain supported.

Capture/filtering covers supported commands reported for DirectX 9/10/11/12, Vulkan and OpenGL. Unsupported or unknown signatures are excluded. Cached command lists may not generate fresh callbacks each frame. DX12 indirect filtering needs captured native metadata; OpenGL multidraw subcalls are excluded when isolating one would remove the entire batch.

## Merging groups

Use this to combine several working refined groups under one name and hotkey.

1. Finish Hunt or close the finder.
2. On **Groups**, click **Merge**.
3. Select the destination under **Merge into**.
4. Select the source groups to combine with it.
5. Check the resulting shader/filter counts and **Remove merged source groups**.
6. Click **Merge selected groups**.

The destination keeps its name, hotkey, current active state, startup settings, hold/timed behaviour and other controls. The merge combines shader selections and saved filters, removing exact duplicates.

Refined filters retain their conditions. Merging them does not convert them into broad shader rules. Any whole-shader rules in selected source groups are also included and identified in the preview.

A merge exceeding 64 unique filters is rejected without discarding filters. With **Remove merged source groups** enabled, only the selected sources are removed. If disabled, they remain available with their own hotkeys and activation settings.

Merges save automatically to ShaderToggler.ini.

## Reordering and sorting

Drag a group's **dotted handle or name directly in the main list**. Drop in the upper or lower half of another row to insert before or after it. A green line shows the insertion position.

Ordering saves automatically. Dragging also works while searching or filtering; other groups retain their relative order. A normal name click still opens the editor.

There is no separate Order panel. **Sort** offers:

- **Hotkey layout**: the Gametism key layout.
- **Name A-Z**.
- **Name length**.

Using Sort replaces the current manual order.

## Hotkeys and controller labels

Keyboard, mouse and supported controller bindings can be mixed across groups and timed inputs.

In **Settings**, **Controller labels** offers **Auto**, **Xbox** and **PlayStation**. Auto attempts to detect PlayStation controllers. This setting changes displayed button names; it does not add support for a controller the input system cannot read.

### Global hotkey modifier

A global modifier adds Ctrl, Alt, Shift or any combination of them to all keyboard and mouse bindings at runtime. **None** leaves them unchanged.

For example:

| Stored binding | Global modifier | Effective binding |
| --- | --- | --- |
| Caps Lock | Ctrl | Ctrl + Caps Lock |
| Alt + F1 | Ctrl | Ctrl + Alt + F1 |

Stored bindings are not rewritten. Controller bindings are unaffected by the global modifier.

## Activation modes

For a typical hide group, active means the selected content is hidden. Keep that relationship in mind when choosing a mode.

| Mode | Behaviour |
| --- | --- |
| **Toggle** | Press the main hotkey to switch the group on or off |
| **Hold** | Active while the main hotkey is held |
| **Inverted hold** | Active while the key is released; inactive while held |
| **Timed** | A trigger temporarily makes the group active, then it returns to inactive |
| **Inverted timed** | A trigger temporarily makes the group inactive, then it returns to active |

Hold and timed behaviour are configured in **Edit**. They are alternative operating modes.

### Active at Startup

Enable **Is active at startup** to start a group active when the configuration loads. It is supported on x86 and x64.

An optional startup duration ends that startup activation automatically. This is useful for temporary startup-logo or launch-time shader changes. Without a duration, normal group controls determine later changes.

## Timed triggers, suppression and linger

Expand **Timed triggers and suppression** in the group editor.

Multiple trigger keys can be assigned. If none are configured, timed mode uses the group's main hotkey.

| Trigger mode | Behaviour |
| --- | --- |
| **On press** | Starts the temporary state on a press |
| **While held** | Refreshes the timed state while held |
| **Press + hold** | Starts on press and continues refreshing while held |

### Timing options

- **Hide delay / show time**: the duration of the temporary state; for an inverted HUD group, how long the HUD remains visible.
- **Minimum visible time**: prevents the temporary state from ending too quickly after a short input.
- **Fade-out linger**: an additional delay before returning to the resting state.
- **Suppression linger**: keeps suppression active briefly after releasing a suppression key, useful when controller triggers release slightly later than face buttons.

**Fade-out linger is a timing option, not an animated opacity fade.** The current add-on does not gradually fade HUD elements.

### Suppression keys

Holding any configured suppression key takes priority over timed triggers. It cancels the current temporary state, resets its linger timing and returns the group to its resting state. Suppression linger can extend that protection briefly after release.

Suppression applies to timed mode. It does not change how a group works when using normal toggle or hold mode.

### Example: automatically reveal a hidden HUD

1. Hunt and save the HUD shaders.
2. Confirm that the group's ON state hides the HUD.
3. Enable **Is active at startup**, timed mode and **Invert auto-hide behavior**.
4. Add attack or aim as a timed trigger.
5. Set a duration such as **1500 ms**.

The HUD is hidden at rest. A trigger reveals it temporarily, then it hides again. Add both a mouse button and a controller button if both should activate it.

For continuous combat, use **While held** or **Press + hold**. A duration such as **500 ms** keeps the HUD visible briefly after the input stops.

### Example: briefly hide an overlay

Use normal timed mode without inversion. The overlay is visible at rest; a trigger activates the hide group temporarily, then the overlay returns.

### Example: protect controller combinations

If RT triggers a temporary effect change but RT + A/B/X/Y should not, add those face buttons as suppression keys. The combination suppresses the timed action while those buttons are held.

This controls when an existing group acts. To separate a trail from smoke/fire that share the same shader, use **Refine**.

## Suspend All Toggle Groups

Configure suspension under **Settings** while gameplay is visible and the game's menu is closed. Use **Set Current State as Gameplay** to establish the normal state.

Separate **Suspend Hotkeys (menu open)** and **Restore Hotkeys (menu close/back)** support games that use different inputs to enter and leave menus. A restore key is checked only while groups are suspended.

While suspended:

- Normal group shader rules and saved filters are temporarily ignored.
- Timed, startup and hold behaviour pauses.
- Normal toggle presses are queued for restoration.
- The group list displays amber **SUS** badges.

Use the configured restore input or the UI's restore control to resume operation. This is useful for menus, maps, inventory, dialogue and photo mode. It is driven by your configured inputs rather than automatic recognition of every game's menu state.

## Saving and compatibility

Use **Save all** to write the complete configuration. **Save group** also saves and closes the editor. Drag ordering, sorting, merges, interface-size changes and confirmed Smart disable methods save through their respective controls.

The configuration remains **ShaderToggler.ini**, regardless of the new add-on filename. It includes ordinary shaders, precise filters, Smart disable methods, hotkeys, notices, activation rules, ordering and preferences.

Original matching defaults to enabled for existing configurations. Saved refined filters work without their original source group. Ordinary broad rules remain broad after copying or merging, so keep them disabled when checking a refined result.

## Diagnostic logging

**ShaderTogglerAdvanced.log** is created beside the game executable and overwritten on each launch.

It records:

- Add-on version, architecture, startup and graphics API information.
- Configuration loading/saving and group changes.
- Hunting, Smart disable methods and compatibility failures.
- Finder capture summaries, preview selections and filter saves.
- Merge and reorder operations.
- Requested/applied interface scale and font size.
- Periodic status summaries.

The startup header does not include a dated build label.

Logging is bounded to **2 MiB**, with rate limiting for rapid routine messages. It does not install a crash handler or log every draw/dispatch. A logging failure does not disable the add-on.

When reporting an issue, copy the log **before starting the game again**. Include the game/API, what you were doing and whether the issue occurs during Hunt, Smart disable, Refine or normal toggling. The last preview entry can help investigate a crash but is not proof of its cause.

The **Help** tab shows the log filename and any logging error.

## Troubleshooting

| Problem | What to check |
| --- | --- |
| The add-on does not appear | Full Add-on support in ReShade, executable architecture, actual executable folder and duplicate older add-on files |
| A toggle that worked in the original version no longer matches | Enable **Settings → Hunting and controls → Use original shader matching** |
| A target never appears during Hunt | Keep it visible during collection and increase **Frames to collect** for intermittent effects |
| Smart disable is unavailable | Finish collection and select a pixel shader; colour replacement requires supported current shader/pipeline state |
| The preview asks for dxcompiler.dll | Supply the compiler matching the game's architecture beside its executable, then restart |
| A pass needs one known colour target | That backend needs one identified compatible output target; DX12 supports compatible multi-target layouts, while the other colour backends retain their one-target requirements |
| A colour replacement leaves the effect unchanged | Check the preview's compatibility reason and try another method, including ordinary skipping |
| A trail toggle also hides fire or smoke | Turn the broad toggle off and **Refine** it into a separate group |
| A paused scan finds nothing useful | Ensure the game continues rendering while paused, keep the effect visible and select a relevant source group |
| Refinement still hides wanted content | Check for active broad shader rules; identical captured signatures may not be separable |
| A refined result works paused but not during gameplay | Capture/test another animation or scene; additional filters may be needed |
| Numpad finder shortcuts do nothing | Enable Num Lock and close ReShade's keyboard capture; use the equivalent UI buttons |
| Groups show SUS | Restore globally suspended groups |
| Text is too small or too large | Use **Size**; Auto targets 32 px at 4K, while manual percentages multiply the current ReShade font |
| A save fails | Check ShaderTogglerAdvanced.log and whether the game folder permits writing |

## Credits and licensing

**Original ShaderToggler:** Frans "Otis_Inf" Bouma.  
**ShaderToggler Advanced modifications:** Sven "Gametism" Königsmann.

Third-party components include ReShade/Dear ImGui interfaces, MinHook, Microsoft's DirectX Shader Compiler, Khronos headers and SPIRV-Reflect. Their notices are included with the corresponding source files, under **src/Vendor**, and in **Runtime/DXC-LICENSE.txt**.

The new Advanced modifications are proprietary: **Copyright (c) 2026 Sven "Gametism" Königsmann. All Rights Reserved.** Without prior written permission, copying, reuse, modification, redistribution, mirroring, derivative works and AI-assisted reuse of those modifications or their source are prohibited. See **PROPRIETARY_LICENSE.txt**.

Original work and third-party components retain their own licenses.


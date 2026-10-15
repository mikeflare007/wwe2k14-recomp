# Keyboard controls

The keyboard plays as an Xbox 360 controller, so in-game prompts show controller buttons. A controller and the
keyboard can be used together. WWE 2K14 Recomp uses the game's built-in layout (right column) unless you put a `wwe2k14.toml` with your own
bindings next to `wwe2k14.exe` (same format as WWE '13 Recomp's `wwe2k14.toml`).

| Pad input | WWE 2K-style layout (needs a wwe2k14.toml) | Default (no wwe2k14.toml) |
| --- | --- | --- |
| A (grapple, menu confirm) | L | Space |
| B (Irish whip / pin, menu back) | K | Shift |
| X (strike) | J | R |
| Y (signature / finisher) | Space | E |
| Left trigger (LT, run / climb) | Shift | Z |
| Right trigger (RT, reversal) | 1 | Control |
| Left shoulder (LB, pick up / ring in-out) | U | Q |
| Right shoulder (RB, limb targeting) | Control | F |
| Left stick up / down / left / right | W / S / A / D | W / S / A / D |
| Left stick press (L3) | C | C |
| Right stick up / down / left / right | T / G / F / H | I / K / J / L |
| Right stick press (R3) | V | M |
| D-pad up / down / left / right (taunts) | Up / Down / Left / Right | same |
| Back | Tab | Tab |
| Start | Escape | Escape |

The game's own built-in keys (ReXGlue mnk_input_driver.cpp) apply when no wwe2k14.toml is present.

## Changing keys by hand

The keys are `keybind_*` settings in `wwe2k14.toml` next to the game executable (next to `wwe2k14.exe`). Use the flat names as top-level keys and ReXGlue's key names (`Control`, not `Ctrl`):

```toml
keybind_a = "L"
keybind_right_trigger = "1"
keybind_rstick_up = "T"
```

The mouse is not used for gameplay.

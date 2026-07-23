# GUI state machine

The watch GUI navigation is driven by a StateSmith state machine. Its editable
source of truth is:

```text
statecharts/GuiSm.plantuml
```

PlantUML was selected instead of draw.io for this machine because the diagram
is compact, reviewable as text, and can be generated without a graphical
editor. A future machine may use StateSmith draw.io input, but one machine
must never have two independently edited source diagrams.

## Current flow

```text
                       ACTIVATE
                   +-------------+
                   v             |
[*] -> HOME_CONTEXT        DIAGNOSTIC_FACE
                   |             ^
                   +-------------+
                       ACTIVATE
```

`HOME_CONTEXT` is the first target-specific prototype of the planned main
window. `DIAGNOSTIC_FACE` keeps the earlier watch face available as a simple
fallback. The temporary touch adapter maps a complete-screen tap to the
semantic `ACTIVATE` event; raw coordinates remain outside the machine.

```text
touch driver
    -> semantic event (ACTIVATE)
    -> generated StateSmith machine
    -> entry action adapter
    -> watch screen model/view
    -> NOG_C
    -> GC9A01 backend
```

## Ownership boundaries

- `statecharts/GuiSm.plantuml` owns navigation states, events, guards, and
  transitions.
- `statecharts/GuiSm.c` and `statecharts/GuiSm.h` are generated artifacts.
  Never edit them manually.
- `src/app/gui_state_actions.c` adapts state entry/exit actions to firmware
  screens. Hardware drivers and rendering details do not belong in the chart.
- screen modules own their data and rendering behavior.
- NOGGUI will own reusable UI semantics, layout, profiles, skins, and widgets.
- NOG_C remains an independent graphics engine and knows nothing about watch
  navigation.

## Commands

StateSmith CLI is pinned to version `0.22.2`. Install it once:

```powershell
npm run install:statesmith
```

Regenerate the state machine explicitly:

```powershell
npm run generate:statecharts
```

The normal build also regenerates it before CMake compilation:

```powershell
npm run build
```

The local CLI binary is stored below `tools/statesmith/` and is ignored by
Git. The installer verifies its SHA-256 checksum before use.

## Adding a screen

1. Add a state and its transitions to `statecharts/GuiSm.plantuml`.
2. Add a semantic event only when an input has acquired GUI meaning. Raw touch
   coordinates and bus events must stay outside the state machine.
3. Give the state an `enter` action and declare that action in
   `include/watch/gui_state_actions.h`.
4. Implement the thin action in `src/app/gui_state_actions.c`; delegate actual
   rendering to a screen module.
5. Run `npm run build`. Compilation must fail if generated actions and their
   implementations disagree.
6. Test the transition and the screen independently where practical, then
   verify the generated machine on the target through the diagnostic state.

Keep state names stable and descriptive. Prefer semantic events such as
`ACTIVATE`, `BACK`, `NEXT`, `PREVIOUS`, `TIMEOUT`, `PHONE_CONNECTED`, and
`ALERT_RECEIVED` over driver-specific names. A raw tap or swipe is translated
to one of these events by the input adapter according to the active screen.

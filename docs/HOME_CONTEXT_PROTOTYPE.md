# Home-context prototype

Date: 2026-07-23

`HOME_CONTEXT` is the first watch-target prototype of the main window described
by the Universal UI specification. It validates the round-screen composition,
incremental rendering, and StateSmith navigation before the same screen is
expressed as a reusable NOGGUI package.

It deliberately belongs to the watch firmware for now. It must not become a
device-specific API in NOG_C, and it must not be treated as the final NOGGUI
renderer or package format.

## Visible model

- `HH:MM`: local watch time;
- seconds bar: smooth subsecond progress;
- `PH`: phone connection slot, currently offline;
- `TS`: Technosense connection slot, currently offline;
- `DET`: detector connection slot, currently offline;
- `LOCAL`: the only currently available context, reported with a green marker.

Dim status markers mean that no corresponding connection has been established.
The prototype does not display fabricated notifications, battery values, radio
state, or remote context.

## Round layout

The active content stays inside the practical safe area of the 240 x 240 round
display:

- connection header begins at `y = 28`;
- header divider is at `y = 49`;
- the large time begins at `y = 67`;
- seconds progress is at `y = 151`;
- the context panel occupies `x = 44..195`, `y = 174..209`.

The tiny built-in text skin uses a fixed 3 x 5 bitmap alphabet scaled by an
integer factor. It is a bounded, allocation-free target aid, not a replacement
for NOGGUI typography.

## Rendering contract

Screen entry may clear and compose the complete window. Steady-state clock
updates do not clear complete widget rectangles:

- seven-segment digits repaint only changed segments;
- the seconds bar paints only its changed pixels;
- static labels and panels are not redrawn on every RTC tick.

This preserves the no-flicker behavior required by
`docs/RENDERING_POLICY.md`.

## Temporary navigation

The entire touch surface currently produces the semantic `ACTIVATE` event.
StateSmith uses it to toggle between `HOME_CONTEXT` and `DIAGNOSTIC_FACE`.
This is only a bring-up interaction. Future screen-specific hit targets and
gestures must be translated to semantic events in the input adapter.

## Promotion path

Before this screen becomes reusable UI:

1. define the 240 x 240 round Display Profile in the independent NOGGUI repo;
2. express connection and context data as a semantic View Model;
3. reproduce this composition in the reference renderer;
4. define bounded embedded layout and text resources;
5. replace the watch-owned composition with a thin NOGGUI adapter;
6. keep the diagnostic screen in firmware as a recovery path.

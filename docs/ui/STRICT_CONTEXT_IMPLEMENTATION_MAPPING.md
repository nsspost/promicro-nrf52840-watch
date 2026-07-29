# Strict Context graphics mapping

Target: GC9A01 round 240×240 RGB565, rendered as a monochrome visual profile.
The graphics package uses a 256×256 authoring canvas; the target receives a
separately rounded compact-240 layout, never a cropped square layout.

| Graphics responsibility | Existing source | Target responsibility |
|---|---|---|
| Semantic snapshot | `ui_demo_model.c` | Remains independent of the skin |
| Navigation and safety lifecycle | `watch_ui.c` | Remains shared by both skins |
| Render snapshot | `watch_ui_t` | Adds only selected skin/profile state |
| Renderer-neutral geometry | none | Bounded strict-context layout/IR tables |
| Host renderer | none | 256×256 HTML/SVG executor for the same layout roles |
| Font authoring source | compact 3×5 fallback | Roboto WOFF2 from the graphics package |
| Icon authoring source | none | Phosphor SVG from the graphics package |
| Asset compiler | none | Offline conversion to fixed mono bitmap resources |
| Embedded raster backend | `nog_gc9a01_backend.c` | Executes primitives through NOG_C |
| Physical display | `gc9a01.c` | Unchanged |
| Input and hit map | fixed targets in `watch_ui.c` | Shared semantic actions, ≥44×44 |

The current color skin remains `builtin.color-context`. The new variant is
`builtin.strict-context`. Neither skin changes event severity, command risk,
confirmed/pending meaning, or overlay precedence.

## Implemented prototype

- All 11 semantic screens can render through either built-in skin.
- The strict skin uses black, white and gray only, divider-based composition,
  explicit glyphs and text labels, and no decorative cards or shadows.
- Fourteen Phosphor SVG sources are compiled offline into fixed 1-bit masks;
  firmware never parses SVG.
- Roboto 500/600/700 WOFF2 sources are compiled offline into eight fixed
  1-bit profiles (8, 10, 12, 16, 20, 23, 39 and 64 px). The generated subset
  includes Latin, Cyrillic, digits, units, degree and true minus glyphs.
- The 240×240 target has its own round-safe geometry and ≥44×44 hit targets.
- Skin switching is exposed in the diagnostic screen and by debug mailbox
  command `5`. Reset intentionally returns to the color skin for now.
- `scripts/verify-ui.ps1` walks both skins, guarded commands, critical requests,
  event preemption and acknowledgement.

## Validation

`scripts/render-strict-host.ps1` renders the 256×256 Roboto/Phosphor authoring
view and emits the actual, difference, and side-by-side golden images under
`build/strict-context-host/`. The current host result is 54.53% exact pixels:
geometry and semantic grouping match, while the supplied reference renders
noticeably narrower text than the supplied Roboto assets.

The embedded strict skin now uses the same supplied Roboto family through a
27,403-byte compiled bitmap subset. It allocates no glyph heap or framebuffer.
The color skin intentionally retains its original compact 3×5 font. Top and
bottom elements use explicit 240×240 circular safe-area coordinates rather
than a scaled or cropped 256×256 layout.

# Test-data GUI conformance

This target implementation is a watch-first prototype of the Universal UI
semantics. It intentionally embeds a bounded test View Model instead of
loading a binary UI package or connecting live transports.

| Requirement | Target implementation |
|---|---|
| normalized semantic values | fixed `watch_ui_demo_model_t` |
| quality is visible | good, uncertain, stale, offline |
| bounded layout | fixed 240×240 round templates |
| no steady-state allocation | static model, navigation and hit tables |
| guarded command | 1500 ms hold; success/rejected/timeout |
| critical request-only | approved/denied; no execute action |
| desired != confirmed | confirmed AUTO remains visible during request |
| blocking event precedence | event detail preempts request/command surface |
| bounded navigation | depth 4 |
| minimum hit target | 44×44 px |
| Russian labels | bounded uppercase UTF-8 glyph set |
| target interaction test | 43-step J-Link mailbox scenario |

Not implemented in this prototype: package parser/compiler, CRC and malformed
package fallback, locale resources, real transport, TTL/idempotency wire
contract, external authority, persistence and power-loss recovery. Those are
integration stages, not simulated as if they already existed.

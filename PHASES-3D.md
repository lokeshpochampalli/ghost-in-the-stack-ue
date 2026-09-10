# PHASES-3D.md

Operational file for the Unreal build. Same run protocol as before: say "go", it finds the lowest
phase not marked done, verifies the previous phase still passes, executes, updates the status
table, reports, stops.

New repo: `ghost-in-the-stack-ue`. The TypeScript repo stays as the reference implementation and
fixture source. Copy `docs/02-LANGUAGE-SPEC.md`, `docs/DECISIONS.md`, `tests/fixtures/`, and
`docs/3D-REDESIGN.md` into it before Phase 0.

Rules that hold throughout:

- **The fixtures are the spec.** The C++ interpreter is correct when it reproduces all 30 golden
  traces byte for byte. No interpreter behaviour is decided by reading the TypeScript; it is
  decided by the fixture, and disagreements go back to the language spec.
- **Every system binds effects to animation, never to logic.** A `Door` actor does not decide
  whether to open. It receives an effect from the trace and animates. All decisions live in the
  interpreter.
- **Blueprints for wiring and sequencing. C++ for anything with a loop in it.**
- **Screenshot every phase.** The editor MCP can capture the viewport. Every acceptance report
  includes screenshots. This is the lesson of the isometric view that was blank for a month.
- **Blender assets go through one pipeline.** One scale, one export preset, one naming scheme,
  collision on everything. Fix the pipeline, not individual assets.
- `docs/DECISIONS.md` continues from ADR-026. Append-only, human-written. Say when one is needed.

---

## Status

| Phase | State |
|---|---|
| 0 — Project and toolchain | done 10 Sep 2026: builds clean, MCP connected, kit pipeline (`Content/Kit/Kit.blend`, `Tools/export_kit.py`, `Tools/import_kit.py`) exports the test cube with collision into the level, screenshots in the report. Outstanding: GitHub remote |
| 1 — Interpreter port | done 10 Sep 2026: all 30 golden traces byte for byte, 45 automation tests green, run headless and through the MCP. See `docs/INTERPRETER-PORT.md` |
| 2 — First system: a door | done 10 Sep 2026: `L_Sector1_Airlock` with one corridor, one door, one terminal and one wall display; the interpreter's trace drives the door through the station subsystem's play head; single-line editing and the ADR-020 refusal on the terminal; 60 fps (vsync-capped, min 59.5) while the door animates. Screenshots in `docs/screenshots/phase-2`. See `docs/STATION-LAYER.md` for the divergences (C++ Slate screens, trace player built early, door/light builtins) |
| 3 — Recorder and rewind | not started |
| 4 — VANT and prediction | not started |
| 5 — Power | not started |
| 6 — Blender kit and Sector 1 blockout | not started |
| 7 — Sector 1 complete: the vertical slice | not started |
| 8 — Telemetry and instruments | not started |
| 9 — Sectors 2 to 4 | not started |
| 10 — Automation and Make | not started |
| 11 — Polish, audio, packaging | not started |

---

## Phase 0 — Project and toolchain

UE 5.8 C++ project, first-person template as the starting point. Enable Epic's official
`ModelContextProtocol` plugin (ships with 5.8) plus `AllToolsets`, and Epic's Claude Code skills
plugin; confirm `list actors` works. Confirm Blender MCP is
connected. Set up the Blender export pipeline: one `.blend` kit file, one export script, glTF with
collision, 1 unit = 1 cm, naming `SM_Kit_<Name>`. Export one test cube through it and place it in
the level.

A GitHub repo with `Content/` under Git LFS. A `README.md` that states which UE version, which MCP
plugin, and how to start the server.

**Acceptance:** project opens, compiles clean, MCP connected, one Blender-authored mesh in the
level with collision, screenshot in the report.

---

## Phase 1 — Interpreter port

C++ code under `Source/GhostInTheStack/Interpreter/`. Lexer, parser with recognition pass, an
evaluator emitting one `FGitsStep` per AST node into a recorded `FGitsTrace` (eager rather than a
generator; the trace is identical and the caps bound the work), a `WorldOracle` with every read
recorded, observable-state `Diff`, error catalogue with all 46 codes.

Port the fixture runner: read `Fixtures/<name>.py`, produce the compact one-line-per-step
format, compare byte for byte against `Fixtures/<name>.trace.txt`. The fixtures are flat files,
not one folder per fixture; normalise line endings when reading, since Git checks them out as
CRLF on Windows. This runs as an Unreal automation test so
the MCP can trigger it.

**Acceptance:** all 30 golden traces pass byte for byte. Both caps enforced in their own units.
Recursion to depth 100 gives the friendly message. The `-7 % 3` and floor-division fixtures pass.
No fixture was edited to make it pass — if one is wrong, the TypeScript reference is wrong too and
that is a language-spec decision, not a port decision.

This is the phase to be slow on. Everything else consumes its output.

---

## Phase 2 — First system: a door

One corridor, one door, one terminal. The door has a script. The terminal shows it in a UMG
monospace panel with keyword tinting. Run executes it through the interpreter, the trace's effects
drive the door's animation. A wrong script leaves the door shut and the wall display showing what
the code actually produced.

The terminal supports editing a single line. Running unchanged source is refused with VANT's
message on the display.

**Acceptance:** walk to the terminal, read the script, run it, watch the door open. Break the
script, run it, watch it not open, see why on the display. Screenshots of both. Frame rate stays
above 60 while the door animates.

---

## Phase 3 — Recorder and rewind

The `Recorder` module maps trace step index to world time. Hold a key to scrub backward: the door
reverses along its animation, the terminal highlights the line executing at each step, the wall
display shows the bindings at that step. Release to resume. Scrubbing is stepping the index and
letting every system pose itself; nothing is simulated in reverse.

Loop collapsing from the ribbon becomes a visual: while rewinding through a loop, VANT's display
shows "iteration 3 of 12" rather than every line flicker.

**Acceptance:** rewind a 20-step door script and a 200-step loop. Terminal highlight matches world
state at every step. Screenshot mid-rewind. Under 16 ms per step change.

---

## Phase 4 — VANT and prediction

VANT as a companion actor: a voice, text on the terminal, a presence on the wall displays. Before a
first run, VANT asks the prediction. Three or four options on the terminal, keyboard or gamepad
selectable, each with its misconception tag from the level data. Committing is free. Correctness
revealed at the run, by what the door does and by what VANT says about it.

Wrong answer locks the prediction until the player has rewound through the anchor. VANT says so.

Hints in VANT's voice, tiered.

**Acceptance:** a full predict-run-rewind-fix loop on the door, with VANT speaking at each stage.
The scrub gate demonstrably blocks re-answering and releases after rewind. Options shuffled via a
seeded RNG, seed recorded.

---

## Phase 5 — Power

A wall gauge and the lighting rig. Running draws power; a committed prediction discounts it.
Below a threshold, corridor lights dim in stages. At zero, emergency lighting only and a torch, and
a generator to restore.

**Acceptance:** run the door script four times with unchanged predictions and watch the corridor
go dark. Restore the generator and watch it come back. Validator enforces `budget >= runCost`
and `predictedRunCost < runCost` on every level asset.

---

## Phase 6 — Blender kit and Sector 1 blockout

The modular kit in Blender via MCP: corridor straight and corner, door frame and door, terminal,
wall gauge, wall display, cold-store unit, ceiling light, generator, drone, crate. Consistent
scale and style — low-poly, clean edges, one trim sheet. Export through the Phase 0 pipeline.

Sector 1 blocked out with the kit: entry airlock, main corridor, two side rooms, the cold store,
the sealed airlock to Sector 2. Lit for the mood in `3D-REDESIGN.md`: dark, warm monitors, cold
fill. Lumen.

**Acceptance:** walk Sector 1 end to end at 60 fps. Every mesh has collision. Screenshots from
four positions. It should look like somewhere, not like a test level.

---

## Phase 7 — Sector 1 complete: the vertical slice

Eight systems in Sector 1, re-authored from the Act 1 level content: corridor lights, two doors,
the cold store, a wall clock, a vent, a pressure display, and the Make task — a four-line script
that lights the corridor to the airlock. Each keeps its concept tags and misconception tags. Each
misconception has its physical failure choreographed per the table in `3D-REDESIGN.md`.

Ilse's logs placed in the world. VANT's dialogue for the sector written. The airlock to Sector 2
opens on completion.

**Acceptance:** a playtester who has never programmed completes Sector 1 in under 45 minutes,
laughs at least once, and the airlock opens. This is the October demo. Record it.

---

## Phase 8 — Telemetry and instruments

C++ port of the event log with the same 18 event types and the same JSON export format, so the
existing `analyse.ts` reads it unchanged. Consent gate before any event, with a test. Pseudonymous
participant code. The four instruments delivered on a terminal in the airlock at session start and
end.

**Acceptance:** a full Sector 1 playthrough exports a bundle `analyse.ts` reads without error.
No event before consent, proven by test.

---

## Phase 9 — Sectors 2 to 4

Greenhouse, Logistics, Reactor. Twenty-two systems from the remaining level content. New kit
pieces: sprinkler, grow-light, heater, conveyor, more drone behaviours, reactor coolant. Each
sector's Make task. Ilse's story resolved in the Reactor.

**Acceptance:** all four sectors completable. Every concept tag covered by two systems, every
misconception tag choreographed in at least two.

---

## Phase 10 — Automation and Make

Player-owned drones that run player-written scripts continuously: harvest, restock, patrol. A
drone terminal, a small library of station builtins for drones, a way to see your drones working
from anywhere. This is the endgame loop and the commercial hook.

**Acceptance:** write a drone script that keeps the greenhouse harvested, walk away, come back to
a full cold store.

---

## Phase 11 — Polish, audio, packaging

Audio: ambience per sector, VANT's voice, mechanical sounds for every system, the bonk. Settings.
Accessibility: subtitles, remappable controls, colour-blind safe gauges, reduced-motion rewind.
Packaged Windows build. Steam page.

---

## Milestones

| When | State |
|---|---|
| October demo | Phases 0–7. Sector 1, fifteen minutes of play, someone laughs at the drone. |
| Post-viva | Phases 8–9. Full game, study-ready. |
| Commercial | Phases 10–11. |

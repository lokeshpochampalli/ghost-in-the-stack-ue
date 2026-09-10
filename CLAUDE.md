# CLAUDE.md

Persistent context for Claude Code. Read this before doing anything in this repo.

## What this is

**Ghost in the Stack** — a first-person 3D game in Unreal Engine 5.7 that teaches introductory
programming by having the player read, predict and repair the code that runs a derelict station.
Every line of code moves something in the world, and every bug does something you can laugh at.

Second design cycle. The first was a working browser prototype that a supervisor evaluated as
pedagogically sound and not fun. That repo is now the **reference implementation**: the language
spec, the decision record, and the thirty golden trace fixtures come from it and are authoritative.

Read `docs/3D-REDESIGN.md` for the game and `PHASES-3D.md` for the build order.

## The one idea everything serves

Novices who cannot trace code cannot write it. So the player cannot write code at first — they read
the previous maintainer's scripts, commit to a prediction of what each will do, pay power to run it,
**watch it happen in the world**, rewind it, and fix it a line at a time. PRIMM as mechanics.

If a feature does not make code visible, a bug entertaining, or the station wake up, it does not
go in.

## Non-negotiable constraints

- **The fixtures are the spec.** `Fixtures/` holds thirty `source.py` + expected-trace pairs. The
  C++ interpreter is correct when it reproduces every one byte for byte. Never edit a fixture to
  make it pass. If a fixture seems wrong, stop and say so — that is a language-spec decision.
- **The interpreter is reproducible.** No randomness in it. World reads go through the
  `WorldOracle` interface and every response is recorded in the trace, so replay never re-consults
  the world. Given the same source, initial world, oracle and seed, a run is identical.
- **Systems bind effects to animation, never to logic.** A `Door` receives an effect from the trace
  and animates. It never decides whether to open. All decisions live in the interpreter.
- **Rewind is index stepping.** Nothing is simulated backward. Every system poses itself for a given
  `(trace, stepIndex)`. If a system cannot pose itself from a step index alone, it is designed wrong.
- **Blueprints for wiring and sequencing. C++ for anything with a loop in it.** The interpreter,
  the trace, the recorder, the telemetry, the power model are C++. Door timelines, VANT's dialogue
  sequencing, UI bindings are Blueprints.
- **No PII, ever.** Telemetry identifies participants by a pseudonymous code. No names, emails, IPs,
  or third-party analytics. No event is written before consent.
- **Telemetry export format is unchanged from the reference repo** so its `analyse.ts` still reads
  it.

## Stack

- Unreal Engine 5.7, C++ project, first-person template as the base
- Unreal MCP (5.7 backport of Epic's plugin) + Epic's Claude Code plugin — see `SETUP.md`
- Blender 4.x via Blender MCP for the modular kit
- Git with LFS for `Content/`

Add no third-party Unreal plugins without asking. Every one is a compile risk and a thing to
justify.

## Repo layout

```
Source/GhostInTheStack/
  Interpreter/     C++ port — lexer, parser, recognition pass, evaluator, trace, oracle, errors
  World/           station power model, effect reducer, level/system data assets
  Systems/         one actor class per system type: Door, Light, Drone, Conveyor, Sprinkler…
  Recorder/        trace ↔ world-time mapping, rewind
  Companion/       VANT — prompts, hints, dialogue hooks
  Telemetry/       event log, consent gate, JSON export
Content/
  Kit/             Blender-authored modular meshes (SM_Kit_*)
  Sectors/         one map per sector
  Scripts/         Ilse's code as DataAssets, one per system
  UI/              terminal, gauge, VANT display widgets
Fixtures/          the thirty golden traces, read-only
docs/              LANGUAGE-SPEC, DECISIONS, 3D-REDESIGN
```

## How to work

- **One phase at a time.** `PHASES-3D.md` has the run protocol. When the human says "go", follow it.
- **Screenshot every acceptance report.** Use the editor MCP's viewport capture. A phase is not
  done until there is a picture of it working. This is the lesson of the previous repo's
  isometric view, which was blank on every machine for a month because nobody looked.
- **Compile and run the automation tests before reporting.** Interpreter fixtures run as Unreal
  automation tests; trigger them through the MCP.
- **Commit before every MCP-driven editing session.** MCP tools mutate live editor state and can
  move or delete assets in one call. A clean working copy is the undo.
- **Update `docs/` in the same commit when implementation diverges from spec.**
- **`docs/DECISIONS.md` is append-only and human-written.** Continue from ADR-026. Say when a
  decision is needed and stop; do not write entries.
- **Commits:** `phase-N: short imperative summary`.

## Definition of done for any phase

1. Acceptance criteria in `PHASES-3D.md` met
2. Project compiles clean; automation tests pass
3. Screenshots attached to the report
4. `docs/` updated where anything diverged
5. Status table in `PHASES-3D.md` updated

## Visual direction

Cold, over-engineered industrial installation, circa 1978. Enamelled signage, heavy doors, warm
monitors in dark rooms. Stylised and clean, not photoreal — low-poly with strong silhouettes, one
trim sheet, and the art budget spent on lighting.

- Palette: slate `#3A4750`, oxidised copper `#5F8A7D`, bone `#E8E4DA`, ink `#1C2126`; warning
  amber `#D99A2B` reserved for power and error states only
- Terminals: monospace, warm phosphor, keyword tinting via RichText. A game terminal, not an IDE.
- Not cyberpunk. Not neon. Not a dark screen with green text.

## Things that will waste time

- Building Sectors 2–4 before Sector 1 is playable end to end
- Photoreal assets. Lighting does the work.
- Any system that decides anything. Systems animate; the interpreter decides.
- Reverse simulation for rewind. Pose from the step index.
- A level editor. Systems are DataAssets, hand-authored.
- Voice acting before the dialogue is final. Text on the terminal is enough until Phase 11.

## Timeline context

- **October demo:** Phases 0–7. Sector 1, first person, fifteen minutes of play.
- **After the viva:** Phases 8–9. Full game, study-ready.
- **Commercial:** Phases 10–11.

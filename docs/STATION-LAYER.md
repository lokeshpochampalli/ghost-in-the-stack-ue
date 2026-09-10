# The station layer

How a script becomes something that moves in the world. Written with Phase 2 (the first door);
the same layer carries every later system. Read `INTERPRETER-PORT.md` first for the trace.

## Layout

```
Source/GhostInTheStack/
  Station/
    GitsScript.h              UDataAsset: Ilse's source, title, tier, statement cap, editable lines
    GitsStationSubsystem.*    world subsystem: runs a script, holds the trace, plays it, poses systems
    GitsSystemActor.*         AGitsSystemActor base; AGitsDoor (sliding panel); AGitsLight (point light)
    GitsStation.*             level data: sensors, the initial world, the oracle
    GitsTerminal.*            diegetic terminal: screen widget, line editing, run; IGitsInteractable
    GitsWallDisplay.*         VANT's wall display: what the last run actually produced
    GitsInteractable.h        UINTERFACE for anything the player can use
  UI/
    GitsScreen.*              SGitsScreen (Slate) + UGitsScreenWidget: the shared "phosphor screen"
    GitsTerminalEditor.*      SGitsTerminalEditor: the full-screen overlay while a terminal is in use
```

`CLAUDE.md`'s planned `World/` and `Systems/` folders collapsed into `Station/`: the effect
reducer lives in the interpreter (`GitsWorld::Reduce`, because the fixtures test it), and with two
system types a folder per concept was noise. Split it when the Systems list grows in Phase 6.

## The pipeline, one run

1. The player uses a terminal (E, gamepad face-left; `AGhostInTheStackCharacter::DoInteract` line
   traces for an `IGitsInteractable`). The controller opens `SGitsTerminalEditor` over the viewport.
2. Enter calls `AGitsTerminal::RunCurrent`, which hands the terminal's current lines to
   `UGitsStationSubsystem::Run(Script, Source)`.
3. The subsystem refuses unchanged source per ADR-020 (per script asset, keyed by source text),
   otherwise parses, builds the initial world from `AGitsStation` (`InitialSwitches`,
   `InitialLevels`), and runs the evaluator with the station's sensors as the oracle. The trace is kept whole.
4. Playback: the play head advances one **statement boundary** per beat (`StatementsPerSecond`,
   2.5 by default), folding effects into `CurrentWorld` and calling `PoseFromWorld` on every
   system. Systems animate towards the pose; they never look at the trace's logic.
5. Terminal and wall display listen to `OnStepChanged` / `OnRunFinished` / `OnMessage`. The
   terminal highlights the current line and shows VANT's message; the display lists the log lines,
   effects and sensor reads the run produced, then the world state it left behind.

The play head is a step index. `SeekTo(StepIndex)` poses every system from the world folded up
to that step, which is what Phase 3's rewind will call; nothing here is simulated backward.

## Systems

`AGitsSystemActor` has a `SystemId`; its world key is `door.<id>` or `light.<id>`.
`PoseFromWorld(World, bInstant)` reads only that key (`ReadBool` / `ReadNumber`).

- `AGitsDoor`: `Frame` and `Panel` static meshes. Open is the panel raised by `OpenHeight` cm at
  `Speed` cm/s. Its key holds a bool.
- `AGitsLight`: a point light in candelas. Its key holds a level 0..10; intensity is
  `FullIntensity * level / 10`.

Adding a system: subclass, pick a key, implement `PoseFromWorld`. Add the builtin that writes that
key to the evaluator (`open_door`, `close_door`, `set_light` are in `GitsEvaluator.cpp`) and a
row to `GitsDiagnostics` if it needs a new error. The interpreter test
`Evaluator.StationBuiltins` shows the contract.

## Screens

Both the terminal and the wall display are `UWidgetComponent`s (world space, 660x450 and
1140x640 draw sizes) hosting `UGitsScreenWidget`, which wraps `SGitsScreen`: a Slate widget that
draws an `FGitsScreenModel` (title, lines with keyword tints, a highlighted line, a status line).
Keyword tinting asks the lexer's keyword table, so the terminal and the language never disagree.
The screen fits its rows to the space it has each tick: code scrolls as a window around the
selected or executing line, messages keep the newest, and the status line is never overdrawn.

The screens are unlit emissive surfaces with no intensity control, so the level's exposure is
pinned at EV100 0 (`PostProcessVolume`, min = max brightness) and the corridor lamps are weak on
purpose (24 cd full, 2.5/10 lit). Brighter lamps blow the walls out before the screens dim.

## Divergences from the design docs

- **UI is C++ Slate, not Blueprint UMG.** `3D-REDESIGN.md` §4 and `CLAUDE.md` say the terminal
  is a UMG panel with RichText tinting. The screens are Slate widgets driven from C++ instead:
  the model is rebuilt from the trace on every step, and doing that through a Blueprint would be
  a loop in a Blueprint. Widget Blueprints can still wrap `UGitsScreenWidget` for layout. This
  needs an ADR (ADR-027 candidate).
- **The trace player was built in Phase 2, not Phase 3.** Statement-per-beat playback needed a
  play head to make the door open *after* the line runs, so `SeekTo` exists now. Phase 3 adds the
  recorder's time mapping and the scrub input on top of it.
- **Two new builtins.** `open_door(id)` / `close_door(id)` set `door.<id>`; `set_light(id, level)`
  sets `light.<id>`. The reference language spec lists valves, heaters and pumps; doors and lights
  are what Sector 1 has. Spec update needed in `LANGUAGE-SPEC.md` if they stay.
- **Collision comes through a sidecar**, not UCX meshes: Unreal's Interchange glTF importer drops
  `UCX_` meshes, so `export_kit.py` writes `<piece>.collision.json` and `import_kit.py` applies the
  boxes. See `Tools/README.md`.

## Console commands

For automation and for testing without walking: `GitsUse` (open the nearest terminal's overlay),
`GitsSetLine <n> <text>`, `GitsRun`, `GitsReset`, `GitsStatus` (run summary, world state and
playback frame times to the log), `GitsClose`.

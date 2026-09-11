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
    GitsPower.h               the bus as rules: stages (nominal, low, critical, out) and the light factor
    GitsGenerator.*           AGitsGenerator: Ilse's reserve cell, used like a terminal
    GitsPowerGauge.*          AGitsPowerGauge: the wall gauge screen
    GitsVent.*                AGitsVent: a grille whose fan spins on vent.<id>
    GitsClock.*               AGitsClock: the shift clock panel, ticking on clock.<id>
  UI/
    GitsScreen.*              SGitsScreen (Slate) + UGitsScreenWidget: the shared "phosphor screen"
    GitsTerminalEditor.*      SGitsTerminalEditor: the full-screen overlay while a terminal is in use
  Recorder/
    GitsRecorder.*            FGitsRecorder: step index <-> world time, loop contexts for the display
  Companion/
    GitsTags.*                the curriculum map: concept and misconception tags, script validation
    GitsShift.*               the prediction gate as plain rules (select, commit, resolve, lock, release)
    GitsVant.*                UGitsVantSubsystem: asks, settles at the anchor, locks, hints, speaks
    Tests/GitsShiftTests.cpp  the gate, anchors and tag validation
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
   `InitialLevels`) with the world the previous run left behind laid over it, and runs the
   evaluator with the station's sensors as the oracle. The trace is kept whole and the
   recorder is built from it. A run that started closes the overlay: it is watched in the world.
4. Playback: the clock (`PlayClock`, seconds) runs forward; the recorder maps it to a step, one
   **statement boundary** per beat (`StatementsPerSecond`, 2.5 by default). Reaching a new step
   folds effects into `CurrentWorld` and calls `PoseFromWorld` on every system. Systems animate
   towards the pose; they never look at the trace's logic.
5. Terminal and wall display listen to `OnStepChanged` / `OnRunFinished` / `OnMessage` /
   `OnPlayStateChanged`. The terminal highlights the current line and shows VANT's message; the
   display lists the log lines, effects and sensor reads the run produced, then the world state
   it left behind.

The play head is a step index. `SeekTo(StepIndex)` poses every system from the world folded up
to that step; nothing here is simulated backward.

## Rewind (the recorder)

`FGitsRecorder::Build(Trace, StatementsPerSecond)` computes, from the trace alone:

- the statement boundaries, one beat each, so `StepAtTime(t)` and `TimeOfStep(i)` map the
  clock to a step and back (before the first beat the head is `-1`; at `TotalTime()` the run
  is over and the head is the last step);
- for every beat, the stack of loops it is inside (`LoopsAt(step)`): loop node, header text,
  iteration and total. A loop owns every boundary whose span sits inside its own, which is
  the reference ribbon's rule (ADR-008, `scrubberModel.ts`), so nesting needs no extra state.

Holding rewind (`R`, gamepad left shoulder; `UGitsStationSubsystem::BeginRewind`) puts the
subsystem in `Rewinding`: the same clock runs backward, ramping from 1.5x to 5x playback
pace over two seconds so a long loop is not a long wait, and every time the clock crosses a
beat the head seeks to that boundary. Systems get `PoseFromWorld` with the refolded world and
animate towards it at their own speed, so the door reverses along its travel and the lights
dim back down. Releasing (`EndRewind`) resumes forward from wherever the clock got to; rewinding
to the start and releasing replays the run.

While rewinding the wall display becomes the recorder view: the line and label at the head,
one `VANT: for notch: iteration 3 of 9` line per enclosing loop instead of every iteration
flickering past, the bindings at that step, the world at that step, and `statement n of m`.
The terminal keeps highlighting the executing line with `rewind  line n`.

`GitsVerifyRewind` (console) seeks backwards through every boundary, checks the folded world
and the current line against `GitsTrace::WorldAt` and the step's span, and reports the slowest
step change in ms; that is the acceptance evidence for Phase 3.

## VANT (the companion)

`UGitsVantSubsystem` is one per world and listens to the station. The script asset carries the
curriculum (`Concepts`, `Predictions` with misconception-tagged distractors, tiered `Hints`,
`Intro`, `Outro`, a `GoalKey`/`GoalValue` world assertion); `GitsTags::Validate` checks it
against the tag lists and logs any problem when the script is first touched.

The loop, for one prediction:

1. **Ask.** Using a terminal with a pending prediction opens the overlay in question mode:
   VANT's prompt and the options in shuffled order. The order comes from
   `mulberry32(fnv1a(sessionId + scriptName + predictionId))` (`Interpreter/GitsRng`,
   byte-for-byte with the reference `rng.ts`; `Interpreter.Rng.Parity` proves it) and the seed
   is logged with `prediction_shown`. The script name is in the seed, which ADR-011's formula
   lacks: prediction ids repeat across scripts (`p1`), and two scripts sharing a permutation
   would seat the correct answer in the same place twice. Up and down choose, Enter commits. Committing is free and reveals
   nothing (ADR-006). Running with a pending prediction is refused with VANT's reason.
2. **Reveal at the run.** The anchor `{line, occurrence}` resolves against the trace's statement
   boundaries when the run starts (ADR-005; unresolvable means skipped and logged, never a
   blocker). When the play head reaches the anchored step during forward playback the
   commitment is settled: right satisfies it, wrong locks it and VANT says so.
3. **The gate.** A locked prediction releases when the head has been at or before the first
   statement and then passes the anchor playing forward (ADR-030): hold rewind to the start,
   release, watch. `scrub_gate_satisfied` is logged.
4. **Re-answer.** The prediction is pending again. A commitment made while a trace for this
   exact source exists is settled against that trace at once, with no run (ADR-020: there is
   nothing to re-run).
5. **Fix, run, outro.** An edit on the anchored line drops the commitment; other edits do not.
   When a run finishes with the goal met, VANT speaks the outro (`level_complete`).

Hints are Ilse's notes, revealed in tier order with F1 (gamepad Y) in the overlay or the
`GitsHint` console command; `hint_requested` is logged with the tier and its Phase 5 power cost.

VANT speaks through `OnSpeak`: the overlay shows the line under the screen, the wall display
carries it, and `SGitsVantCaption` shows it at the bottom of the viewport wherever the player
looks, fading after it has been readable. Text only until the dialogue is final.

Telemetry names follow the reference (`session_start` with the seed, `prediction_shown`,
`prediction_committed`, `prediction_submitted` with attempt and misconception,
`prediction_unresolvable`, `scrub_gate_satisfied`, `hint_requested`, `level_complete`) and go
to `LogGitsTelemetry` for now; Phase 8 writes the export.

## Goals, unlocks and the sector (Phase 7)

A script's goal is any of: a world assertion (`GoalKey`/`GoalValue`), the exact output
(`GoalOutput`), or a Make script's `TestCases` (ADR-012: labels shown before writing, values
hidden, failures spoken in the station's voice). A reading script also requires every reading
to be confirmed: the panel can show the right number while the player still holds the wrong
belief about how it got there, so the system holds "until the reading is yours". A confirmed
re-answer against the existing trace completes it with no run.

Completion speaks the outro, logs `level_complete`, and applies `GoalUnlocks` (`door.entry=true`,
`light.coldstore=8`) through `UGitsStationSubsystem::SetWorldValue`: a level interlock, not a
script effect; it lands in the world now and in the next run's initial world. Once every
counted terminal's script is complete, `AGitsStation::SectorUnlocks` is applied and
`SectorCompleteLine` spoken (`sector_complete`): the airlock to Sector 2 opens. Ilse's
`LogEntry` appears on the terminal's idle screen once its system works: her logs live in the
world. A wall display bound to a `Script` shows only that system's runs, and a Make script's
test checklist.

Sector 1's eight systems come from the reference's Act 1 levels, re-authored in
`Tools/setup_sector1_content.py`: the door log (entry interlock), the gauge mirror (pressure
display), the cold store setpoint (frost light), the reclaimer panel (vent), the shift clock (the
mixer's two divisions, re-themed), the door plate (cold store door), the manifest, and the Make
task (the approach rig: `set_light` then `print("READY")`, written by the player). Every wrong
option carries its misconception tag and a `Reveal` line VANT speaks at the run. The Phase 3
loops script stays as an optional engineer's terminal that does not count.

## Power (the bus)

One bus per sector, held by `UGitsStationSubsystem` and declared by the level's `AGitsStation`
(`PowerBudget`, `ReserveRestore`). Every script declares `RunCost` and `PredictedRunCost`
(ADR-006). The station validates each terminal's script at BeginPlay: a run costs something,
the discount is real, the budget covers a full-price run (`GitsTags::ValidatePower`).

- **Running draws.** `AGitsTerminal::RunCurrent` asks VANT for the discount (any reading
  committed or confirmed, the reference's `runCost`), refuses with VANT's line when the bus
  cannot cover it, and charges after the run starts. `run_executed` is logged with the cost.
  A wrong reading costs nothing in power; it costs the discount until it is re-answered.
- **Notes draw.** Ilse's later notes carry a price; VANT refuses one the bus cannot pay.
- **The rig follows the bus.** `GitsPower::StageFor` gives nominal above half, low above a
  quarter, critical above zero, out at zero; `LightFactor` multiplies every `AGitsLight`'s
  scripted level (1, 0.45, 0.15, 0). An `AGitsLight` marked `bEmergencyOnly` does the opposite:
  amber, and only when the bus is out. The player's torch (a spot on the camera) lights at out.
- **The reserve cell.** `AGitsGenerator` is interactable like a terminal; using it restores the
  bus to `ReserveRestore` (never above), counts the draw and logs `reserve_drawn`. Running out
  is a setback with a diegetic way out, not a game over.
- **The gauge.** `AGitsPowerGauge` is the wall display's panel at half size showing a
  twenty-cell bar, holding-of-budget, reserve draws and the stage; amber for critical and out,
  which is what the palette reserves amber for.

Power is not part of the world state: the trace does not carry it, and rewind does not refund
it. It is the station's economy, not the script's physics.

## Systems

`AGitsSystemActor` has a `SystemId`; its world key is `door.<id>` or `light.<id>`.
`PoseFromWorld(World, bInstant)` reads only that key (`ReadBool` / `ReadNumber`).

- `AGitsDoor`: `Frame` and `Panel` static meshes. Open is the panel raised by `OpenHeight` cm at
  `Speed` cm/s. Its key holds a bool.
- `AGitsLight`: a point light in candelas. Its key holds a level 0..10; intensity is
  `FullIntensity * level / 10`. `SwitchKey` (e.g. `valve.lamp`) makes a bool key drive it instead;
  `bBeacon` pulses it while on (the mast).
- `AGitsVent`: a grille and a fan that spins while `vent.<id>` is true.
- `AGitsHeater` (Phase 9): a wall radiator whose warm glow follows `heater.<id>`, 0 to 3.
- `AGitsSprinkler`: a head whose spray column grows while `valve.<id>` is true.
- `AGitsConveyor`: a belt whose crates ride round while `conveyor.<id>` is true.
- `AGitsSectorGate`: a hatch that reads a door key (`door.inner`) and, released and used, loads
  the next sector's map (`UGameplayStatics::OpenLevel`).

## Sectors (Phase 9)

One map per sector: `L_Sector1_Airlock`, `L_Sector2_Greenhouse`, `L_Sector3_Logistics`,
`L_Sector4_Reactor`, each built by its `Tools/setup_sector<N>_content.py` on the shared
`Tools/gits_content_lib.py`. The 22 systems of Sectors 2 to 4 are the reference's Acts 2 to 4
re-authored line for line (level ids `a2-l01` to `a4-l06` on the script assets), each with a
fitting that answers its goal: the frost lamp, the corridor heaters, the sprinklers over the
channel planters, the west corridor going dark, the intake vent, the ballast and outbound lines,
the marker lamps, the shelf grid, the survey board going red, the core ring, and the mast beacon
that pulses when the transmission goes out. Each sector ends with its Make task from the
reference (the corridor lamp, the depot tally, the transmission).

Between maps: `UGitsProgressSubsystem` (game instance) remembers completed sectors, so a
sector's exit stays released when the player comes back through it (`AGitsStation::BeginPlay`
re-applies `SectorUnlocks`). The station and VANT state are per map; the power bus is per sector
(`PowerBudget` 80 to 90 for the larger sectors). Sector 4's west door opens on the sector and
leads nowhere the station can take you: the ending is the empty sledge rack and the mast holding
the carrier.

The recorder's wall-display view shows every open call frame as a "room" (`FGitsFrameSnapshot`),
outermost first, so Sector 4's scope and recursion scripts can be read the way Ilse's log
describes them.

`Tools/check_curriculum.py` reads the four content scripts and checks the phase's acceptance:
every concept tag in two or more systems, every misconception tag choreographed in two or more.

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

## Telemetry and the study (Phase 8)

`Telemetry/GitsTelemetry` is `UGitsTelemetrySubsystem`, a game-instance subsystem, so it outlives
a map load. It is the reference's `telemetry/` ported with the same shape:

- **The gate.** `Record(type, payload)` refuses, returning false and logging an error to
  `LogGitsTelemetry`, until `RecordConsent` has been called; it also refuses any type not in the
  reference's eighteen. Nothing touches disk before consent. `Erase()` forgets the session and
  deletes the store. VANT's `Emit` logs every event as a `LogGitsTelemetry` line regardless and
  only calls `Record` once consent is on file, so a pre-consent event is visible in the log but
  never stored.
- **The participant.** A code of the form `word-word-NN` from the reference's 22-word list,
  generated from a clock seed at start-up, shown on the consent screen, and the only identifier
  in the bundle. The session id is a GUID.
- **The bundle** is byte-compatible with the reference: `{formatVersion: 1, participantCode,
  exportedAt, consent: {version, codeCapture, recordedAt}, events: [{sessionId,
  participantCode, levelId | null, timestamp, sequence, type, payload}]}`. `Export` writes it to
  `Saved/Telemetry/exports/gits-<code>-<date>.json`; the running store is a JSON-lines file under
  `Saved/Telemetry/` so a crash keeps what was recorded. `npm run analyse -- --in <dir>` in the
  reference repo reads the exports unchanged.
- **Edits.** `Edit(line, before, after)` hashes both texts (FNV-1a, the interpreter's
  `GitsRng::HashToken`) and keeps the text only under code-capture consent (ADR-014). A PII
  tripwire (`FindPii`: email, phone, URL, long digit runs) refuses captured text that matches.
- **Which level.** `UGitsScript::LevelId` and `Act` carry the reference's level ids
  (`a1-l01` … `a1-l08`) so the analysis's level axis still works; `SetLevel` is called on
  every terminal use and cleared while an instrument is open.

What is recorded, by whom:

| event | from | payload |
|---|---|---|
| `session_start` | `RecordConsent` | `experience` band, `seed` |
| `level_start` / `level_complete` | VANT `TerminalUsed` / `TryComplete` | `levelId`, `act`, `tier` |
| `prediction_shown` / `_submitted` / `_unresolvable` | VANT | `predictionId`, `order`+`seed`; `optionId`, `correct`, `attempt`, `misconception`, `settledBy` |
| `scrub` | station `EndRewind` | `from`, `to`, `method: "key"` |
| `scrub_gate_satisfied` | VANT `HandleStep` | `predictionId`, `stepsScrubbed`, `durationMs` since the lock |
| `run_executed` | VANT `NoteRun` | `powerBefore`, `powerAfter`, `discounted`, `traceLength`, `terminatedNormally`, `capHit` |
| `error_shown` | VANT `NoteError` (parse and runtime) | `code`, `line` |
| `hint_requested` | VANT `RevealNextHint` | `tier`, `costsPower`, `affordable` |
| `reserve_drawn` | generator via VANT | `before`, `after`, `draws` |
| `edit` | terminal `SetLine` | `line`, `beforeHash`, `afterHash`, text only with capture consent |
| `instrument_started` / `_response` / `_completed` | the study panel | `instrumentId`, `occasion`, `itemId`, `value` or `skipped`, `latencyMs`; `score` |
| `session_end` | the post panel | `durationMs` |

`prediction_committed`, `goal_missed` and `sector_complete` stay log-only: they are not in the
reference's type list and the analysis does not read them.

**The instruments** (`Telemetry/GitsInstruments`) are the reference's four, ported item for item:
the tracing test (11 A/B pairs, form chosen by the code's hash parity and swapped between
occasions, options shuffled per participant and item), MEEGA+ (33 items, 13 dimensions, -2..2,
median overall), SUS (10 items, 0–100, null if incomplete) and IMI (22 items, four subscales, no
overall). Every tracing item's correct option is checked against the interpreter in
`Telemetry/Tests`. Scoring produces the same JSON the reference's `scoring.ts` does.

**In the world.** Two `AGitsInstrumentTerminal`s: the *pre* one by the entry (consent, the
experience band, the pre-test) and the *post* one beyond the airlock (the post-test, MEEGA+, SUS,
IMI, then `session_end` and the export, with the file path on the screen). `SGitsInstrumentPanel`
is one generic panel for consent and every instrument: up and down choose, Enter answers, S skips
(a skip is recorded; an item never reached is not), Escape only works at the consent screen and at
the end. Declining consent closes the panel and the game plays with nothing recorded.

## Divergences from the design docs

- **UI is C++ Slate, not Blueprint UMG.** `3D-REDESIGN.md` §4 and `CLAUDE.md` say the terminal
  is a UMG panel with RichText tinting. The screens are Slate widgets driven from C++ instead:
  the model is rebuilt from the trace on every step, and doing that through a Blueprint would be
  a loop in a Blueprint. Widget Blueprints can still wrap `UGitsScreenWidget` for layout. This
  needs an ADR (ADR-027 candidate).
- **The trace player was built in Phase 2, not Phase 3.** Statement-per-beat playback needed a
  play head to make the door open *after* the line runs, so `SeekTo` came first; Phase 3 put the
  recorder's clock and the rewind on the same head.
- **The world carries over between runs.** A run starts from the world the previous run left
  (a door the airlock script opened stays open while the lights script runs), with the station's
  initial values filling in what no run has set. The reference had one script per level and
  no such question. ADR-020's refusal still compares source only; a changed world does not
  make an unchanged script runnable. Worth an ADR if it stays.
- **Two new builtins.** `open_door(id)` / `close_door(id)` set `door.<id>`; `set_light(id, level)`
  sets `light.<id>`. The reference language spec lists valves, heaters and pumps; doors and lights
  are what Sector 1 has. Spec update needed in `LANGUAGE-SPEC.md` if they stay.
- **Collision comes through a sidecar**, not UCX meshes: Unreal's Interchange glTF importer drops
  `UCX_` meshes, so `export_kit.py` writes `<piece>.collision.json` and `import_kit.py` applies the
  boxes. See `Tools/README.md`.

## Console commands

For automation and for testing without walking: `GitsUse` (open the nearest terminal's overlay),
`GitsSetLine <n> <text>`, `GitsRun`, `GitsReset`, `GitsStatus` (run summary, play state, world
state, playback frame times and the slowest step change to the log), `GitsClose`, `GitsRewind`
and `GitsResume` (hold and release without a key), `GitsVerifyRewind`, `GitsPredict <n>` (answer
VANT with the nth shown option), `GitsHint`, `GitsVantStatus`, `GitsPower`, `GitsReserve` (use the
generator), `GitsSetPower <n>`, `GitsInsertLine <after> <text>` (Make scripts), `GitsSector`,
`GitsFrameSample <s>`, `GitsWalkTo <x> <y>`, `GitsGate` (use the nearest sector gate). The study: `GitsStudy pre|post` (open the study
panel, at the terminal if there is one), `GitsAnswer <n>` (the nth shown option, also the consent
choice and the experience band), `GitsSkip`, `GitsExport`, `GitsTelemetry` (code, consent, count).

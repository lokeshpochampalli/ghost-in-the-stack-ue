# EVIDENCE-UE.md

Factual record of the second design cycle (the Unreal Engine artefact), measured against the
repository at HEAD on 11 September 2026. Every number below was produced by a command run
against the repo or a file on disk on that date; the command is given where it is not obvious.
`UNMEASURED` marks what could not be measured and says why. Cycle 1 (the TypeScript browser
artefact) has its own record at `docs/EVIDENCE.md` in the reference repository
(`C:\Users\M9Baz\OneDrive\Desktop\ghost in the stack`, HEAD `04e67e5`, 1 Sep 2026, "docs: the
evidence file, measured against 2575935").

Conventions: paths are relative to the repo root unless stated. "Sector" is the cycle-2 unit
(one map); "Act" is the cycle-1 unit the content was re-authored from. Line counts are
`wc -l` over the named files. Rounding is against the project where it matters.

---

## 1. Repo summary

### 1.1 HEAD

| Item | Value |
|---|---|
| Commit | `df497b48dd117a9e55c486ae79654de066ab8c22` |
| Date | 2026-09-11 16:35:00 +0100 |
| Subject | phase-9: Sectors 2 to 4, twenty-two systems, sector gates, new fittings and kit, the ending |
| Branch | `main` |
| Commits in history | 15 (`git log --oneline \| wc -l`) |
| Remotes | 0 (`git remote \| wc -l`); the Phase 0 GitHub remote was never added |
| Working tree at measurement | clean |

### 1.2 Lines of C++ by module under `Source/` (`*.cpp` + `*.h`, recursive)

| Module directory | Total lines | Of which `Tests/` | Game code (non-test) | Purpose |
|---|---:|---:|---:|---|
| `Interpreter/` | 4,875 | 1,118 | 3,757 | lexer, parser + recognition pass, evaluator, trace, diff, diagnostics, RNG |
| `Station/` | 3,157 | 48 | 3,109 | station subsystem (run, play head, world), system actors, terminal, displays, gate, progress |
| `UI/` | 1,349 | 0 | 1,349 | Slate: phosphor screen, terminal overlay/editor, VANT caption, instrument panel |
| `Companion/` | 1,182 | 146 | 1,036 | VANT subsystem, prediction gate (shift), tag lists and validator |
| `Telemetry/` | 1,033 | 165 | 868 | event log, consent gate, export, instruments |
| `Recorder/` | 304 | 104 | 200 | trace step ⇄ world time, loop contexts |
| module root (`Source/GhostInTheStack/*.h,*.cpp`) | 1,028 | 0 | 1,028 | character (torch, interact), player controller (console commands), game mode, module |
| `Variant_Horror/` | 569 | 0 | 569 | UE first-person template leftovers, not referenced by the game |
| `Variant_Shooter/` | 3,410 | 0 | 3,410 | UE first-person template leftovers, not referenced by the game |
| `*.cs` (Build.cs, two Target.cs) | 80 | | | |
| **All `Source/`** | **16,987** | **1,581** | | 120 `.cpp`/`.h` files |
| **Game code excluding template variants and tests** | | | **11,347** | |

Cycle 1 for comparison, measured the same way on the reference repo: 17,306 lines of
TypeScript in 70 files under `src/`; 1,582 tests in 32 files (`npx vitest run`, all passing,
4.79 s); 30 level JSON files; 17 commits.

### 1.3 Content assets

Counted with `find Content -name "*.uasset"` (618 `.uasset`) and `-name "*.umap"` (7).

| Folder | `.uasset` | Notes |
|---|---:|---|
| `Content/Kit/` | 45 | 28 `SM_Kit_*` static meshes, 3 materials (`M_Kit_Trim`, `M_Kit_Glow`, `M_Kit_Test`), 14 `T_*` textures (Interchange re-imports left duplicate texture assets; only `T_Kit_Trim` is bound) |
| `Content/Scripts/` | 31 | 31 `DA_*` script DataAssets (`UGitsScript`) |
| `Content/Sectors/` | 0 + 4 `.umap` | `L_Sector1_Airlock`, `L_Sector2_Greenhouse`, `L_Sector3_Logistics`, `L_Sector4_Reactor` |
| `Content/__ExternalActors__/`, `__ExternalObjects__/` | 279 + 12 | one-file-per-actor storage for the four maps (World Partition style) |
| `Content/Input/` | 11 | 6 `IA_*` input actions (2 authored: `IA_Interact`, `IA_Rewind`), 2 `IMC_*` contexts, 3 template |
| `Content/FirstPerson/` | 6 + 1 `.umap` | template character/game mode/controller Blueprints, `Lvl_FirstPerson` |
| `Content/Characters/` | 128 | template mannequin |
| `Content/LevelPrototyping/` | 29 | template |
| `Content/Variant_Horror/`, `Content/Variant_Shooter/`, `Content/Weapons/` | 14 + 36 + 27, + 2 `.umap` | template |
| Non-asset files under `Content/` | 1 `.blend`, 1 `.blend1`, 28 `.glb`, 28 `.collision.json`, 1 `.png` | the kit pipeline's sources and sidecars |

### 1.4 Blueprint count by folder (`BP_*.uasset`)

| Folder | Count | Used by the game |
|---|---:|---|
| `Content/FirstPerson/Blueprints/` | 3 | yes: `BP_FirstPersonCharacter` (carries `IA_Interact`/`IA_Rewind`), `BP_FirstPersonGameMode`, `BP_FirstPersonPlayerController` |
| `Content/LevelPrototyping/Interactable/` | 3 | no (template) |
| `Content/Variant_Horror/Blueprints/` | 3 | no (template) |
| `Content/Variant_Shooter/Blueprints/` | 14 | no (template) |
| **Total** | **23** | 3 |

No gameplay logic is in Blueprints. All 20 `Gits*` classes are C++ (§4.1).

### 1.5 Toolchain

| Item | Value | Source |
|---|---|---|
| Unreal Engine | 5.8.2, changelist 56702186, branch `++UE5+Release-5.8`, launcher build | `Engine/Build/Build.version` |
| Compiler | MSVC 14.44.35228 toolchain (Visual Studio 18 BuildTools, compiler 14.44.35207), Windows SDK 10.0.28000.0 | UnrealBuildTool `Log.txt` |
| Build invocation | `Build.bat GhostInTheStackEditor Win64 Development -Project=... -WaitMutex -NoUBA` | `Tools/README.md`, memory of the session |
| Last recorded build | 262.49 s total execution (incremental, one changed file plus link, UBA disabled) | UBT `Log.txt` |
| MCP plugin | `ModelContextProtocol` ("Unreal MCP"), Version 1, VersionName 1.0, `Engine/Plugins/Experimental/` | `.uplugin` |
| Other enabled plugins | `ToolsetRegistry` 1.0, `AllToolsets` 1.0, `PythonScriptPlugin` 1.0, `EditorScriptingUtilities` 1.0, `ModelingToolsEditorMode`, `StateTree`, `GameplayStateTree` | `GhostInTheStack.uproject` |
| Third-party plugins | none | `.uproject` |
| Blender | 5.2.1 LTS | `blender --version` |
| Python (driving the editor) | 3.14.4 | |
| Node (reference analysis only) | v24.14.0 | |
| Lighting | Lumen: `r.DynamicGlobalIlluminationMethod=1`, `r.ReflectionMethod=1` | `Config/DefaultEngine.ini` |

### 1.6 Machine

Windows 11 Home 10.0.26200; Intel Core i7-13650HX; 16,092 MB RAM; NVIDIA GeForce RTX 4060
Laptop GPU (Intel UHD also present). The project, DDC and intermediates live on `D:`, an
external USB hard disk (observed 13 MB/s cold reads; the 2.4 GB precompiled header re-reads
from it after memory pressure). Every timing in this document was taken on this machine.

---

## 2. Phase record

Commit ranges from `git log --reverse`. "Status row" quotes `PHASES-3D.md`'s status table;
the criteria are quoted from the phase sections of the same file. Screenshots are every file
under `docs/screenshots/phase-N/` (105 `.png` + 2 `.json` in total).

### Phase 0 — Project and toolchain

| | |
|---|---|
| Commits | `3fab9b3`, `012ac94`, `477db00`, `ceb9d2d`, `2d8e552` (10 Sep 2026) |
| Built | UE 5.8 C++ first-person template; MCP plugin enabled and a client configured; Blender kit pipeline (`Kit.blend`, `export_kit.py`, `import_kit.py`) exporting one test cube with collision into the level; reference material imported (spec, ADRs, 30 fixtures, 3D docs); ADR-025/026. |

| Criterion | Result |
|---|---|
| project opens, compiles clean | passed (every later phase built on it; `Result: Succeeded` on every recorded build) |
| MCP connected | passed (Phase 1's test results were pulled through the MCP: `docs/screenshots/phase-1/mcp-test-results.json`) |
| one Blender-authored mesh in the level with collision | passed (`SM_Kit_TestCube`, collision from sidecar) |
| screenshot in the report | passed (5 files) |
| GitHub repo with `Content/` under Git LFS; README stating versions and server start | **not done**: 0 git remotes; no `.gitattributes` LFS rule was verified. Flagged in the status row as "Outstanding: GitHub remote". |

Screenshots (`docs/screenshots/phase-0/`):
- `01-template-level-annotated.png` — the first-person template level as inherited.
- `02-SM_Kit_TestCube-asset-thumbnail.png` — the imported test cube asset in the content browser.
- `03-cube-at-origin-pedestal-hidden.png` — the cube placed at the origin.
- `04-origin-context-pedestal-and-disc.png` — the origin with the template pedestal.
- `05-kit-mesh-render-check.png` — the cube rendered in the viewport.

### Phase 1 — Interpreter port

| | |
|---|---|
| Commit | `c9da1f1` (10 Sep 2026) |
| Built | `Source/GhostInTheStack/Interpreter/`: lexer, parser with recognition pass, eager evaluator producing `FGitsTrace`, oracle, diff, 46-entry diagnostic catalogue, golden fixture runner as automation tests. |

| Criterion | Result |
|---|---|
| all 30 golden traces pass byte for byte | passed: 30 `GhostInTheStack.Interpreter.Golden.*` tests `Result={Success}` (§3.7) |
| both caps enforced in their own units | passed: `Evaluator.Caps` test; safety cap 50,000 steps, statement cap 2,000 statement executions (§3.5) |
| recursion to depth 100 gives the friendly message | passed: fixture `outcome-call-depth` (cap 4000 statements), code `call-depth-exceeded` |
| `-7 % 3` and floor-division fixtures pass | passed: fixture `tier1-python-sign-rules` |
| no fixture edited to make it pass | passed: `git log -- Fixtures/` shows one commit, `012ac94` (the import); 0 commits modify a fixture afterwards |
| "error catalogue with all 46 codes" (phase text) | passed: 46 codes, names identical to the reference's `DiagnosticCode` union (`comm` diff empty) |

Status row says "45 automation tests green"; at HEAD there are 57 (§10).

Screenshots (`docs/screenshots/phase-1/`):
- `01-editor-automation-log.png` — the editor's automation log after the run.
- `02-mcp-test-results-table.png` — the results table as returned through the MCP.
- `mcp-test-results.json` — `{"passed": 45, "failed": 0, "total": 45, "duration_s": 1.20}` from `AutomationTestToolset.RunTestsByFilter('GhostInTheStack.Interpreter')`, 10 Sep 2026.

### Phase 2 — First system: a door

| | |
|---|---|
| Commit | `3313758` (10 Sep 2026) |
| Built | `L_Sector1_Airlock` with one corridor, one door, one terminal, one wall display; `UGitsStationSubsystem` runs the interpreter and drives the door from the trace through a play head; single-line editing; ADR-020 refusal on unchanged source. |

| Criterion | Result |
|---|---|
| walk to the terminal, read, run, watch the door open | passed (screenshots 01–03) |
| break the script, run it, watch it not open, see why on the display | passed (04, 05) |
| screenshots of both | passed |
| frame rate stays above 60 while the door animates | **passed on a technicality**: the status row records "60 fps (vsync-capped, min 59.5)". The editor viewport is capped at 60 by vsync, so "above 60" is not observable; the minimum sampled was 59.5. |
| "UMG monospace panel with keyword tinting" (phase text) | **reinterpreted**: screens are C++ Slate widgets, not UMG (ADR-027, decided by delegation in Phase 4). Keyword tinting is present. |

Screenshots (`docs/screenshots/phase-2/`):
- `01-arrive-terminal-script.png` — the terminal showing Ilse's door script.
- `02-run-in-progress.png` — the run playing, current line highlighted.
- `03-door-open-correct-script.png` — the door open after the correct script.
- `04-door-shut-broken-script.png` — the door shut after the broken script.
- `05-wall-display-explains.png` — the wall display showing what the code produced.
- `06-unchanged-run-refused.png` — the ADR-020 refusal message.
- `07-terminal-editor-overlay.png` — the editing overlay on a line.

### Phase 3 — Recorder and rewind

| | |
|---|---|
| Commit | `feb0b4b` (11 Sep 2026) |
| Built | `FGitsRecorder` (clock ⇄ statement beats, loop contexts); hold-R rewind on the same play head; terminal line highlight and wall-display bindings during rewind; `GitsVerifyRewind` console check. |

| Criterion | Result |
|---|---|
| rewind a 20-step door script and a 200-step loop | passed: the door script (5 boundaries) and the corridor-lights loop (224 steps, 93 boundaries) per the status row |
| terminal highlight matches world state at every step | passed: `GitsVerifyRewind` reported zero mismatches (status row); re-run at HEAD in §11 |
| screenshot mid-rewind | passed (01, 05) |
| under 16 ms per step change | passed: slowest step change 5.2 ms at the time (status row); re-measured at HEAD in §11 |
| "iteration 3 of 12" style loop display | passed (06: `for notch: iteration 3 of 9`) |

Screenshots (`docs/screenshots/phase-3/`):
- `01-door-mid-rewind.png` — the door panel part-way down during rewind.
- `02-door-rewound.png` — the door fully rewound to before the run.
- `03-door-replay-after-release.png` — the run replaying after release.
- `04-lights-loop-finished.png` — the corridor lights loop at its end.
- `05-lights-mid-rewind.png` — the lights loop mid-rewind.
- `06-recorder-display-iteration.png` — the wall display's iteration line and bindings.
- `07-lights-terminal-rewind.png` — the terminal's highlighted line during rewind.
- `08-lights-resumed.png` — the loop resumed after release.

### Phase 4 — VANT and prediction

| | |
|---|---|
| Commit | `03b2035` (11 Sep 2026) |
| Built | `UGitsVantSubsystem`, `FGitsShift` (the gate), ported mulberry32/FNV-1a RNG, terminal question mode, reveal at the anchored line, lock/release per ADR-030, tiered hints, intro/outro, caption HUD; ADR-027 to 030. |

| Criterion | Result |
|---|---|
| full predict-run-rewind-fix loop on the door, VANT speaking at each stage | passed (01–08) |
| the scrub gate demonstrably blocks re-answering and releases after rewind | passed (04, 05, 06); unit tests `Companion.Shift.Gate`, `Companion.Shift.Anchors` |
| options shuffled via a seeded RNG, seed recorded | passed: `Interpreter.Rng.Parity` test; `prediction_shown` carries `seed` and `order` |
| "VANT as a companion actor: a voice, ... a presence on the wall displays" (phase text) | **reinterpreted**: VANT is a world subsystem with a text caption and lines on the screens; there is no actor, and no voice (§7.4). |

Screenshots (`docs/screenshots/phase-4/`):
- `01-vant-asks.png` — the prediction question on the terminal.
- `02-committed-free.png` — a committed reading, no power drawn.
- `03-reveal-at-the-run-wrong.png` — a wrong reading revealed at the anchored line.
- `04-locked-no-question.png` — the prediction locked; no re-answer offered.
- `05-gate-released-after-rewind.png` — the gate released after rewinding to the start and watching the line.
- `06-asked-again.png` — the question re-asked.
- `07-reanswer-confirmed-against-trace.png` — the re-answer settled against the existing trace without a run.
- `08-fixed-door-open-outro.png` — the fixed script, the door open, VANT's outro.
- `09-hint-tier-2.png` — a tier-2 hint drawing power.
- `10-run-refused-pending-reading.png` — a run refused while a reading is pending.
- `11-lights-question.png` — the lights loop's prediction question.

### Phase 5 — Power

| | |
|---|---|
| Commit | `679b481` (11 Sep 2026) |
| Built | power bus on the station subsystem; per-script `RunCost`/`PredictedRunCost`; dimming stages; emergency lighting and camera torch at out; `AGitsGenerator` (reserve cell); `AGitsPowerGauge`; hints charge the bus; `GitsTags::ValidatePower` at level start. |

| Criterion | Result |
|---|---|
| run the door script four times with unchanged predictions and watch the corridor go dark | **reinterpreted**: ADR-020 makes re-running unchanged source inexpressible, so the acceptance sequence was a wrong reading (full price), then three edited runs on a 40 budget. The corridor went dark (04–07). |
| restore the generator and watch it come back | passed (09–11) |
| validator enforces `budget >= runCost` and `predictedRunCost < runCost` on every level asset | passed: `GitsTags::ValidatePower`, run on every terminal's script at `AGitsStation::BeginPlay`; test `Station.Power.Validate` |

Screenshots (`docs/screenshots/phase-5/`):
- `01-bus-full-corridor.png` — the corridor at a full bus.
- `02-overlay-run-draws-4-predicted.png` — the overlay showing a discounted run cost of 4.
- `03-after-run2-nominal.png` — after the second run, nominal stage.
- `04-after-run3-low-dimmed.png` — low stage, corridor dimmed.
- `05-out-torch.png` — bus out, camera torch on.
- `06-out-emergency-amber.png` — the amber emergency lighting.
- `07-gauge-out.png` — the wall gauge reading out.
- `08-run-refused-no-power.png` — a run refused for want of power.
- `09-generator-in-the-dark.png` — the generator in the dark.
- `10-lights-back-after-reserve.png` — lights back after the reserve draw.
- `11-gauge-restored.png` — the gauge after restore.

### Phase 6 — Blender kit and Sector 1 blockout

| | |
|---|---|
| Commit | `c255389` (11 Sep 2026) |
| Built | 15 kit pieces (`Tools/build_kit.py`, headless Blender) on one trim sheet (`Tools/make_trim_sheet.py`); Sector 1 laid out from the kit; rig, emergency lights, fills, dressing. |

| Criterion | Result |
|---|---|
| walk Sector 1 end to end at 60 fps | passed per status row: "average 59.6 to 59.9 over four windows; the only frames under 60 were the ones running console commands" (vsync-capped) |
| every mesh has collision | passed: every `SM_Kit_*` carries box collision from its sidecar (84 boxes at Phase 6; 152 `UCX_` objects in `Kit.blend` at HEAD) |
| screenshots from four positions | passed (9 files) |
| "it should look like somewhere, not like a test level" | UNMEASURED: a judgement, not a measurement |
| Lumen | enabled (§1.5) |

Screenshots (`docs/screenshots/phase-6/`):
- `01-entry-hall.png` — the entry chamber.
- `02-generator-room-from-the-doorway.png` — the generator room through its doorway.
- `03-cold-store-from-the-doorway.png` — the cold store through its doorway.
- `04-airlock-leg-terminal-and-display.png` — the leg to the airlock with terminal and display.
- `05-airlock-door.png` — the airlock door.
- `06-generator-room.png` — inside the generator room.
- `07-cold-store.png` — inside the cold store.
- `08-beyond-the-airlock-sealed-door.png` — the sealed door beyond the airlock.
- `09-trim-sheet.png` — the painted trim sheet `T_Kit_Trim`.

### Phase 7 — Sector 1 complete: the vertical slice

| | |
|---|---|
| Commit | `c9fcf3b` (11 Sep 2026) |
| Built | eight systems re-authored from Act 1 (plus one optional loops script); goals of three kinds including the Make task with tests; panels per system; vent and shift clock; sector unlock opens the airlock. |

| Criterion | Result |
|---|---|
| a playtester who has never programmed completes Sector 1 in under 45 minutes, laughs at least once, and the airlock opens; record it | **not done**. No human has played any sector (§10.4). Automation completed the sector in PIE. Status row marks it "Open". |
| eight systems: "corridor lights, two doors, the cold store, a wall clock, a vent, a pressure display, and the Make task" (phase text) | **reinterpreted**: the eight counted systems are the Act 1 levels a1-l01..a1-l08 (door log, gauge mirror, cold store, reclaimer, shift clock, door plate, manifest, approach-rig Make). The corridor-lights loop script exists as a ninth, optional, non-counting terminal. |
| each misconception has its physical failure choreographed per the table in `3D-REDESIGN.md` | **not as specified**: the failures are the generic choreography (panel shows the wrong output, the system holds, VANT reveals in words); the table's tag-specific gags (drone bonk, conveyor stall, counter flicker) are not implemented (§5). |

Screenshots (`docs/screenshots/phase-7/`):
- `01_entry_chamber.png` — the entry chamber with the door-log terminal.
- `02_doorlog_wrong_reading.png` — the door log run after a wrong reading; the system holds.
- `03_doorlog_released.png` — the door released after the reading is corrected.
- `04_pressure_display.png` — the pressure display reading west.
- `05_shift_clock_running.png` — the shift clock running.
- `06_reclaimer_vent_running.png` — the reclaimer vent fan spinning.
- `07_cold_store_door_plate.png` — the cold store door plate.
- `08_coldstore_wrong_reading.png` — the cold store setpoint after a wrong reading.
- `09_coldstore_frost.png` — the frost light on the cold store after the correct reading.
- `10_manifest_panel.png` — the manifest panel.
- `11_make_before_writing.png` — the Make terminal before any line is written.
- `12_make_written.png` — the Make script written.
- `13_approach_lit_and_airlock.png` — the approach lights up and the airlock released.
- `14_airlock_open.png` — the airlock door open.
- `15_ilses_log_on_terminal.png` — Ilse's log entry on a completed terminal.

### Phase 8 — Telemetry and instruments

| | |
|---|---|
| Commits | `b28af41`, `30e1726` (11 Sep 2026) |
| Built | `UGitsTelemetrySubsystem` (consent gate, 18 types, JSON-lines store, bundle export, PII tripwire); every event wired through VANT, the station, the terminal, the generator; the four instruments; `SGitsInstrumentPanel`; two `AGitsInstrumentTerminal`s in Sector 1; level ids on scripts. |

| Criterion | Result |
|---|---|
| a full Sector 1 playthrough exports a bundle `analyse.ts` reads without error | passed: `docs/screenshots/phase-8/sample-bundle.json` (158 events) is read by the reference's `npm run analyse` (§9.5, output kept) |
| no event before consent, proven by test | passed: `GhostInTheStack.Telemetry.ConsentGate` |
| "the four instruments delivered on a terminal in the airlock at session start and end" | **minor reinterpretation**: two terminals, one by the entry (consent, experience band, pre-test) and one beyond the airlock (post-test, MEEGA+, SUS, IMI, export) |

Screenshots (`docs/screenshots/phase-8/`):
- `01_study_terminal_pre.png` — the pre study terminal after consent, showing the participant code.
- `02_consent.png` — the consent screen with the participant code and three choices.
- `03_experience.png` — the experience band question.
- `04_pretest_item.png` — a tracing-test item (code and four options).
- `05_pretest_done.png` — the pre-test finished.
- `06_beyond_airlock.png` — the post study terminal beyond the airlock.
- `07_posttest_item.png` — a post-test item.
- `08_meega_item.png` — a MEEGA+ item with its five-point scale.
- `09_sus_item.png` — a SUS item.
- `10_imi_item.png` — an IMI item (seven-point scale).
- `11_exported.png` — the export screen with the bundle path.
- `sample-bundle.json` — the exported bundle from the automated playthrough (participant `slate-slate-82`).

### Phase 9 — Sectors 2 to 4

| | |
|---|---|
| Commit | `df497b4` (11 Sep 2026) |
| Built | three maps; 22 systems re-authored from Acts 2–4; `AGitsHeater`, `AGitsSprinkler`, `AGitsConveyor`, `AGitsSectorGate`, `UGitsProgressSubsystem`; light `SwitchKey`/`bBeacon`; 11 kit pieces; the rooms view; `Tools/check_curriculum.py`. |

| Criterion | Result |
|---|---|
| all four sectors completable | passed by automation: each sector completed in PIE and its gate loaded the next map (screenshots `s2_08`, `s3_10`; Sector 4 ends at the west door). Not played by a human. |
| every concept tag covered by two systems | passed: minimum 2 (`elif-chain`, `truthiness`, `while-loop`, `list-literal`, `list-mutation`, `local-scope`, `recursion`), §4.3 |
| every misconception tag choreographed in at least two | passed for "a distractor with that tag in two or more systems": minimum 2 (`assignment-as-equality`, `parallel-assignment`, `index-from-one`, `scope-leak`, `return-vs-print`, `recursion-no-return`), §4.4. See §5 for what "choreographed" means in the build. |
| new kit pieces "sprinkler, grow-light, heater, conveyor, more drone behaviours, reactor coolant" | partly: sprinkler, grow light, heater, conveyor, reactor core and coolant pipe built; **no drone behaviours** (the drone is a static dressing mesh) |
| "each sector's Make task"; design says Sector 3's "writes a drone route" | **deferred**: Sector 3's Make is the reference's depot tally (list statistics); the drone route waits for Phase 10 |

Screenshots (`docs/screenshots/phase-9/`):
- `s2_01_entry.png` — the Greenhouse entry chamber.
- `s2_02_frost_wrong.png` — the frost alarm after a wrong reading.
- `s2_03_heater_room.png` — the heater room with the corridor heater glowing.
- `s2_04_shelf_alarm.png` — the shelf alarm panel amber.
- `s2_05_sprinklers.png` — the channel-room sprinklers running.
- `s2_06_make_written.png` — the corridor-lamp Make script written.
- `s2_07_lamp_and_gate.png` — the west-corridor lamp lit and the Sector 3 gate released.
- `s2_08_through_to_sector3.png` — the gate has loaded Logistics.
- `s3_01_drone_bay.png` — the drone bay.
- `s3_02_intake_vent.png` — the intake vent after the purge.
- `s3_03_ballast_line.png` — the ballast conveyor with crates.
- `s3_04_survey_room.png` — the survey room panel after the walk.
- `s3_05_shelf_grid.png` — the shelf grid rack.
- `s3_06_survey_board.png` — the survey board after twelve months.
- `s3_07_make_written.png` — the depot tally Make script written.
- `s3_08_outbound_line.png` — the outbound conveyor.
- `s3_09_reactor_gate.png` — the Sector 4 gate released.
- `s3_10_through_to_sector4.png` — the gate has loaded the Reactor.
- `s4_01_approach.png` — the Reactor approach.
- `s4_02_rooms_rewind.png` — the recorder display during a rewind of "two rooms" (rewound to the start).
- `s4_03_depot_form_room.png` — the depot-form room.
- `s4_04_core_lit.png` — the reactor core ring lit after the countdown.
- `s4_05_how_long.png` — "how long" run.
- `s4_06_make_written.png` — the transmission Make script written.
- `s4_07_west_door_open_mast.png` — the west door open, the mast beacon lit.
- `s4_08_empty_rack.png` — the empty sledge rack by the west door.

---

## 3. The interpreter port

Location: `Source/GhostInTheStack/Interpreter/` (16 files: `GitsAst`, `GitsDiagnostics`,
`GitsEvaluator`, `GitsLexer`, `GitsParser`, `GitsRng`, `GitsTrace`, `GitsValue.cpp`,
`GitsTypes.h`, and `Tests/`). Documentation: `docs/INTERPRETER-PORT.md`.

### 3.1 `FGitsStep` as it is in source (`GitsTypes.h`, lines 259–279)

```cpp
/** One AST node evaluation (ADR-001). */
struct FGitsStep
{
	int32 Index = 0;
	FString NodeId;
	FGitsSpan Span;
	EGitsStepKind Kind = EGitsStepKind::Literal;
	/** Expression nesting depth within the statement. Zero at the statement itself. */
	int32 Depth = 0;
	/** Which statement execution this step belongs to. */
	int32 StmtIndex = 0;
	bool bIsStatementBoundary = false;
	/** The call stack, innermost last. Deep-copied. */
	TArray<FGitsFrameSnapshot> Frames;
	TArray<FString> Output;
	TArray<FGitsEffect> Effects;
	bool bHasOracleRead = false;
	FGitsOracleRead OracleRead;
	/** One-line summary for the golden-trace format and the runtime views. */
	FString Label;
};
```

`FGitsFrameSnapshot` (lines 237–249): `FString FunctionName; TArray<TPair<FString, FGitsValue>> Bindings; bool bHasCallSite; FGitsSpan CallSite;`.
`FGitsTrace` (lines 290–300): `TArray<FGitsStep> Steps; FGitsOutcome Outcome; TArray<FString> Output; int32 Seed; FGitsWorldState InitialWorld; int32 StatementCount;`.
`EGitsStepKind` has 19 values: `Def, Return, Literal, Name, BinOp, UnaryOp, BoolOp, Compare, List, Subscript, Call, Method, Assign, AugAssign, Expr, Branch, Iterate, Break, Continue`.

### 3.2 The WorldOracle interface (`GitsTypes.h`, lines 206–224)

```cpp
struct FGitsQuery
{
	FString Kind = TEXT("sensor");
	FString Id;
};

/** The injected reader (ADR-003). Pure, synchronous, total: must return for any query. */
typedef TFunction<FGitsValue(const FGitsWorldState&, const FGitsQuery&)> FGitsWorldOracle;

namespace GitsWorld
{
	extern const TCHAR* ClockKey;     // the tick counter wait(n) advances
	extern const TCHAR* LogCountKey;  // how many lines the station log has taken
	FGitsWorldState Reduce(const FGitsWorldState& World, const FGitsEffect& Effect);
	FGitsWorldState ReduceAll(const FGitsWorldState& World, const TArray<FGitsEffect>& Effects);
	FGitsValue NullOracle(const FGitsWorldState&, const FGitsQuery&);
}
```

Every oracle response is recorded in the step (`bHasOracleRead`, `OracleRead {Query, Value}`).
`GitsTrace::ReplayOracle(Trace)` answers subsequent reads from the recorded values in order and
never consults the world. In the game, `AGitsStation::Read` is the oracle: a declared sensor value
plus `DriftPerTick × clock`.

### 3.3 The trace accessor (`GitsTrace.h`, namespace `GitsTrace`)

| Function | Purpose |
|---|---|
| `const FGitsStep& At(const FGitsTrace&, int32)` | the only indexing entry point (ADR-009) |
| `int32 Length(const FGitsTrace&)` | |
| `TArray<int32> StatementBoundaries(const FGitsTrace&)` | indices where `bIsStatementBoundary` |
| `TArray<FString> OutputAt(const FGitsTrace&, int32)` | printed text up to a step |
| `FGitsWorldState WorldAt(const FGitsTrace&, int32)` | the initial world folded with effects up to a step |
| `TArray<FGitsEffect> AllEffects(const FGitsTrace&)` | |
| `TArray<TPair<FString, FGitsValue>> BindingsAt(const FGitsTrace&, int32)` | innermost frame's bindings |
| `FGitsWorldOracle ReplayOracle(const FGitsTrace&)` | |
| `FGitsDiffResult Diff(const FGitsTrace& A, const FGitsTrace& B)` | ADR-002 |
| `FString Serialise(const FGitsTrace&)` | the golden-trace text (ADR-015) |
| `FString DescribeOutcome`, `DescribeEffect`, `QuoteJson` | |

The station subsystem poses systems from `WorldAt(Trace, PlayIndex)` merged with the level's
overrides; the wall display reads `Steps[Head].Frames` directly for the rooms view (the one
place a raw step is read outside the accessor, added in Phase 9).

### 3.4 Diff

Signature: `FGitsDiffResult GitsTrace::Diff(const FGitsTrace& A, const FGitsTrace& B)`.
`FGitsDiffResult` carries `bDiverged, AtBoundary, AStepIndex, BStepIndex, TArray<EGitsDivergenceKind> Kinds, TArray<FString> Summary`.

Algorithm (`GitsTrace.cpp`, "diff (ADR-002)"): The two traces are aligned by statement-boundary
sequence, not by raw step index, and the shared prefix of boundaries is walked in order. At each
boundary pair the frames are compared depth for depth and name for name, with a value counted as
changed when its kind or its value differs (so `2` and `2.0` differ). The printed output up to each
boundary is compared as a whole, and the cumulative effect lists up to each boundary are compared
element by element. The first boundary at which any of bindings, output or effects differ is
reported with the kinds that differed and one summary line per difference. NodeId, span and step
index are never compared.

### 3.5 Caps and units (`GitsTypes.h` `GitsLimits`, `GitsEvaluator.cpp`)

| Cap | Constant | Unit | Where enforced | Outcome |
|---|---:|---|---|---|
| Safety | `SafetyStepCap = 50000` | raw steps | `if (Trace.Steps.Num() >= GitsLimits::SafetyStepCap)` | `SafetyCapExceeded`, code `safety-cap-exceeded` |
| Pedagogical | `DefaultStatementCap = 2000`, per-run `Options.StatementCap` | statement executions (counted on `bIsStatementBoundary`, ADR-021) | `if (bBoundary && StatementCount >= Options.StatementCap)` | `StatementCapExceeded`, code `statement-cap-exceeded` |

Every script asset sets `statement_cap = 2000` (`ensure_script` in the content scripts). Two
fixtures override the cap in the test harness: `outcome-statement-cap` at 40 and
`outcome-call-depth` at 4000. Recursion depth is a third limit (`call-depth-exceeded`), inherited
from the reference's `MAX_CALL_DEPTH`.

### 3.6 Counts

| Item | Count | Source |
|---|---:|---|
| AST node kinds (`EGitsNodeKind`) | 24: `Program`; 13 expressions (`Num, Str, Bool, NoneLit, Name, BinOp, UnaryOp, BoolOp, Compare, Call, Method, Subscript, List`); 10 statements (`Assign, AugAssign, ExprStmt, If, While, For, FunctionDef, Return, Break, Continue`) | `GitsAst.h` |
| Step kinds (`EGitsStepKind`) | 19 | `GitsTypes.h` |
| Diagnostic codes | 46 (15 lexical/structural, 19 recognition "not here" + "not yet", 12 runtime) | `GitsDiagnostics.cpp`, `CodeCount()`; names identical to the reference's 46 (§16.1) |
| Station builtins | 9: `open_valve, close_valve, set_heater, log, wait, read_sensor, open_door, close_door, set_light` | `GitsEvaluator.cpp` `StationBuiltinNames()` |
| Golden fixtures | 30 pairs (60 files in `Fixtures/`) | `ls Fixtures \| wc -l` |

### 3.7 Fixture result

All 30 pass byte for byte. Command (from the project root, editor closed or open):

```
Tools\run_interpreter_tests.cmd
```

which runs

```
UnrealEditor-Cmd.exe GhostInTheStack.uproject -ExecCmds="Automation RunTests GhostInTheStack; Quit" -unattended -nopause -nullrhi -NoSound -nosplash -log=GitsTests.log
```

and prints `passed=57 failed=0` (11 Sep 2026, three runs during Phases 8–9, all identical). The
30 `GhostInTheStack.Interpreter.Golden.<fixture>` results are listed in §16.4; a 31st test,
`Interpreter.GoldenCoverage`, checks that every fixture's `-- covers:` line is represented.

Fixture integrity by git: `git log --format=%h -- Fixtures/` returns exactly one commit,
`012ac94` ("Add reference material: language spec, decisions, golden fixtures, 3D scaffold docs",
10 Sep 2026); `git log --diff-filter=M -- Fixtures/ | wc -l` is 0.

### 3.8 Where the C++ differs from the TypeScript reference

From `docs/INTERPRETER-PORT.md` "Known differences". None was exposed by a fixture: all 30 pass,
so each difference is outside what the fixtures exercise.

| Difference | Fixture that exposed it |
|---|---|
| Eager evaluation into a recorded trace, not a generator | none (trace identical) |
| No C++ exceptions; runtime errors unwind through status returns | none |
| Integers are 64-bit (Python's are unbounded); `2 ** 70` wraps | none; no level content approaches it |
| Float formatting follows Python (`1e+16`, `inf`) where the reference printed JavaScript's `Infinity` | none; no fixture value affected |
| `in` on strings, `+`/`*` on lists, iterating a string are allowed rather than refused | none |
| `range` outside a `for` header is runtime `bad-range`, as the reference chose; user `def range` shadows it | none |

Three inputs the fixtures do not carry were taken from the reference harness (per-fixture caps,
the fixture oracle, the `-- covers:` lines) and live in `Tests/GitsGoldenTests.cpp`.

---

## 4. Systems inventory

### 4.1 Classes

The repo layout places system actors under `Source/GhostInTheStack/Station/`, not `Systems/`.
All 20 `Gits*` classes (`grep "class GHOSTINTHESTACK_API"`):

| Class | Base | Role |
|---|---|---|
| `AGitsSystemActor` | `AActor` | abstract; `SystemId`, `PoseFromWorld(World, bInstant)`, `ResetPose()`, `ReadBool/ReadNumber` |
| `AGitsDoor` | `AGitsSystemActor` | `door.<id>` bool → panel rises `OpenHeight` at `Speed` |
| `AGitsLight` | `AGitsSystemActor` | `light.<id>` 0–10 → candela; `SwitchKey` (bool key), `bEmergencyOnly`, `bBeacon` |
| `AGitsVent` | `AGitsSystemActor` | `vent.<id>` bool → fan spin-up/coast |
| `AGitsClock` | `AGitsSystemActor` | `clock.<id>` → hands run |
| `AGitsHeater` | `AGitsSystemActor` | `heater.<id>` 0–3 → warm glow winds up |
| `AGitsSprinkler` | `AGitsSystemActor` | `valve.<id>` bool → spray column grows, mist light |
| `AGitsConveyor` | `AGitsSystemActor` | `conveyor.<id>` bool → crates ride the belt |
| `AGitsSectorGate` | `AGitsSystemActor` + `IGitsInteractable` | `DoorKey` bool → released; `OpenLevel(NextLevel)` |
| `AGitsTerminal` | `AActor` + `IGitsInteractable` | runs a `UGitsScript`; line edits; run gate |
| `AGitsWallDisplay` | `AActor` | per-script panel; rewind view with frames as rooms |
| `AGitsPowerGauge` | `AActor` | bus readout |
| `AGitsGenerator` | `AActor` + `IGitsInteractable` | reserve cell |
| `AGitsInstrumentTerminal` | `AActor` + `IGitsInteractable` | study terminal, `Occasion` pre/post |
| `AGitsStation` | `AActor` | per-map config: sensors, initial world, budget, sector unlock |
| `UGitsScript` | `UDataAsset` | one system's script and curriculum data |
| `UGitsStationSubsystem` | `UWorldSubsystem` | run, trace, play head, rewind, power, world overrides |
| `UGitsVantSubsystem` | `UWorldSubsystem` | predictions, gate, hints, goals, sector completion |
| `UGitsTelemetrySubsystem` | `UGameInstanceSubsystem` | event log |
| `UGitsProgressSubsystem` | `UGameInstanceSubsystem` | completed sectors across maps |

Slate widgets (not UObjects): `SGitsScreen`, `UGitsScreenWidget`, `SGitsTerminalEditor`, `SGitsVantCaption`, `SGitsInstrumentPanel`.

### 4.2 System instances (31 script assets across four sectors)

Extracted from `Tools/setup_sector{1..4}_content.py`, which author the assets. Edit policy:
`readonly` = no editable lines; `lines n,m` = those lines editable; `free` = free edit (Make).
Goal kind: `output` = the printed lines must match `goal_output` (and all predictions satisfied);
`tests` = `TestCase`s over output/world (Make); `world` = a world key must equal a value.
Power budget is the sector's bus (`PowerBudget/ReserveRestore`); costs are `RunCost/PredictedRunCost`.

| Sector | Asset (`Content/Scripts/`) | Level id | Tier | Concept tags | Misconception tags on distractors | Edit | Goal | Cost (full/predicted) | Unlock on goal | Bus |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | `DA_S1_DoorLog` | a1-l01 | 1 | variable-assignment, output | sequence-ignored, type-confusion | readonly | output | 10/3 | `door.entry=true` | 60/30 |
| 1 | `DA_S1_GaugeMirror` | a1-l02 | 1 | variable-assignment, reassignment, output | operator-confusion, parallel-assignment, sequence-ignored | readonly | output | 10/3 | `gauge.west=12` | 60/30 |
| 1 | `DA_S1_ColdStore` | a1-l03 | 1 | variable-assignment, reassignment, arithmetic, output | assignment-as-equality, operator-confusion, type-confusion | readonly | output | 12/4 | `light.coldstore=8` | 60/30 |
| 1 | `DA_S1_Reclaimer` | a1-l04 | 1 | data-types, output | sequence-ignored, type-confusion | readonly | output | 10/3 | `vent.reclaimer=true` | 60/30 |
| 1 | `DA_S1_ShiftClock` | a1-l05 | 1 | arithmetic, type-coercion | fencepost, operator-confusion, type-confusion | readonly | output | 10/3 | `clock.shift=true` | 60/30 |
| 1 | `DA_S1_DoorPlate` | a1-l06 | 1 | string-ops, output | fencepost, loop-runs-once, operator-confusion, type-confusion | readonly | output | 10/3 | `door.coldstore=true` | 60/30 |
| 1 | `DA_S1_Manifest` | a1-l07 | 1 | type-coercion, string-ops | operator-confusion, sequence-ignored, type-confusion | readonly | output | 10/3 | `manifest.outbound=true` | 60/30 |
| 1 | `DA_S1_ApproachLights` | a1-l08 | 1 | variable-assignment, output, type-coercion | (none: Make) | free | tests | 8/4 | (sector unlock `door.inner=true`) | 60/30 |
| 1 | `DA_Corridor_Lights` (optional, does not count) | s1-lights-optional | 3 | for-loop, range, nested-loop, accumulator | fencepost, loop-runs-once | readonly | world (`light.corridor=9`) | 12/4 | | 60/30 |
| 2 | `DA_S2_FrostAlarm` | a2-l01 | 2 | conditional, comparison, output | branch-both, inverted-comparison | readonly | output | 12/4 | `light.frost=10` | 90/45 |
| 2 | `DA_S2_HeaterLadder` | a2-l02 | 2 | elif-chain, comparison, conditional | branch-both, inverted-comparison, sequence-ignored | readonly | output | 14/5 | `heater.corridor=2` | 90/45 |
| 2 | `DA_S2_Interlock` | a2-l03 | 2 | boolean-logic, conditional, data-types | branch-both, operator-confusion | lines 4, 5 | output | 12/4 | `light.interlock=10` | 90/45 |
| 2 | `DA_S2_EmptyChannel` | a2-l04 | 2 | truthiness, conditional, data-types | branch-both, operator-confusion, type-confusion | lines 4, 5, 6 | output | 14/5 | `valve.channel=true` | 90/45 |
| 2 | `DA_S2_Fallback` | a2-l05 | 2 | boolean-logic, truthiness, data-types, string-ops | operator-confusion, type-confusion | lines 4, 5 | output | 14/5 | `light.nameplate=10` | 90/45 |
| 2 | `DA_S2_IceLine` | a2-l06 | 2 | elif-chain, comparison, conditional, reassignment | branch-both, inverted-comparison | lines 5–12 | output | 14/5 | `light.shelf=10` | 90/45 |
| 2 | `DA_S2_NightWatch` | a2-l07 | 2 | conditional, comparison, boolean-logic | branch-both, inverted-comparison, type-confusion | lines 4, 5 | output | 14/5 | `light.west=0` | 90/45 |
| 2 | `DA_S2_CorridorLamp` | a2-l08 | 2 | conditional, comparison, output | (none: Make) | free | tests | 10/5 | (sector unlock `door.dronebay=true`) | 90/45 |
| 3 | `DA_S3_PurgeCycle` | a3-l01 | 3 | while-loop, comparison, output, reassignment | fencepost, loop-runs-once | lines 4 | output | 14/5 | `vent.intake=true` | 90/45 |
| 3 | `DA_S3_SensorSweep` | a3-l02 | 3 | for-loop, range, accumulator | accumulator-reset, assignment-as-equality, fencepost | lines 4 | output | 14/5 | `light.channelboard=0` | 90/45 |
| 3 | `DA_S3_BallastTally` | a3-l03 | 3 | augmented-assignment, accumulator, range, for-loop, arithmetic | accumulator-reset, fencepost, loop-runs-once | lines 4, 5 | output | 14/5 | `conveyor.ballast=true` | 90/45 |
| 3 | `DA_S3_MarkerLine` | a3-l04 | 3 | list-literal, indexing | fencepost, index-from-one, operator-confusion, type-confusion | lines 4 | output | 16/5 | `light.markers=10` | 90/45 |
| 3 | `DA_S3_TheWalk` | a3-l05 | 3 | for-loop, accumulator, indexing, augmented-assignment | accumulator-reset, fencepost, loop-runs-once, type-confusion | lines 4, 5 | output | 14/5 | `light.survey=10` | 90/45 |
| 3 | `DA_S3_TheGrid` | a3-l06 | 3 | nested-loop, while-loop, for-loop, list-mutation, range | fencepost, index-from-one, loop-runs-once, sequence-ignored | lines 7–10 | output | 16/5 | `light.grid=10` | 90/45 |
| 3 | `DA_S3_LastWalk` | a3-l07 | 3 | for-loop, accumulator, list-mutation, augmented-assignment, indexing | accumulator-reset, fencepost, loop-runs-once, parallel-assignment, type-confusion | lines 5, 6, 7 | output | 16/6 | `light.board=10` | 90/45 |
| 3 | `DA_S3_DepotTally` | a3-l08 | 3 | for-loop, nested-loop, accumulator, list-literal, indexing | (none: Make) | free | tests | 12/6 | `conveyor.outbound=true` (+ sector `door.reactor=true`) | 90/45 |
| 4 | `DA_S4_Conversion` | a4-l01 | 4 | function-def, parameters, return-value | arg-param-identity, operator-confusion, return-vs-print, sequence-ignored | lines 7 | output | 16/5 | `light.depot=10` | 90/45 |
| 4 | `DA_S4_TheRooms` | a4-l02 | 4 | local-scope, parameters, return-value, function-def | arg-param-identity, return-vs-print, scope-leak | lines 4 | output | 16/5 | `light.tally=10` | 90/45 |
| 4 | `DA_S4_TwoRooms` | a4-l03 | 4 | call-stack, parameters, local-scope, return-value, arithmetic | arg-param-identity, fencepost, scope-leak, sequence-ignored, type-confusion | lines 11, 12 | output | 18/6 | `light.report=10` | 90/45 |
| 4 | `DA_S4_LadderDown` | a4-l04 | 4 | recursion, call-stack, return-value, conditional | fencepost, loop-runs-once, recursion-no-return | lines 10 | output | 18/6 | `light.core=10` | 90/45 |
| 4 | `DA_S4_HowLong` | a4-l05 | 4 | recursion, function-def, parameters, return-value, call-stack, arithmetic | arg-param-identity, fencepost, recursion-no-return, type-confusion | lines 13 | output | 18/6 | `light.doorlog=10` | 90/45 |
| 4 | `DA_S4_Transmission` | a4-l06 | 4 | function-def, parameters, return-value, type-coercion, string-ops | (none: Make) | free | tests | 12/6 | `light.mast=10` (+ sector `door.west=true`) | 90/45 |

Totals: 31 assets; 30 counted systems (the corridor-lights script does not count for its sector);
27 prediction-gated, 4 Make (one per sector); 50 predictions; 163 options of which 113 are
tagged distractors; 68 hints (31 at tier 1, all free; 32 at tier 2, costing 3–6; 5 at tier 3,
costing 8–12).

Divergences from cycle 1 to note: the reference's per-level `power.budget` (40–160) became one
bus per sector (60 or 90); the reference `editable.kind` `region` became explicit line lists.
Concept tags compared with the reference level JSON (`src/content/levels/*.json`): Sector 1's
eight are identical; seven levels carry one concept tag added in cycle 2 and none dropped:
`a2-l05` +string-ops, `a2-l06` +reassignment, `a2-l07` +boolean-logic, `a3-l01` +reassignment,
`a3-l03` +arithmetic, `a4-l03` +arithmetic, `a4-l05` +arithmetic. Every distractor's
misconception tag and every source line is as in the reference.

### 4.3 Systems per concept tag (`python Tools/check_curriculum.py`)

| Concept | Systems | Which |
|---|---:|---|
| variable-assignment | 4 | S1 DoorLog, GaugeMirror, ColdStore, ApproachLights |
| reassignment | 4 | S1 GaugeMirror, ColdStore; S2 IceLine; S3 PurgeCycle |
| data-types | 4 | S1 Reclaimer; S2 Interlock, EmptyChannel, Fallback |
| type-coercion | 4 | S1 ShiftClock, Manifest, ApproachLights; S4 Transmission |
| arithmetic | 5 | S1 ColdStore, ShiftClock; S3 BallastTally; S4 TwoRooms, HowLong |
| string-ops | 4 | S1 DoorPlate, Manifest; S2 Fallback; S4 Transmission |
| output | 9 | S1 ×6; S2 FrostAlarm, CorridorLamp; S3 PurgeCycle |
| conditional | 8 | S2 ×7; S4 LadderDown |
| elif-chain | **2** | S2 HeaterLadder, IceLine |
| boolean-logic | 3 | S2 Interlock, Fallback, NightWatch |
| comparison | 6 | S2 ×5; S3 PurgeCycle |
| truthiness | **2** | S2 EmptyChannel, Fallback |
| while-loop | **2** | S3 PurgeCycle, TheGrid |
| for-loop | 7 | S1 Corridor_Lights; S3 ×6 |
| range | 4 | S1 Corridor_Lights; S3 SensorSweep, BallastTally, TheGrid |
| accumulator | 6 | S1 Corridor_Lights; S3 ×5 |
| nested-loop | 3 | S1 Corridor_Lights; S3 TheGrid, DepotTally |
| augmented-assignment | 3 | S3 BallastTally, TheWalk, LastWalk |
| list-literal | **2** | S3 MarkerLine, DepotTally |
| indexing | 4 | S3 MarkerLine, TheWalk, LastWalk, DepotTally |
| list-mutation | **2** | S3 TheGrid, LastWalk |
| function-def | 4 | S4 Conversion, TheRooms, HowLong, Transmission |
| parameters | 5 | S4 ×5 |
| return-value | 6 | S4 ×6 |
| local-scope | **2** | S4 TheRooms, TwoRooms |
| call-stack | 3 | S4 TwoRooms, LadderDown, HowLong |
| recursion | **2** | S4 LadderDown, HowLong |

No concept is under 2. Seven are at exactly 2 (bold). The optional, non-counting
`DA_Corridor_Lights` contributes to four of the counts (for-loop, range, accumulator,
nested-loop); without it, nested-loop is 2 and range is 3.

### 4.4 Systems per misconception tag (a distractor carrying the tag)

| Misconception | Systems | Which |
|---|---:|---|
| assignment-as-equality | **2** | S1 ColdStore; S3 SensorSweep |
| parallel-assignment | **2** | S1 GaugeMirror; S3 LastWalk |
| sequence-ignored | 8 | S1 DoorLog, GaugeMirror, Reclaimer, Manifest; S2 HeaterLadder; S3 TheGrid; S4 Conversion, TwoRooms |
| type-confusion | 14 | S1 ×6; S2 EmptyChannel, Fallback, NightWatch; S3 MarkerLine, TheWalk, LastWalk; S4 TwoRooms, HowLong |
| branch-both | 6 | S2 FrostAlarm, HeaterLadder, Interlock, EmptyChannel, IceLine, NightWatch |
| inverted-comparison | 4 | S2 FrostAlarm, HeaterLadder, IceLine, NightWatch |
| operator-confusion | 10 | S1 GaugeMirror, ColdStore, ShiftClock, DoorPlate, Manifest; S2 Interlock, EmptyChannel, Fallback; S3 MarkerLine; S4 Conversion |
| loop-runs-once | 8 | S1 DoorPlate, Corridor_Lights; S3 PurgeCycle, BallastTally, TheWalk, TheGrid, LastWalk; S4 LadderDown |
| fencepost | 13 | S1 ShiftClock, DoorPlate, Corridor_Lights; S3 ×7; S4 TwoRooms, LadderDown, HowLong |
| accumulator-reset | 4 | S3 SensorSweep, BallastTally, TheWalk, LastWalk |
| index-from-one | **2** | S3 MarkerLine, TheGrid |
| scope-leak | **2** | S4 TheRooms, TwoRooms |
| return-vs-print | **2** | S4 Conversion, TheRooms |
| arg-param-identity | 4 | S4 Conversion, TheRooms, TwoRooms, HowLong |
| recursion-no-return | **2** | S4 LadderDown, HowLong |

No misconception is under 2. Six are at exactly 2 (bold). Distractor count per tag (options,
not systems): type-confusion 25, fencepost 17, operator-confusion 12, loop-runs-once 11,
sequence-ignored 10, branch-both 7, arg-param-identity 6, accumulator-reset 5,
inverted-comparison 5, return-vs-print 4, index-from-one 3, and 2 each for the remaining four.

---

## 5. Misconception choreography

What "choreographed" means in the build at HEAD. A wrong reading is settled when the anchored
statement executes during playback (ADR-030). At that moment, for every tag alike: the wall
panel shows the output the code actually produced, VANT speaks the distractor's `Reveal` text
(the `reveal` field on the option, authored per distractor), the prediction locks until the
player rewinds to the start and watches the line, and the system's goal is withheld so the
fitting stays in its initial state (door shut, lamp dark, vent still). There is no tag-specific
physical gag in any system. The tag-specific failures in `docs/3D-REDESIGN.md` §"Bugs are
choreography" (drone bonk, conveyor stall, counter flicker, frost creep, sprinklers inverted,
drone drift, wrong crate) are **not implemented**; the drone is a static dressing mesh.

| Tag | What the player sees at HEAD | Systems (distractor present) | Screenshot | Designed gag (3D-REDESIGN) | Implemented as designed |
|---|---|---|---|---|---|
| assignment-as-equality | panel prints the computed value; VANT: "Line 6 is not a claim about base_temp. It is an instruction..." | S1 ColdStore; S3 SensorSweep | `phase-7/08_coldstore_wrong_reading.png` (wrong reading on the cold store; which distractor was chosen depends on the seeded shuffle) | heater set once, frost creeps | no |
| parallel-assignment | panel shows west still 12; VANT: "West took a copy of east on line 6. It did not take a wire to it" | S1 GaugeMirror; S3 LastWalk | none tag-specific | (not in table) | n/a |
| sequence-ignored | panel shows the earlier line's value; VANT: "Line 5 ran before line 8..." | 8 systems | `phase-7/02_doorlog_wrong_reading.png` (door log, wrong reading) | (not in table) | n/a |
| type-confusion | panel shows the value, not the name; VANT: "It printed the name, not what the name keeps. The interlock wants a number." | 14 systems | `phase-4/03-reveal-at-the-run-wrong.png` (Phase 4 door script reveal) | display reads `4metres`; door does nothing | partly: the door does nothing (goal withheld); no `4metres` display |
| branch-both | one printed line, not two; VANT: "One question, one branch. The else is the road not taken" | 6 systems (S2) | `phase-9/s2_02_frost_wrong.png` (frost alarm, wrong reading) | (not in table) | n/a |
| inverted-comparison | FROST printed, lamp lit; VANT: "Minus nineteen is the colder" | 4 systems (S2) | `phase-9/s2_02_frost_wrong.png` | sprinklers run when wet | no: the sprinklers are on a truthiness system, not a comparison one |
| operator-confusion | panel shows the copied/subtracted value; VANT: "Nothing on this panel adds. Line 6 copies." | 10 systems | none tag-specific | (not in table) | n/a |
| loop-runs-once | panel shows all iterations' output; VANT: "Told twelve times, it made twelve." | 8 systems | `phase-3/04-lights-loop-finished.png` (loop output, not a wrong reading) | conveyor advances one item and stops | no: conveyors run or stop whole |
| fencepost | panel shows the count; VANT: "range(9) is nought to eight. Nine notches" | 13 systems | none tag-specific | drone flies one bay too far, bonks | no |
| accumulator-reset | panel shows the running total; VANT: "total is made once, on line 4, outside the loop" | 4 systems (S3) | none | counter flickers to zero, door never opens | no |
| index-from-one | panel shows position 2's marker; VANT: "Positions count from nought." | S3 MarkerLine, TheGrid | none | wrong crate picked, one bay off | no |
| scope-leak | panel shows outer count 0; recorder shows two rooms with two `count`s | S4 TheRooms, TwoRooms | `phase-9/s4_02_rooms_rewind.png` (rewound to start; rooms not visible in the frame) | drone drifts | no |
| return-vs-print | panel shows the returned value printed; VANT: "return hands the answer back out to the print that asked" | S4 Conversion, TheRooms | none | display prints, door gets nothing | no |
| arg-param-identity | panel shows the doubled value; VANT: "metres is the number handed over, called by a new name inside" | 4 systems (S4) | none | (not in table) | n/a |
| recursion-no-return | panel shows BOTTOM after 3,2,1; VANT: "Line 8 hands back whatever the room below handed it" | S4 LadderDown, HowLong | `phase-9/s4_04_core_lit.png` (correct outcome, core ring) | (not in table) | n/a |

Reveal text is the `reveal` argument of `option(...)` in the four content scripts; §4.2's
distractor count (113) is the number of authored reveals. Full reveal text per distractor can be
regenerated with the extractor used for this document (`grep -o 'option("[a-z]", ...'` over
`Tools/setup_sector*_content.py`).

---

## 6. The recorder

Source: `Source/GhostInTheStack/Recorder/GitsRecorder.h/.cpp` (200 lines), driven by
`UGitsStationSubsystem` (`Station/GitsStationSubsystem.cpp`).

### 6.1 Step index to world time

- `FGitsRecorder::Build(Trace, StatementsPerSecond)` takes the trace's statement boundaries as
  beats; `StatementsPerSecond = 2.5f` (subsystem default), so `BeatInterval = 0.4 s`.
- `TimeOfStep(i)` = the beat carrying step `i` × interval; `StepAtTime(t)` = the step the head is
  on at world time `t` (−1 before the first beat; the last step at or after `TotalTime`);
  `BeatOfStep(i)` = 0-based beat of the boundary that owns `i`; `TotalTime = (NumBeats + 1) × interval`.
- Playback: each tick the subsystem advances `PlayClock`, asks `StepAtTime`, and calls
  `SeekTo(Step, false)` when it changes. Playback is forward-only through the recorded trace.
- Rewind: while the key is held, `PlayClock` decreases at `Lerp(RewindSpeedStart=1, RewindSpeedMax=4, held/RewindRampSeconds=3)`
  times real time, and the same `StepAtTime` → `SeekTo` path runs. Release resumes forward
  play from wherever the clock got to. Nothing is simulated backwards.

### 6.2 How systems pose from (trace, stepIndex)

`SeekTo(StepIndex, bInstant)`: `CurrentWorld = GitsTrace::WorldAt(Trace, StepIndex)` merged with
`LevelOverrides` (world keys set outside runs, e.g. unlocks), then `PoseSystems(bInstant)` calls
`PoseFromWorld(CurrentWorld, bInstant)` on every registered `AGitsSystemActor`. Each actor reads
only its own key and animates toward the target (door height, light level, fan speed, heater
glow, spray amount, conveyor speed); `bInstant` snaps. `OnStepChanged` also fires so the
terminal highlights the step's line and the wall display rebuilds. Systems never see the trace;
they see a world state.

### 6.3 Rewind through a loop

`FGitsRecorder::LoopsAt(StepIndex)` returns the stack of `FGitsLoopContext {NodeId, Header, Line,
Iteration, Total, bEnded}` enclosing the step, outermost first, computed per beat at build time
the way the reference ribbon collapses loops (ADR-008): a loop owns every boundary whose span
sits inside its own. The wall display's rewind view prints one line per context via
`Describe()` (e.g. `for notch: iteration 3 of 9`), then the bindings, then the world state. The
head still steps through every boundary inside the loop (no collapsing of playback), so the
door/lights pose at every iteration; the collapsing is in the display only. Test:
`GhostInTheStack.Recorder.Loops`.

### 6.4 Measured rewind step latency

`UGitsStationSubsystem` records `MaxSeekMs` (the slowest `SeekTo` since the last state change)
and playback frame statistics; `GitsStatus` prints them. Phase 3 recorded 5.2 ms slowest step
change against the 16 ms target (status row). Re-measured at HEAD in §11.

---

## 7. VANT

### 7.1 Dialogue line count by sector

Counted from the content scripts (one "line" = one authored string). Reveals are per distractor;
prompts are per prediction; the sector line is the station's `sector_complete_line`.

| Sector | Intros | Outros | Ilse's logs | Hints | Prediction prompts | Options | Reveals | Sector line |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 (9 scripts) | 9 | 9 | 8 | 19 | 12 | 42 | 30 | 1 |
| 2 (8) | 8 | 8 | 8 | 18 | 11 | 34 | 23 | 1 |
| 3 (8) | 8 | 8 | 8 | 17 | 16 | 51 | 35 | 1 |
| 4 (6) | 6 | 6 | 6 | 14 | 11 | 36 | 25 | 1 |
| **Total** | **31** | **31** | **30** | **68** | **50** | **163** | **113** | **4** |

Code-side lines (not per sector): 19 `Speak(` call sites in C++ (15 in `GitsVant.cpp`, 2 in
`GitsGenerator.cpp`, 1 in `GitsSectorGate.cpp`, 1 in `GitsTerminal.cpp`), several with
substituted numbers, e.g. "Noted. Run it, and we both find out.", "You watched it. Now tell me
again what that line does.", "That note draws %d and the bus is holding %d.", "The panel reads
%s. The system holds until the reading is yours.", "Sealed. Every system in this sector has to
run before the station lets you through." (gate default; each level overrides it).

### 7.2 How predictions are delivered

- `UGitsVantSubsystem::TerminalUsed` speaks the intro on first use and records `level_start`.
- The terminal overlay (`SGitsTerminalEditor`, question mode) shows the pending prediction's
  prompt and its options, shuffled by `GitsRng::FMulberry32` seeded with
  `HashToken(sessionId + scriptName + predictionId)` (FNV-1a); `prediction_shown` records the
  seed and order. Keyboard/gamepad select; `GitsPredict n` on the console.
- Commit is free (`prediction_committed`, log only). The run is refused while a reading is
  pending ("Noted. Run it, and we both find out." is spoken on commit).
- Correctness is revealed when the anchored statement (`AnchorLine`, `AnchorOccurrence` resolved
  against the trace's boundaries) executes during playback: `Settle` speaks the option's reveal
  for a wrong reading, locks it (`FGitsShift`), and records `prediction_submitted`. A right reading
  at the anchor speaks "That is what happened. You read it right."
- The gate (`FGitsShift::RecordHead`, ADR-030): released when, since the lock, the head has been
  at or before the first statement and then passed the anchor going forward; then "You watched
  it. Now tell me again what that line does." and `scrub_gate_satisfied`. A re-answer is settled
  against the existing trace with no run and no power.
- The caption HUD (`SGitsVantCaption`) shows the last line and fades; the same line is on the
  terminal.

### 7.3 Hint tier implementation

`FGitsHint {Tier 1..3, Text, CostsPower}` per script (validator: tiers 1–3, no duplicate tier,
text required). `RevealNextHint` sorts by tier, skips revealed tiers, refuses with "That note
draws N and the bus is holding M." when `CostsPower > Power` (recording `hint_requested
{affordable:false}`), otherwise charges the bus, marks the tier revealed and speaks the text
(`hint_requested {affordable:true}`). Tier 1 hints cost 0 in every script; tier 2 costs 3–6;
tier 3 (Make scripts and two others) costs 8–12. Console: `GitsHint`.

### 7.4 Voice

All text. No `USoundBase`, `UAudioComponent` or `PlaySound` reference exists in
`Source/GhostInTheStack` (grep count 0) and there is no `Content/Audio`. The phase text's "a
voice" for VANT is not implemented (Phase 11 in `PHASES-3D.md`).

---

## 8. Power

### 8.1 The model as implemented (`Station/GitsPower.h`, `GitsStationSubsystem`, `GitsTerminal`, `GitsVant`)

- One bus per map: `AGitsStation::PowerBudget` and `ReserveRestore` handed to the subsystem at
  `BeginPlay` (`InitialisePower`). Sector 1: 60/30; Sectors 2–4: 90/45.
- A run costs `RunCost`; with every prediction on the script committed and none locked, it costs
  `PredictedRunCost` (the discount, ADR-006). `AGitsTerminal::RunCostNow` computes it;
  `RunCurrent` refuses when `Cost > Power` or a reading is pending or the source is unchanged
  (ADR-020); on success `ChargePower(Cost)` then `NoteRun` records `run_executed
  {powerBefore, powerAfter, discounted, ...}`.
- Hints charge `CostsPower` (§7.3).
- The reserve: `AGitsGenerator::Draw` → `DrawReserve` restores the bus to `ReserveRestore` only
  when the bus is below it (otherwise VANT: "The bus is holding N. The reserve cell would not add
  to that."), records `reserve_drawn {before, after, draws}`.
- Stage: `GitsPower::StageFor(Power, Budget)`: `Out` at 0; `Critical` at ≤ 25 %; `Low` at ≤ 50 %;
  `Nominal` above 50 %; a budget of 0 is Nominal.

### 8.2 Invariants the validator enforces (`GitsTags::ValidatePower`, `AGitsStation::BeginPlay`, test `Station.Power.Validate`)

1. `RunCost > 0` ("a run must cost something").
2. `PredictedRunCost < RunCost` ("the discount must be real").
3. `Budget >= RunCost` ("the sector budget cannot cover a full-price run").

`GitsTags::Validate` (test `Companion.Tags.Validate`) additionally checks every prediction:
non-empty id and prompt, unique ids, anchor line inside the script, occurrence ≥ 1, 2–4
options, unique option ids, the correct option carries no misconception tag, every distractor
carries a known misconception tag, the correct id names an option; every concept tag is in the
list; a Make script has tests, no predictions and free edit; test cases have a label and expect
something; hints have tiers 1–3, unique, with text; `GoalUnlocks` is `key=value`.

### 8.3 Visual states and where each is triggered

| State | Rule | Code |
|---|---|---|
| Corridor rig brightness | `intensity = FullIntensity × level/10 × LightFactor(stage)`; factors 1.0 / 0.45 / 0.15 / 0 for nominal / low / critical / out | `AGitsLight::ApplyLevel`, subscribed to `OnPowerChanged` |
| Emergency lighting | `bEmergencyOnly` lights use factor 1 at `Out`, 0 otherwise (amber, 1.5 cd, four to five per map) | `AGitsLight::ApplyLevel` |
| Torch | spot light on the camera, `TorchIntensity = 6 cd` at `Out`, else 0 | `AGhostInTheStackCharacter::UpdateTorch` on `OnPowerChanged` |
| Gauge | text readout of holding/budget/stage | `AGitsPowerGauge` on `OnPowerChanged` |
| Generator | interactable; `Draw()` as above | `AGitsGenerator::Interact_Implementation` |
| Refusal | terminal status line and VANT line when a run cannot be paid for | `AGitsTerminal::RunCurrent` |

Test `Station.Power.Stages` covers `StageFor`/`LightFactor`.

---

## 9. Telemetry and instruments

Source: `Source/GhostInTheStack/Telemetry/` (`GitsTelemetry`, `GitsInstruments`, `Tests/`).
Reference: `src/telemetry/`, `src/instruments/`, `scripts/analyse.ts` in the cycle 1 repo.

### 9.1 Event taxonomy as implemented

`UGitsTelemetrySubsystem::EventTypes()` lists exactly the reference's 18 types. "Phase added"
is the cycle 1 phase per the reference `types.ts` comments (core set Phase 3 per ADR-013;
`reserve_drawn` Phase 5; the three instrument events Phase 8); in cycle 2 all 18 were implemented
in Phase 8. Payload keys are those written in source.

| Type | Payload keys (cycle 2) | Emitted by | Cycle 1 phase |
|---|---|---|---|
| `session_start` | `consentVersion, codeCapture, seed, experience` | `RecordConsent` | 3 |
| `level_start` | `levelId, act, tier` | VANT `TerminalUsed` (first use after consent) | 3 |
| `prediction_shown` | `predictionId, seed, order[]` | VANT `Show` | 3 |
| `prediction_submitted` | `predictionId, optionId, correct, attempt, misconception\|null, settledBy` | VANT `Settle` | 3 |
| `prediction_unresolvable` | `predictionId, line, occurrence, occurrencesFound` | VANT (anchor not in trace) | 3 |
| `scrub_gate_satisfied` | `predictionId, stepsScrubbed, durationMs` | VANT `HandleStep` | 3 |
| `run_executed` | `powerBefore, powerAfter, discounted, traceLength, terminatedNormally, capHit\|null` | VANT `NoteRun` | 3 |
| `scrub` | `from, to, method:"key"` | station `EndRewind` | 3 |
| `edit` | `line, before, after, hashed` (text only with code-capture consent, ADR-014) | telemetry `Edit` from terminal `SetLine` | 3 |
| `error_shown` | `code, line` | VANT `NoteError` (parse and runtime) | 3 |
| `hint_requested` | `tier, costsPower, affordable` | VANT `RevealNextHint` | 3 |
| `reserve_drawn` | `before, after, draws` | VANT `NoteReserveDrawn` | 5 |
| `level_complete` | `levelId` | VANT `TryComplete` | 3 |
| `level_abandoned` | (type accepted; never emitted by the build) | — | 3 |
| `instrument_started` | `instrumentId, occasion, items` | `SGitsInstrumentPanel` | 8 |
| `instrument_response` | `instrumentId, occasion, itemId, value\|null, skipped, latencyMs` | `SGitsInstrumentPanel` | 8 |
| `instrument_completed` | `instrumentId, occasion, answered, total, score{...}` | `SGitsInstrumentPanel` | 8 |
| `session_end` | `durationMs` | post panel | 3 |

Log-only events not in the taxonomy (never stored): `prediction_committed`, `goal_missed`,
`sector_complete`. Every event carries the envelope `{sessionId, participantCode, levelId|null,
timestamp (ms), sequence, type, payload}`.

### 9.2 Consent gate

`Record(Type, Payload)` returns false and logs `LogGitsTelemetry: Error ... refused to record`
unless `RecordConsent` has run in this game instance and the type is one of the 18. VANT's
`Emit` logs every event as a `LogGitsTelemetry` line regardless and calls `Record` only under
consent. The consent record is `{version: "2026-08-1", codeCapture, recordedAt}`; the
participant code is `word-word-NN` from the reference's 22-word list, generated from a clock seed
at start-up and shown on the consent screen. `Erase()` forgets the session and deletes the store.

Test proving no event precedes consent: `GhostInTheStack.Telemetry.ConsentGate`
(`Telemetry/Tests/GitsTelemetryTests.cpp`): a fresh subsystem refuses `Record` for every one of
the 18 types, the store stays empty, an `Edit` before consent is hashed and refused, then consent
is recorded and the same calls succeed; the bundle parses with `formatVersion 1`; `Erase` empties
it. Also `Telemetry.PiiTripwire`: `FindPii` refuses export when the serialised bundle matches any
of six patterns (email address, IPv4 address, a `name`/`fullName`/`firstName`/`lastName`/`username`
field, an `email` field, an `ip`/`ipAddress`/`remoteAddr` field, a `userAgent` field).

### 9.3 Export format

`Export(OutPath, OutError)` writes `Saved/Telemetry/exports/gits-<code>-<yyyy-mm-dd>.json`:

```
{ "formatVersion": 1, "participantCode": "...", "exportedAt": <ms>,
  "consent": { "version": "2026-08-1", "codeCapture": bool, "recordedAt": <ms> },
  "events": [ { "sessionId", "participantCode", "levelId"|null, "timestamp", "sequence", "type", "payload" }, ... ] }
```

The running store is `Saved/Telemetry/<code>.events.jsonl` (appended per event) and
`<code>.consent.json`; cycle 1 used IndexedDB (ADR-013), a storage divergence with the same
bundle shape.

### 9.4 Committed sample bundle

`docs/screenshots/phase-8/sample-bundle.json`: participant `slate-slate-82`, `formatVersion` 1,
`codeCapture: true`, 158 events, 16 of the 18 types present (`prediction_unresolvable` and
`level_abandoned` never fire in a clean run), level ids `a1-l01`–`a1-l08` plus `null` for
instrument events. Counts: `instrument_response` 87, `prediction_shown` 15,
`prediction_submitted` 12, `level_start` 8, `run_executed` 8, `level_complete` 8,
`instrument_started` 5, `instrument_completed` 5, `edit` 3, and 1 each of `session_start`,
`scrub`, `scrub_gate_satisfied`, `hint_requested`, `reserve_drawn`, `error_shown`, `session_end`.

### 9.5 Read by the reference `analyse.ts`

Run on 11 Sep 2026 against a directory containing only the committed sample:

```
cd "C:\Users\M9Baz\OneDrive\Desktop\ghost in the stack"
npm run analyse -- --in <dir with sample-bundle.json>
```

Output (excerpt; full text kept in the session's scratchpad `evidence/bundle/analyse-output.txt`):
`Read 1 bundle(s)`, `1 participant(s), 158 events, 30 levels.`, first-attempt accuracy by level
for a1-l01..a1-l08, the misconception-decay table, `PRE/POST TRACING TEST n=1 pre 2 → post 3`,
MEEGA+/SUS/IMI dimension tables, `THE SCRUB GATE 1 wrong first attempts, 1 gates satisfied,
median 4 steps, 11192 ms at the anchor, 1/1 re-answers correct`, `EFFORT 8/8 levels completed,
8 runs, 1 hints (0.125 per level), 1 reserve draws, 3 edits, 0 of them hashed`, `CODE CAPTURE: 1
consented, 0 declined`, `Wrote 7 file(s) to analysis/`. No error, no "does not look like an export
bundle".

### 9.6 The four instruments (`GitsInstruments.cpp`)

| Instrument | Id(s) | Items | Scale | Scoring | Provenance string in source (verbatim excerpt) | `wordingVerified` |
|---|---|---:|---|---|---|---|
| Tracing test | `tracing-A`, `tracing-B` | 11 per form (11 matched pairs, 22 items) | multiple choice, 2–4 options, every distractor tagged | correct/answered/total, by concept | "Authored for this study: matched pairs over the constructs the four tiers teach, every distractor tagged with a misconception, every item checked against the interpreter." | n/a (authored); test `Telemetry.Instruments.TracingItems` runs every item's code through the interpreter and checks the marked answer |
| SUS | `sus` | 10 | 1–5 | 0–100, `null` if any item skipped | "Brooke, J. (1996) SUS ... \"this system\" rendered as \"this game\". Wording not yet verified against the published source." | reference: `false`; port carries the text, no boolean field |
| IMI (short form) | `imi` | 22 | 1–7 | four subscale means, no overall | "McAuley, Duncan and Tammen (1989) ... Wording not yet verified against the published source." | reference: `false` |
| MEEGA+ | `meega-plus` | 33 | −2..2 | 13 dimension mean/median, overall = median item score | "Petri, von Wangenheim and Borgatto (2016), MEEGA+. Wording not yet verified against the published source; no alpha from the paper may be claimed." | reference: `false` |

Form assignment: `FormFor(code, occasion)` by FNV parity of the participant code, swapped between
pre and post; option order per item by `hash(code:itemId)`. Test `Telemetry.Instruments.Scoring`
checks SUS 100/50/null, IMI subscale means with reversed items, MEEGA+ 33 items/13 dimensions.
Skipped items are recorded (`skipped: true`) and excluded from scoring; unreached items are not
recorded.

---

## 10. Verification

### 10.1 Automation tests

57 tests, 0 failures, via `Tools\run_interpreter_tests.cmd` (§3.7). By area: Interpreter 47
(30 golden, 1 golden-coverage, 6 evaluator, 5 parser, 1 each lexer/values/diff/rng/diagnostics),
Companion 3, Recorder 1, Station 2, Telemetry 4. Full list §16.4. The Phase 1 count was 45
(`docs/screenshots/phase-1/mcp-test-results.json`).

### 10.2 Coverage

UNMEASURED. No line-coverage tooling is configured for the Unreal automation tests, and none was
run. Content coverage (tags per system) is measured (§4.3, §4.4).

### 10.3 Verified only by screenshot or PIE automation log, not by test

- Door, light, vent, clock, heater, sprinkler, conveyor animation and the gate's level load.
- Rewind visuals (line highlight, panel bindings, loop line) beyond what `Recorder.Loops` and
  `GitsVerifyRewind` check numerically.
- Power dimming stages, emergency lighting, torch, gauge, generator (the stage function and
  validator are tested; the rendering is not).
- Every VANT line, the terminal overlay, the instrument panel and the consent screen.
- Sector completion and unlocks for all four sectors (PIE console automation:
  `pie_accept7.sh`, `pie_accept8.sh`, `pie_accept9.sh` in the session scratchpad, not in the repo).
- Kit appearance and collision placement in the maps.

### 10.4 Human play

No human has played any sector in cycle 2. No playtest was recorded. The Phase 7 acceptance
(a non-programmer, under 45 minutes, a laugh, the airlock) is open. All "completable" claims
rest on console-driven PIE automation that teleports, answers with the marked option, inserts
the Make solutions programmatically, and reads the log.

---

## 11. Performance

Method: `GitsFrameSample <s>` samples viewport frame times over a window (skipping the frame that
ran the console command); `GitsStatus` prints `minFps avgFps frames worstFrame(ms) maxSeekMs`
for the current play state; `GitsVerifyRewind` seeks every boundary and compares the terminal's
line with the trace. Play In Editor, "Selected Viewport", editor vsync-capped at 60 fps. Machine:
§1.6. Measured 11 Sep 2026 at HEAD.

| Sector (map) | Run measured (first terminal) | Playback: steps / statements | Playback fps min / avg (frames sampled, worst frame ms) | Rewind fps min / avg (frames, worst ms) | Slowest `SeekTo` during playback / rewind (ms) | `GitsVerifyRewind` |
|---|---|---:|---|---|---|---|
| 1 `L_Sector1_Airlock` | DA_S1_DoorLog | 5 / 2 | 47.0 / 59.8 (69, 22) | 60.0 / 60.0 (60, 57) | 2.15 / 3.34 | 2 boundaries backwards, 0 mismatches, slowest step change 2.24 ms |
| 2 `L_Sector2_Greenhouse` | DA_S2_Interlock | 11 / 4 | 60.0 / 60.0 (99, 90) | 59.9 / 60.0 (117, 113) | 3.49 / 3.49 | 4 boundaries, 0 mismatches, 3.42 ms |
| 3 `L_Sector3_Logistics` | DA_S3_PurgeCycle | 42 / 12 | 55.6 / 60.0 (304, 299) | 59.9 / (avg not reported: 0.0 printed) (115, 71) | 3.51 / 4.11 | 12 boundaries, 0 mismatches, 3.58 ms |
| 4 `L_Sector4_Reactor` | DA_S4_TwoRooms | 25 / 8 | 47.0 / 59.9 (208, 197) | 59.9 / (avg not reported: 0.0 printed) (124, 101) | 3.91 / 4.16 | 8 boundaries, 0 mismatches, 5.27 ms |

Notes on the table. `minFps` is the worst single frame in the window and includes the frame that
opened or closed the terminal overlay (47.0 fps = one 21 ms frame); the average is at the 60 fps
cap in every sector. "worst frame" is the index of that frame within the sample. The rewind
average printed 0.0 in Sectors 3 and 4 (the sampler had reset on the state change before the
status line was read); the minimum and the seek times were reported. Idle-walk frame rate was
not captured by this run (the status line prints playback samples only): UNMEASURED at HEAD;
Phase 6's walkthrough figure (59.6–59.9 avg) is the last recorded value. The largest seek
measured, 5.27 ms, is against the 16 ms ADR-022 target; the traces here are short (5–42
steps); the 224-step corridor-lights loop measured 5.2 ms in Phase 3.

Historical figures from the status table (same machine, earlier phases): Phase 2 door animation
60 fps (min 59.5); Phase 3 slowest step change 5.2 ms (target 16); Phase 6 walkthrough averages
59.6–59.9 over four windows.

| Item | Value |
|---|---|
| Editor warm start to first Python answer | 93 s (log open 16:27:38 → `Engine is initialized` 16:28:03 (25 s) → first `PING world` 16:29:11), third launch of the day |
| Editor cold start | UNMEASURED: observed at roughly eight minutes after a reboot on the external disk, but not timed from a log |
| Packaged build | UNMEASURED: no packaged build exists (`Saved/StagedBuilds` absent); package size and cold load to playable cannot be measured |
| Interpreter parse-and-trace time (ADR-022 targets) | UNMEASURED in cycle 2: no timing test; the 30 golden tests ran in 1.20 s total including editor overhead (Phase 1 MCP result) |

---

## 12. Blender pipeline

| Item | Value |
|---|---|
| Kit file | `Content/Kit/Kit.blend` (plus `Kit.blend1` backup), scene units centimetres (unit scale 0.01) |
| Piece count in `Kit.blend` | 28 objects `SM_Kit_*` in the `Kit` collection (27 game pieces + `SM_Kit_TestCube`), 152 `UCX_*` collision objects (counted with `blender --background --python-expr`) |
| Pieces | CeilingLight, ColdStoreUnit, Conveyor, CoolantPipe, CorridorCorner, CorridorDoorway, CorridorSegment, Crate, DoorFrame, DoorPanel, Drone, Generator, GrowLight, Heater, Mast, Planter, ReactorCore, Shelf, SledgeRack, Sprinkler, SprinklerSpray, Terminal, TestCube, Vent, VentFan, WallCap, WallDisplay, WallGauge |
| Builder | `Tools/build_kit.py`: every piece from axis-aligned boxes with per-face UVs onto one of eight trim bands (bone, slate, copper, ink, hazard, grating, ceiling, signage), a bevel modifier, one UCX box per part, glow parts on `M_Kit_Glow`; described in Unreal coordinates with Y mirrored |
| Trim sheet | `Content/Kit/T_Kit_Trim.png` painted by `Tools/make_trim_sheet.py` (Python PIL), not in Blender |
| Export script | `Tools/export_kit.py`: one `.glb` per piece (metres; copies scaled by 0.01), `SM_Kit_<Name>.collision.json` sidecar with UCX bounds in Unreal space, refuses if the unit scale is not 0.01 |
| Import script | `Tools/import_kit.py`: Interchange glTF import into `/Game/Kit`, consolidates duplicate materials/textures, binds slot 0 → `M_Kit_Trim`, slot 1 → `M_Kit_Glow`, applies sidecar boxes |
| Naming | `SM_Kit_<Name>` mesh, `UCX_SM_Kit_<Name>[_NN]` collision, origin at the base on the floor |
| Scale | 1 Blender unit = 1 cm = 1 Unreal unit; corridor segment 400 × 300 × 300 |
| Bypassed the pipeline | the trim texture (PIL); Interchange's leftover texture assets (14 `T_*` under `Content/Kit`, only `T_Kit_Trim` bound); no hand-modelled piece exists |
| Blender MCP | listed in CLAUDE.md; the pipeline ran headless (`blender --background ... --python`) rather than through the MCP socket |

---

## 13. Decisions

`docs/DECISIONS.md`, 30 entries. Cycle: ADR-001–025 cycle 1; ADR-026 is the cycle boundary
(written at the end of cycle 1, opens cycle 2); ADR-027–030 cycle 2 (decided by Claude at the
developer's delegation, 11 Sep 2026). "Chapter" is a suggested placement, not a fact in the
repo: D = design of the artefact, I = implementation, E = evaluation/method, P = project
management.

| ADR | Title | One line | Cycle | Ch. | Status |
|---|---|---|---|---|---|
| 001 | Step granularity is per node, with statement boundaries marked | one step per AST node; `depth`, `stmtIndex`, `isStatementBoundary` | 1 | D/I | in force (cycle 2 `FGitsStep`) |
| 002 | NodeId is a structural path; diff compares observable state | diff aligns by statement boundaries; bindings, output, effects | 1 | I | in force |
| 003 | The evaluator takes a WorldOracle and records every read | injected pure oracle; reads recorded in the step | 1 | I | in force |
| 004 | WorldState is an open record with a per-level declared schema | open key/value world; per-level schema validated | 1 | I | partly: cycle 2 has no per-level schema, keys are initial switches/levels on `AGitsStation` |
| 005 | Predictions anchor to a source location, not a step index | `{line, occurrence}`; unresolvable anchors skipped | 1 | D | in force |
| 006 | Prediction is a discount on execution, not a refund after it | free commit, discounted run, wrong answer locks until scrubbed | 1 | D | in force; refined by 030 |
| 007 | Language spec corrections | `+=` at tier 3, `list.append`, truthiness by tier, recognition pass | 1 | I | in force |
| 008 | Two caps, and a three-level scrubber LOD | 50,000 safety / 2,000 pedagogical (steps); ribbon LOD | 1 | D/I | cap unit **superseded by 021**; ribbon LOD superseded by 026 (no ribbon; loop contexts on the wall display) |
| 009 | Trace access goes through an accessor, always | `At`, `StatementBoundaries` | 1 | I | in force (one raw read in the rooms view, §3.3) |
| 010 | A step can produce multiple outputs and multiple effects | arrays | 1 | I | in force |
| 011 | Prediction options are shuffled deterministically | mulberry32 seeded from session+prediction id | 1 | D/E | in force (`Rng.Parity` test) |
| 012 | Make is a per-act mechanic, and it gets a goal kind | `tests` goal; one Make per act | 1 | D | in force (one Make per sector) |
| 013 | Minimal telemetry lands in Phase 3, not Phase 8 | core log early; instruments later | 1 | E/P | cycle 2 did both in Phase 8; store is JSON-lines not IndexedDB |
| 014 | Edit events capture the code, with consent | text or hashes | 1 | E | in force |
| 015 | Golden traces are compact text snapshots | one line per step | 1 | I | in force (the 30 fixtures) |
| 016 | Two renderers, one contract | DiagramView (pedagogical) + IsometricView (presentation) | 1 | D | **superseded by 026**: both replaced by the 3D world; the contract `(trace, stepIndex)` survives as `PoseFromWorld` |
| 017 | The prediction gate is our answer to Sorva | commitment is the active ingredient | 1 | D/E | in force |
| 018 | instruments module contract | pure descriptors, one generic renderer | 1 | E | in force (`FGitsInstrument`, `SGitsInstrumentPanel`) |
| 019 | Ethics scope must be confirmed before Phase 3 | open action | 1 | E | **open**; not resolved in either repo |
| 020 | Running unchanged source is not a thing | refusal; fresh commitments on changed statements; Make exempt | 1 | D | in force |
| 021 | The pedagogical cap counts statement executions, not steps | 2,000 statement executions | 1 | I | in force; supersedes 008's unit |
| 022 | Performance targets split by interaction frequency | 50 ms / 400 ms / 16 ms | 1 | E | 16 ms scrub target carried; trace targets UNMEASURED in cycle 2 |
| 023 | Building is complete; the remaining time is for the report | cycle 1 freeze at 2575935 | 1 | P | historical |
| 024 | A browser harness is approved for measurement | playwright/axe-core devDependencies | 1 | E | cycle 1 only |
| 025 | The evaluation study runs on Sector 1 | study scope; post-test after eight systems | 1 | E | in force (post terminal beyond Sector 1's airlock) |
| 026 | Second design cycle: a first-person 3D game in Unreal Engine | rebuild presentation; TS repo is the reference | boundary | D/P | in force |
| 027 | Diegetic screens are Slate widgets driven from C++, not Blueprint UMG | | 2 | I | in force |
| 028 | The world carries over between runs | initial values fill only unset keys | 2 | D/I | in force |
| 029 | Doors and lights are station builtins | `open_door`, `close_door`, `set_light` | 2 | I | in force |
| 030 | The scrub gate in a time-based player asks for the start, then the anchor | reveal at the anchored statement during playback | 2 | D | in force; refines 006 |

Not recorded as ADRs but decided in cycle 2 (in `docs/STATION-LAYER.md` "Divergences" and the
phase rows): one power bus per sector instead of per level; sector gates and a game-instance
progress record between maps; collision via sidecar JSON; VANT as a subsystem not an actor.

---

## 14. Known issues

### Open acceptance and process (5)
1. No human playtest of any sector (Phase 7 criterion); no recording. A human must do this.
2. No GitHub remote / LFS (Phase 0 criterion).
3. ADR-019 ethics scope unresolved (a human decision with the supervisor and committee).
4. Instrument wording not verified against the published SUS, IMI, MEEGA+ sources (a human check).
5. No packaged build; nothing has run outside the editor.

### Divergences from spec or design still standing (6)
6. Tag-specific physical failures from `3D-REDESIGN.md` not implemented; all misconceptions share the generic choreography (§5).
7. VANT has no voice and is not an actor (Phase 4 text; Phase 11 scope).
8. Sector 3's Make is the depot tally, not a drone route; no drone behaviours; the drone is dressing.
9. Screens are Slate, not UMG (ADR-027 records it; `3D-REDESIGN.md` §4 still says UMG).
10. The recorder's rooms view reads `Steps[Head].Frames` directly, outside the accessor (ADR-009).
11. `LANGUAGE-SPEC.md` still lists valves/heaters/pumps first; `open_door`/`close_door`/`set_light` are appended (ADR-029) and `set_heater` writes `heater.<id>` used only via unlocks.

### Measurement gaps (4)
12. Cold editor start, packaged cold load, package size: UNMEASURED.
13. ADR-022 parse-and-trace budgets: no timing test in cycle 2.
14. Test coverage: no tooling.
15. Frame rate figures are PIE, vsync-capped at 60, on one machine.

### Engine and tooling incidents recorded on disk (3 `Saved/Crashes`, all `CrashType Ensure`, none fatal)
16. 10 Sep 16:33 and 19:24: `Ensure condition failed: !ChaosConvex` (BodySetup) during kit import; resolved by the sidecar box collision.
17. 11 Sep 12:26: `Ensure condition failed: false` in `UObjectGlobals` from the first version of the `ConsentGate` test (subsystem created without a game-instance outer); fixed the same day.
18. The Unreal Build Accelerator stalled on the external disk; all builds after 11 Sep use `-NoUBA` (memory note, `Tools/README.md`).

### Content and behaviour notes (5)
19. `level_abandoned` is accepted by the gate but never emitted; `prediction_unresolvable` never fires in authored content.
20. A `level_start` for a terminal used before consent is recorded at the first use after consent (by design); the first automated Phase 8 bundle showed 7/8 starts before this fix.
21. Long traces play back in real time (0.4 s per statement): the depot tally's 210-step run takes about 26 s to reach its goal; automation waits 32 s.
22. Interchange import leaves duplicate `T_*` texture assets under `Content/Kit` (14 present, 1 used).
23. Console `Exec` string arguments strip leading spaces, so indented lines cannot be inserted with `GitsInsertLine`; the PIE helper inserts through the terminal's Python-callable `InsertLineAfter`/`SetLine` instead.

### Cycle 1 issues carried by reference
The reference repo's `docs/EVIDENCE.md` and its KI list (KI-23, 36, 39, 42, 43, 44, 45, 53, 60, 61, 62 cited in ADR-023/024) are the cycle 1 record; none of those numbered issues is tracked in this repo.

---

## 15. Figures

| Figure | Data or path |
|---|---|
| System architecture | Modules and classes: §1.2 table and §4.1 table. Data flow: `UGitsScript` → `AGitsTerminal::RunCurrent` → `UGitsStationSubsystem::Run` (parser → evaluator with `AGitsStation::Read` as oracle → `FGitsTrace`) → `FGitsRecorder::Build` → tick: `StepAtTime` → `SeekTo` → `WorldAt` + overrides → `PoseFromWorld` on every `AGitsSystemActor`; `UGitsVantSubsystem` observes `OnStepChanged` for anchors and goals; `UGitsTelemetrySubsystem` (game instance) receives `Record` from VANT, station, terminal, panel; `UGitsProgressSubsystem` (game instance) across maps. Source of truth: `docs/STATION-LAYER.md` "Layout" and "The pipeline, one run". |
| Trace-to-world data flow | §3.1 (`FGitsStep`), §3.2 (oracle), §6.2 (pose). One step's `Effects` are folded by `GitsWorld::Reduce` into the world at that index; a door reads `door.<id>` from it. |
| PRIMM-to-mechanics mapping as implemented | Predict = terminal question mode, free commit, seeded shuffle (§7.2). Run = `RunCurrent`, power discount (§8.1), playback at 2.5 statements/s. Investigate = hold-R rewind, wall-display bindings/loops/rooms (§6), the scrub gate (ADR-030). Modify = editable lines per script (§4.2 "Edit"), ADR-020 refusal, `edit` events. Make = four free-edit scripts with `tests` goals (§4.2). |
| Sector map with concept progression | Maps: `L_Sector1_Airlock` (Tier 1: variables, types, sequence, output; 8 + 1 optional), `L_Sector2_Greenhouse` (Tier 2: conditionals, comparison, boolean logic, truthiness; 8), `L_Sector3_Logistics` (Tier 3: loops, lists, indexing; 8), `L_Sector4_Reactor` (Tier 4: functions, scope, recursion; 6). Layout coordinates and terminal positions are in `Tools/setup_sector{1..4}_content.py`; gates at (1350, 995) in Sector 1 and (1750, 995) in Sectors 2–3; Sector 4 ends at the west door. Per-system tags: §4.2. |
| Misconception choreography table | §5. |
| Coverage charts | §4.3 and §4.4 (data); raw output `python Tools/check_curriculum.py`. |
| Telemetry taxonomy | §9.1. Analysis figures produced by the reference: `analysis/fig-prediction-accuracy.svg`, `analysis/fig-misconception-decay.svg` in the reference repo after §9.5's run. |
| Phase timeline | §2 commit dates: 10 Sep (Phases 0–2), 11 Sep (Phases 3–9). |
| Screenshots | 105 files under `docs/screenshots/phase-{0..9}/`, captioned in §2. |

---

## 16. Appendix material

### 16.1 Error catalogue

Index of the 46 codes with the catalogue's own one-line "fires when" summary
(`GitsDiagnostics.cpp`, generated by regex over the `Set(...)` table). The player-facing "what"
and "check" strings follow verbatim in 16.1.2.

| # | code | enum | fires when (catalogue summary, verbatim) |
|---|---|---|---|
| 1 | `tab-indentation` | `TabIndentation` | A tab character appears in a line's leading whitespace. Recognition case 18. |
| 2 | `inconsistent-indentation` | `InconsistentIndentation` | A line's indentation matches no open block. |
| 3 | `unexpected-indent` | `UnexpectedIndent` | A line is indented but nothing above it opened a block. |
| 4 | `unterminated-string` | `UnterminatedString` | A piece of text reaches the end of the line without a closing quote. |
| 5 | `unexpected-character` | `UnexpectedCharacter` | A character that means nothing in this subset. |
| 6 | `malformed-number` | `MalformedNumber` | Digits run directly into letters. |
| 7 | `missing-colon` | `MissingColon` | A block-opening statement has no colon. |
| 8 | `expected-indented-block` | `ExpectedIndentedBlock` | A colon opened a block and nothing was indented under it. |
| 9 | `unclosed-bracket` | `UnclosedBracket` | A bracket is opened and never closed. |
| 10 | `unexpected-token` | `UnexpectedToken` | The generic fallback. Something appears where nothing valid could. |
| 11 | `invalid-assignment-target` | `InvalidAssignmentTarget` | The left-hand side of = is not a name. |
| 12 | `assignment-in-condition` | `AssignmentInCondition` | A single = used where == was meant. A high-value novice error. |
| 13 | `leading-plus` | `LeadingPlus` | A unary plus, which this subset does not have. |
| 14 | `elif-without-if` | `ElifWithoutIf` | elif or else with no matching if. |
| 15 | `return-outside-function` | `ReturnOutsideFunction` | return where there is no function to return from. |
| 16 | `excluded-dict-literal` | `ExcludedDictLiteral` | Recognition case 1. Braces containing a colon. |
| 17 | `excluded-set-literal` | `ExcludedSetLiteral` | Recognition case 2. Braces without a colon. |
| 18 | `excluded-fstring` | `ExcludedFString` | Recognition case 3. An f or F immediately before a quote. |
| 19 | `excluded-import` | `ExcludedImport` | Recognition case 4. Leading import or from. |
| 20 | `excluded-class` | `ExcludedClass` | Recognition case 5. Leading class. |
| 21 | `excluded-exception-handling` | `ExcludedExceptionHandling` | Recognition case 6. Leading try, except, finally or raise. |
| 22 | `excluded-with` | `ExcludedWith` | Recognition case 7. Leading with. |
| 23 | `excluded-lambda` | `ExcludedLambda` | Recognition case 8. lambda in expression position. |
| 24 | `excluded-comprehension` | `ExcludedComprehension` | Recognition case 9. A for inside a bracket. |
| 25 | `excluded-tuple` | `ExcludedTuple` | Recognition case 10. A comma-separated value list. |
| 26 | `excluded-slicing` | `ExcludedSlicing` | Recognition case 11. A colon inside a subscript. |
| 27 | `excluded-scope-declaration` | `ExcludedScopeDeclaration` | Recognition case 12. Leading global or nonlocal. |
| 28 | `excluded-keyword-argument` | `ExcludedKeywordArgument` | Recognition case 13. name= inside an argument list. |
| 29 | `excluded-default-argument` | `ExcludedDefaultArgument` | Recognition case 14. = inside a parameter list. |
| 30 | `excluded-comparison-chaining` | `ExcludedComparisonChaining` | Recognition case 15. Two comparisons at one level. Refused, never mis-evaluated. |
| 31 | `excluded-multiple-assignment` | `ExcludedMultipleAssignment` | Chained assignment, a = b = 1. |
| 32 | `excluded-construct` | `ExcludedConstruct` | A construct on the excluded list with no enumerated case of its own. |
| 33 | `not-yet-unlocked` | `NotYetUnlocked` | Recognition case 16. Correct Python from a tier above the level's. |
| 34 | `method-not-available` | `MethodNotAvailable` | Recognition case 17. A method call outside the tier whitelist. |
| 35 | `name-not-defined` | `NameNotDefined` | A name is read before anything was stored under it. |
| 36 | `type-mismatch` | `TypeMismatch` | An operator applied to kinds of value it does not join. |
| 37 | `division-by-zero` | `DivisionByZero` | Division or remainder with a right-hand side of zero. |
| 38 | `index-out-of-range` | `IndexOutOfRange` | A list index outside the list. |
| 39 | `bad-index-type` | `BadIndexType` | A list indexed with something other than a whole number. |
| 40 | `not-a-list` | `NotAList` | Indexing or appending to something that is not a list. |
| 41 | `wrong-argument-count` | `WrongArgumentCount` | A call with the wrong number of arguments. |
| 42 | `unknown-function` | `UnknownFunction` | A call to a name that is not a function. |
| 43 | `bad-range` | `BadRange` | range() called with a step of zero, or with non-integer arguments. |
| 44 | `statement-cap-exceeded` | `StatementCapExceeded` | The per-level design budget (ADR-021). A diagnosable outcome, not a crash. |
| 45 | `safety-cap-exceeded` | `SafetyCapExceeded` | The project-wide runaway guard (ADR-008). An error path. |
| 46 | `call-depth-exceeded` | `CallDepthExceeded` | Recursion deeper than MAX_CALL_DEPTH. |

#### 16.1.2 The catalogue table verbatim (`Source/GhostInTheStack/Interpreter/GitsDiagnostics.cpp`, lines 35–227)

```cpp
			// --- lexical
			Set(EGitsDiagnosticCode::TabIndentation, TEXT("tab-indentation"),
				TEXT("A tab character appears in a line's leading whitespace. Recognition case 18."),
				[](P) { return FString(TEXT("This line is indented with a tab.")); },
				[](P) { return FString(TEXT("Replace the tab with four spaces. Indentation here is always four spaces per level, and a tab is one character however wide it looks.")); });
			Set(EGitsDiagnosticCode::InconsistentIndentation, TEXT("inconsistent-indentation"),
				TEXT("A line's indentation matches no open block."),
				[](P p) { return FString::Printf(TEXT("This line is indented %s spaces, which does not line up with any block that is open here."), *SubjectOf(p)); },
				[](P p) { return FString::Printf(TEXT("Indent it %s spaces to sit inside the block above, or take it back to the left to close that block."), *DetailOf(p)); });
			Set(EGitsDiagnosticCode::UnexpectedIndent, TEXT("unexpected-indent"),
				TEXT("A line is indented but nothing above it opened a block."),
				[](P) { return FString(TEXT("This line is indented, but the line above it does not start a block.")); },
				[](P) { return FString(TEXT("Move it back to the left. Only a line ending in a colon opens a block, and only the lines inside that block are indented.")); });
			Set(EGitsDiagnosticCode::UnterminatedString, TEXT("unterminated-string"),
				TEXT("A piece of text reaches the end of the line without a closing quote."),
				[](P) { return FString(TEXT("This piece of text opens with a quote mark but never closes it.")); },
				[](P p) { return FString::Printf(TEXT("Add a matching %s at the end of the text. A piece of text has to start and finish on the same line."), *SubjectOf(p)); });
			Set(EGitsDiagnosticCode::UnexpectedCharacter, TEXT("unexpected-character"),
				TEXT("A character that means nothing in this subset."),
				[](P p) { return FString::Printf(TEXT("The character %s does not mean anything here."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Check for a typo. If you meant it as part of a piece of text, it needs to be inside quote marks.")); });
			Set(EGitsDiagnosticCode::MalformedNumber, TEXT("malformed-number"),
				TEXT("Digits run directly into letters."),
				[](P p) { return FString::Printf(TEXT("%s starts as a number and then runs into letters."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Put a space or an operator between the number and the name, or fix the spelling if it was meant to be one word.")); });

			// --- structural
			Set(EGitsDiagnosticCode::MissingColon, TEXT("missing-colon"),
				TEXT("A block-opening statement has no colon."),
				[](P p) { return FString::Printf(TEXT("This %s line needs a colon at the end."), *SubjectOf(p)); },
				[](P p) { return FString::Printf(TEXT("Add a colon after the condition, then indent the lines that belong to the %s by four spaces."), *SubjectOf(p)); });
			Set(EGitsDiagnosticCode::ExpectedIndentedBlock, TEXT("expected-indented-block"),
				TEXT("A colon opened a block and nothing was indented under it."),
				[](P p) { return FString::Printf(TEXT("The %s on this line opens a block, but no lines are indented under it."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Indent at least one line by four spaces underneath, so the machine knows what belongs inside.")); });
			Set(EGitsDiagnosticCode::UnclosedBracket, TEXT("unclosed-bracket"),
				TEXT("A bracket is opened and never closed."),
				[](P p) { return FString::Printf(TEXT("This %s is opened here and never closed."), *SubjectOf(p)); },
				[](P p) { return FString::Printf(TEXT("Add the matching %s. Count them from left to right: every one that opens has to close."), *DetailOf(p)); });
			Set(EGitsDiagnosticCode::UnexpectedToken, TEXT("unexpected-token"),
				TEXT("The generic fallback. Something appears where nothing valid could."),
				[](P p) { return FString::Printf(TEXT("%s cannot appear here."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Read the line from the left and check each part is something you can name. A missing operator or a stray bracket is the usual cause.")); });
			Set(EGitsDiagnosticCode::InvalidAssignmentTarget, TEXT("invalid-assignment-target"),
				TEXT("The left-hand side of = is not a name."),
				[](P) { return FString(TEXT("The left of the equals sign has to be a name, and this is not one.")); },
				[](P) { return FString(TEXT("Assignment stores a value under a name, so it reads name = value. Check which side you meant to be which.")); });
			Set(EGitsDiagnosticCode::AssignmentInCondition, TEXT("assignment-in-condition"),
				TEXT("A single = used where == was meant. A high-value novice error."),
				[](P) { return FString(TEXT("This condition uses one equals sign, which stores a value rather than comparing two.")); },
				[](P) { return FString(TEXT("Use two equals signs to ask whether the values match. One equals sign means \"put this value under this name\".")); });
			Set(EGitsDiagnosticCode::LeadingPlus, TEXT("leading-plus"),
				TEXT("A unary plus, which this subset does not have."),
				[](P) { return FString(TEXT("This plus sign has nothing on its left to add to.")); },
				[](P) { return FString(TEXT("Remove it. A plus needs a value on both sides, and a positive number does not need marking as positive.")); });
			Set(EGitsDiagnosticCode::ElifWithoutIf, TEXT("elif-without-if"),
				TEXT("elif or else with no matching if."),
				[](P p) { return FString::Printf(TEXT("This %s has no if above it to attach to."), *SubjectOf(p)); },
				[](P p) { return FString::Printf(TEXT("Every %s continues an if. Check the if is at the same indentation and that nothing between them broke the chain."), *SubjectOf(p)); });
			Set(EGitsDiagnosticCode::ReturnOutsideFunction, TEXT("return-outside-function"),
				TEXT("return where there is no function to return from."),
				[](P) { return FString(TEXT("This line hands a value back, but there is nothing here to hand it back to.")); },
				[](P) { return FString(TEXT("return only makes sense inside a function. If you meant to show the value, use print().")); });

			// --- recognition: not here
			Set(EGitsDiagnosticCode::ExcludedDictLiteral, TEXT("excluded-dict-literal"),
				TEXT("Recognition case 1. Braces containing a colon."),
				[](P) { return NotHere(TEXT("A dictionary")); },
				[](P) { return FString(TEXT("Store the values under separate names, or in a list once lists are available to you.")); });
			Set(EGitsDiagnosticCode::ExcludedSetLiteral, TEXT("excluded-set-literal"),
				TEXT("Recognition case 2. Braces without a colon."),
				[](P) { return NotHere(TEXT("A set")); },
				[](P) { return FString(TEXT("Use a list instead once lists are available to you. Nothing in this facility needs a set.")); });
			Set(EGitsDiagnosticCode::ExcludedFString, TEXT("excluded-fstring"),
				TEXT("Recognition case 3. An f or F immediately before a quote."),
				[](P) { return NotHere(TEXT("An f-string")); },
				[](P) { return FString(TEXT("Join the pieces with + instead, converting numbers with str() first: \"depth \" + str(depth).")); });
			Set(EGitsDiagnosticCode::ExcludedImport, TEXT("excluded-import"),
				TEXT("Recognition case 4. Leading import or from."),
				[](P) { return NotHere(TEXT("Importing a module")); },
				[](P) { return FString(TEXT("Everything this shift needs is already here: the builtins and the station functions in the manual.")); });
			Set(EGitsDiagnosticCode::ExcludedClass, TEXT("excluded-class"),
				TEXT("Recognition case 5. Leading class."),
				[](P) { return NotHere(TEXT("Defining a class")); },
				[](P) { return FString(TEXT("Use names and functions instead. Nothing in the station is built out of classes.")); });
			Set(EGitsDiagnosticCode::ExcludedExceptionHandling, TEXT("excluded-exception-handling"),
				TEXT("Recognition case 6. Leading try, except, finally or raise."),
				[](P) { return NotHere(TEXT("Catching errors")); },
				[](P) { return FString(TEXT("Check the value with an if before you use it. Failures here are meant to stop the shift and be read.")); });
			Set(EGitsDiagnosticCode::ExcludedWith, TEXT("excluded-with"),
				TEXT("Recognition case 7. Leading with."),
				[](P) { return NotHere(TEXT("A with block")); },
				[](P) { return FString(TEXT("Call the station function directly. Nothing here needs opening and closing around a block.")); });
			Set(EGitsDiagnosticCode::ExcludedLambda, TEXT("excluded-lambda"),
				TEXT("Recognition case 8. lambda in expression position."),
				[](P) { return NotHere(TEXT("A lambda")); },
				[](P) { return FString(TEXT("Write it as a named function with def once functions are available to you.")); });
			Set(EGitsDiagnosticCode::ExcludedComprehension, TEXT("excluded-comprehension"),
				TEXT("Recognition case 9. A for inside a bracket."),
				[](P) { return NotHere(TEXT("A comprehension")); },
				[](P) { return FString(TEXT("Write the loop out in full: start with an empty list, then append to it one item at a time.")); });
			Set(EGitsDiagnosticCode::ExcludedTuple, TEXT("excluded-tuple"),
				TEXT("Recognition case 10. A comma-separated value list."),
				[](P) { return NotHere(TEXT("A tuple, or several values separated by commas")); },
				[](P) { return FString(TEXT("Give each value its own line and its own name. One name holds one value here.")); });
			Set(EGitsDiagnosticCode::ExcludedSlicing, TEXT("excluded-slicing"),
				TEXT("Recognition case 11. A colon inside a subscript."),
				[](P) { return NotHere(TEXT("Slicing a range out of a list")); },
				[](P) { return FString(TEXT("Take one item at a time with a single index, or loop over the list and pick out what you need.")); });
			Set(EGitsDiagnosticCode::ExcludedScopeDeclaration, TEXT("excluded-scope-declaration"),
				TEXT("Recognition case 12. Leading global or nonlocal."),
				[](P p) { return NotHere(FString::Printf(TEXT("A %s declaration"), *SubjectOf(p))); },
				[](P) { return FString(TEXT("Pass the value into the function as an argument and hand it back with return instead.")); });
			Set(EGitsDiagnosticCode::ExcludedKeywordArgument, TEXT("excluded-keyword-argument"),
				TEXT("Recognition case 13. name= inside an argument list."),
				[](P) { return NotHere(TEXT("Naming an argument at the call")); },
				[](P) { return FString(TEXT("Pass the values in order, without the name and the equals sign, matching the order the function lists them in.")); });
			Set(EGitsDiagnosticCode::ExcludedDefaultArgument, TEXT("excluded-default-argument"),
				TEXT("Recognition case 14. = inside a parameter list."),
				[](P) { return NotHere(TEXT("A default value for a parameter")); },
				[](P) { return FString(TEXT("Require the argument every time, and pass it explicitly at each call.")); });
			Set(EGitsDiagnosticCode::ExcludedComparisonChaining, TEXT("excluded-comparison-chaining"),
				TEXT("Recognition case 15. Two comparisons at one level. Refused, never mis-evaluated."),
				[](P) { return FString(TEXT("This line compares three things in a row. Real Python allows that, but this station's interpreter does not, and it will not guess at what you meant.")); },
				[](P) { return FString(TEXT("Split it into two comparisons joined by and: instead of 1 < x < 10, write 1 < x and x < 10.")); });
			Set(EGitsDiagnosticCode::ExcludedMultipleAssignment, TEXT("excluded-multiple-assignment"),
				TEXT("Chained assignment, a = b = 1."),
				[](P) { return NotHere(TEXT("Assigning to several names at once")); },
				[](P) { return FString(TEXT("Give each name its own line, so the order the values are stored in stays visible.")); });
			Set(EGitsDiagnosticCode::ExcludedConstruct, TEXT("excluded-construct"),
				TEXT("A construct on the excluded list with no enumerated case of its own."),
				[](P p) { return NotHere(SubjectOf(p)); },
				[](P) { return FString(TEXT("Check the station manual for what this shift has available to it.")); });

			// --- recognition: not yet
			Set(EGitsDiagnosticCode::NotYetUnlocked, TEXT("not-yet-unlocked"),
				TEXT("Recognition case 16. Correct Python from a tier above the level's."),
				[](P p) { return FString::Printf(TEXT("%s is real Python and it works, but this shift does not have it yet. It arrives at tier %s."), *SubjectOf(p), *DetailOf(p)); },
				[](P) { return FString(TEXT("Solve this one with what the shift gives you. You will get this back later, and it will still be right.")); });
			Set(EGitsDiagnosticCode::MethodNotAvailable, TEXT("method-not-available"),
				TEXT("Recognition case 17. A method call outside the tier whitelist."),
				[](P p) { return FString::Printf(TEXT("There is no %s available on this shift."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Check the spelling. If it is a real Python method, it is from a later tier than this shift has unlocked.")); });

			// --- runtime
			Set(EGitsDiagnosticCode::NameNotDefined, TEXT("name-not-defined"),
				TEXT("A name is read before anything was stored under it."),
				[](P p) { return FString::Printf(TEXT("Nothing has been stored under the name %s yet."), *SubjectOf(p)); },
				[](P p) { return FString::Printf(TEXT("Check the spelling against where %s is set, and check that the line setting it runs before this one."), *SubjectOf(p)); });
			Set(EGitsDiagnosticCode::TypeMismatch, TEXT("type-mismatch"),
				TEXT("An operator applied to kinds of value it does not join."),
				[](P p) { return FString::Printf(TEXT("This line tries to %s."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Check which of the two is which. To join a number onto a piece of text, convert the number first with str().")); });
			Set(EGitsDiagnosticCode::DivisionByZero, TEXT("division-by-zero"),
				TEXT("Division or remainder with a right-hand side of zero."),
				[](P) { return FString(TEXT("This line divides by zero, and nothing can be divided into zero parts.")); },
				[](P) { return FString(TEXT("Check the value on the right of the division. If it can be zero, test for that with an if before dividing.")); });
			Set(EGitsDiagnosticCode::IndexOutOfRange, TEXT("index-out-of-range"),
				TEXT("A list index outside the list."),
				[](P p) { return FString::Printf(TEXT("This asks for item %s of a list that holds %s."), *SubjectOf(p), *DetailOf(p)); },
				[](P) { return FString(TEXT("Counting starts at 0, so the last item of a list of three is item 2. Check the length with len() before reaching in.")); });
			Set(EGitsDiagnosticCode::BadIndexType, TEXT("bad-index-type"),
				TEXT("A list indexed with something other than a whole number."),
				[](P p) { return FString::Printf(TEXT("A list can only be indexed with a whole number, and this is %s."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Check what is between the square brackets. It has to count a position, not name one.")); });
			Set(EGitsDiagnosticCode::NotAList, TEXT("not-a-list"),
				TEXT("Indexing or appending to something that is not a list."),
				[](P p) { return FString::Printf(TEXT("This treats %s as if it were a list, and it is not."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Check which name you meant. Only a list can be indexed or appended to.")); });
			Set(EGitsDiagnosticCode::WrongArgumentCount, TEXT("wrong-argument-count"),
				TEXT("A call with the wrong number of arguments."),
				[](P p) { return FString::Printf(TEXT("%s was given %s."), *SubjectOf(p), *DetailOf(p)); },
				[](P) { return FString(TEXT("Count the values inside the brackets against what the function expects.")); });
			Set(EGitsDiagnosticCode::UnknownFunction, TEXT("unknown-function"),
				TEXT("A call to a name that is not a function."),
				[](P p) { return FString::Printf(TEXT("There is no function called %s on this shift."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("Check the spelling against the builtins and the station functions listed in the manual.")); });
			Set(EGitsDiagnosticCode::BadRange, TEXT("bad-range"),
				TEXT("range() called with a step of zero, or with non-integer arguments."),
				[](P p) { return FString::Printf(TEXT("range() cannot count %s."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("range() takes whole numbers, and its step cannot be 0 because the count would never move.")); });
			Set(EGitsDiagnosticCode::StatementCapExceeded, TEXT("statement-cap-exceeded"),
				TEXT("The per-level design budget (ADR-021). A diagnosable outcome, not a crash."),
				[](P p) { return FString::Printf(TEXT("This program is still running after %s steps of work, which is more than this shift has power for."), *SubjectOf(p)); },
				[](P p) { return FString::Printf(TEXT("Look at the loop on line %s. Something inside it has to change each time round, or the condition that ends it will never come true."), *DetailOf(p)); });
			Set(EGitsDiagnosticCode::SafetyCapExceeded, TEXT("safety-cap-exceeded"),
				TEXT("The project-wide runaway guard (ADR-008). An error path."),
				[](P) { return FString(TEXT("This program never stops. It was cut off to keep the station responsive.")); },
				[](P p) { return FString::Printf(TEXT("Look at the loop on line %s. Check that the value its condition tests actually changes inside the loop."), *DetailOf(p)); });
			Set(EGitsDiagnosticCode::CallDepthExceeded, TEXT("call-depth-exceeded"),
				TEXT("Recursion deeper than MAX_CALL_DEPTH."),
				[](P p) { return FString::Printf(TEXT("This function has called itself %s times without finishing, and it was stopped there."), *SubjectOf(p)); },
				[](P) { return FString(TEXT("A function that calls itself needs a case that returns without calling again. Check that case is reachable.")); });
```

### 16.2 One representative script DataAsset: `Content/Scripts/DA_S1_DoorLog` as read from the editor

```
title = 'AIRLOCK 1  door log'
source = '# main door counter\n# ilse, day 3\n# a name on the left, a value on the right. the name keeps the value.\n\nopened = 7\n\n# print shows what the name is keeping.\nprint(opened)\n'
tier = 1
statement_cap = 2000
editable_lines = []
declared_builtins = ['print']
concepts = ['variable-assignment', 'output']
hints = [{'tier': 1, 'text': "Ilse's own note is on line 3. She wrote it for herself and it still holds.", 'costs_power': 0}, {'tier': 2, 'text': 'Line 5 puts 7 under the name opened. Line 8 asks for whatever opened is keeping, and shows that.', 'costs_power': 3}]
intro = 'Nothing in the airlock works until the door log reports. It is the first script Ilse wrote here and it is four lines long. Read it. You have power for one run and no reason to waste it.'
outro = 'The log reports. The inner door releases.'
goal_key = ''
goal_value = ''
goal_output = ['7']
test_cases = []
goal_unlocks = 'door.entry=true'
counts_for_sector = True
log_entry = 'Day 3. First script I have written for this place. It counts the door. That is all it does, and the man who trained me said that is a start. I have written down what every line means because I do not trust myself to remember.'
free_edit = False
panel_title = 'DOOR LOG'
level_id = 'a1-l01'
act = 1
run_cost = 10
predicted_run_cost = 3
prediction: p1 'Before you spend the power: what will line 8 put on the log?' anchor 8 1 output correct a
   option: a '7' '' ''
   option: b 'opened' 'type-confusion' 'It printed the name, not what the name keeps. The interlock wants a number.'
   option: c '0' 'sequence-ignored' 'Line 5 ran before line 8; the name was keeping 7 by then.'
```

The asset is authored by `Tools/setup_sector1_content.py` (the `ensure_script("DA_S1_DoorLog", ...)`
call), which is the human-readable source of every field above.

### 16.3 Tag enumerations verbatim (`Source/GhostInTheStack/Companion/GitsTags.cpp`)

```cpp
	const TArray<FString>& Concepts()
	{
		static const TArray<FString> List = {
			TEXT("variable-assignment"), TEXT("reassignment"), TEXT("data-types"), TEXT("type-coercion"),
			TEXT("arithmetic"), TEXT("string-ops"), TEXT("output"), TEXT("conditional"), TEXT("elif-chain"),
			TEXT("boolean-logic"), TEXT("comparison"), TEXT("truthiness"), TEXT("while-loop"), TEXT("for-loop"),
			TEXT("range"), TEXT("accumulator"), TEXT("nested-loop"), TEXT("augmented-assignment"),
			TEXT("list-literal"), TEXT("indexing"), TEXT("list-mutation"), TEXT("function-def"),
			TEXT("parameters"), TEXT("return-value"), TEXT("local-scope"), TEXT("call-stack"), TEXT("recursion"),
		};
		return List;
	}

	const TArray<FString>& Misconceptions()
	{
		// Each entry is a belief a novice actually holds, not a way of being wrong.
		static const TArray<FString> List = {
			TEXT("assignment-as-equality"), TEXT("parallel-assignment"), TEXT("sequence-ignored"),
			TEXT("type-confusion"), TEXT("branch-both"), TEXT("inverted-comparison"), TEXT("operator-confusion"),
			TEXT("loop-runs-once"), TEXT("fencepost"), TEXT("accumulator-reset"), TEXT("index-from-one"),
			TEXT("scope-leak"), TEXT("return-vs-print"), TEXT("arg-param-identity"), TEXT("recursion-no-return"),
		};
		return List;
	}
```

27 concept tags, 15 misconception tags.

### 16.4 Fixture runner output, full

`Tools\run_interpreter_tests.cmd` → `passed=57 failed=0`. Every `Test Completed` line from
`Saved/Logs/GitsTests.log` (11 Sep 2026), sorted by path:

```
Success GhostInTheStack.Companion.Shift.Anchors
Success GhostInTheStack.Companion.Shift.Gate
Success GhostInTheStack.Companion.Tags.Validate
Success GhostInTheStack.Interpreter.Diagnostics.Catalogue
Success GhostInTheStack.Interpreter.Evaluator.Caps
Success GhostInTheStack.Interpreter.Evaluator.Oracle
Success GhostInTheStack.Interpreter.Evaluator.PythonSemantics
Success GhostInTheStack.Interpreter.Evaluator.RuntimeErrors
Success GhostInTheStack.Interpreter.Evaluator.StationBuiltins
Success GhostInTheStack.Interpreter.Evaluator.Tier4
Success GhostInTheStack.Interpreter.Golden.a1-l03-cold-store
Success GhostInTheStack.Interpreter.Golden.outcome-call-depth
Success GhostInTheStack.Interpreter.Golden.outcome-division-by-zero
Success GhostInTheStack.Interpreter.Golden.outcome-name-not-defined
Success GhostInTheStack.Interpreter.Golden.outcome-statement-cap
Success GhostInTheStack.Interpreter.Golden.outcome-type-mismatch
Success GhostInTheStack.Interpreter.Golden.station-effects
Success GhostInTheStack.Interpreter.Golden.station-oracle-read
Success GhostInTheStack.Interpreter.Golden.tier1-arithmetic
Success GhostInTheStack.Interpreter.Golden.tier1-builtins
Success GhostInTheStack.Interpreter.Golden.tier1-literals
Success GhostInTheStack.Interpreter.Golden.tier1-python-sign-rules
Success GhostInTheStack.Interpreter.Golden.tier1-strings
Success GhostInTheStack.Interpreter.Golden.tier2-boolean-logic
Success GhostInTheStack.Interpreter.Golden.tier2-comparison
Success GhostInTheStack.Interpreter.Golden.tier2-conditional
Success GhostInTheStack.Interpreter.Golden.tier2-truthiness
Success GhostInTheStack.Interpreter.Golden.tier3-accumulator
Success GhostInTheStack.Interpreter.Golden.tier3-break-continue
Success GhostInTheStack.Interpreter.Golden.tier3-for-range
Success GhostInTheStack.Interpreter.Golden.tier3-list-is-a-reference
Success GhostInTheStack.Interpreter.Golden.tier3-lists
Success GhostInTheStack.Interpreter.Golden.tier3-nested-loop
Success GhostInTheStack.Interpreter.Golden.tier3-while
Success GhostInTheStack.Interpreter.Golden.tier4-function
Success GhostInTheStack.Interpreter.Golden.tier4-local-scope
Success GhostInTheStack.Interpreter.Golden.tier4-nested-calls
Success GhostInTheStack.Interpreter.Golden.tier4-recursion
Success GhostInTheStack.Interpreter.Golden.tier4-return-forms
Success GhostInTheStack.Interpreter.Golden.tier4-return-out-of-loop
Success GhostInTheStack.Interpreter.GoldenCoverage
Success GhostInTheStack.Interpreter.Lexer
Success GhostInTheStack.Interpreter.Parser.MalformedInput
Success GhostInTheStack.Interpreter.Parser.RecognitionCases
Success GhostInTheStack.Interpreter.Parser.RecognitionMessages
Success GhostInTheStack.Interpreter.Parser.Recovery
Success GhostInTheStack.Interpreter.Parser.Structure
Success GhostInTheStack.Interpreter.Rng.Parity
Success GhostInTheStack.Interpreter.Trace.Diff
Success GhostInTheStack.Interpreter.Values
Success GhostInTheStack.Recorder.Loops
Success GhostInTheStack.Station.Power.Stages
Success GhostInTheStack.Station.Power.Validate
Success GhostInTheStack.Telemetry.ConsentGate
Success GhostInTheStack.Telemetry.Instruments.Scoring
Success GhostInTheStack.Telemetry.Instruments.TracingItems
Success GhostInTheStack.Telemetry.PiiTripwire
```

### 16.5 Golden fixture files (`Fixtures/`, 60 files, one commit `012ac94`)

a1-l03-cold-store, outcome-call-depth, outcome-division-by-zero, outcome-name-not-defined,
outcome-statement-cap, outcome-type-mismatch, station-effects, station-oracle-read,
tier1-arithmetic, tier1-builtins, tier1-literals, tier1-python-sign-rules, tier1-strings,
tier2-boolean-logic, tier2-comparison, tier2-conditional, tier2-truthiness, tier3-accumulator,
tier3-break-continue, tier3-for-range, tier3-list-is-a-reference, tier3-lists, tier3-nested-loop,
tier3-while, tier4-function, tier4-local-scope, tier4-nested-calls, tier4-recursion,
tier4-return-forms, tier4-return-out-of-loop — each as `<name>.py` and `<name>.trace.txt`.

### 16.6 Commands used for this document

```
git log -1 --format="%H %ad %s" --date=iso
git log --reverse --format="%h %ad %s" --date=short
git log --format="%h %ad %s" --date=short -- Fixtures/ ; git log --diff-filter=M -- Fixtures/ | wc -l
find Source -name "*.cpp" -o -name "*.h" -o -name "*.cs" | xargs cat | wc -l   (and per directory)
find Content -name "*.uasset" | cut -d/ -f2 | sort | uniq -c ; find Content -name "*.umap"
Tools\run_interpreter_tests.cmd ; grep "Test Completed" Saved/Logs/GitsTests.log
python Tools/check_curriculum.py
"...\blender.exe" --background Content/Kit/Kit.blend --python-expr "<count SM_Kit_/UCX_ objects>"
npm run analyse -- --in <dir>   (reference repo) ; npx vitest run (reference repo)
GitsFrameSample / GitsStatus / GitsVerifyRewind in PIE via Tools/ue_remote_python.py
```

# Ghost in the Stack — 3D redesign

Supervisor evaluation, 8 September 2026: pedagogically sound, not fun. Make a 3D game people enjoy
playing and coding in. This document is the response.

Under Design Science Research this is the second design cycle, driven by expert evaluation of the
first. The pedagogy survived the evaluation. The presentation did not.

---

## 1. What the first cycle got right, and what it got wrong

**Right, and kept:** reading before writing; PRIMM as mechanics; prediction gating execution; the
scrubbable execution trace; running unchanged source being impossible; misconception-tagged
distractors; the vanished maintainer whose code is the story.

**Wrong, and replaced:** the game was a text editor with a ribbon. Code had no physical consequence.
The player had no body, no place to be, nothing to want. Every system was a panel. KI-63 said it
weeks ago: *it looks like a well-organised document.*

The fix is not a coat of 3D paint over the same screens. It is making code the thing that moves the
world.

---

## 2. The design principle for cycle 2

**Every line of code does something you can see, and every bug does something you can laugh at.**

This is both the fun principle and the pedagogy principle, and that is not a coincidence. A
misconception that produces a visible, physical, slightly ridiculous failure is a misconception the
learner remembers. An off-by-one that makes a drone fly one bay too far and bonk into a wall teaches
fencepost errors better than any red underline.

---

## 3. The game

### Premise (unchanged)

You arrive at Vantskär Station on a maintenance contract. The previous maintainer, Ilse Rask, left
eleven months ago and did not come back. Everything still runs on her code. Some of it works. You're
here for the rest.

### What is new: it's a place

First person. You walk it. Corridors, a greenhouse under grow-lights, a cold store, a drone bay, a
reactor hall you can't get into yet. Cold, over-engineered, 1978 industrial — enamelled signage,
heavy doors, warm monitors in dark rooms. Stylised and clean, not photoreal. Lumen makes modest
geometry look good; spend the art budget on lighting and silhouette, not on texture detail.

The station is quiet when you arrive. As you fix things it wakes up around you. Lights come on
sector by sector. Conveyors start. The greenhouse sprinklers run. Drones lift off. **Progress is
audible and visible, not a level-select screen.**

### The companion

VANT, the station's management system. Dry, tired, has been alone for eleven months and is not sure
about you. It's the voice of the game — narrator, hint-giver, the thing that asks you to predict, the
thing that comments when a drone hits a wall. It gives the game a character to talk to and a reason
for predictions to feel like conversation rather than a quiz.

Ilse's story arrives through VANT's memories and her logs, and through her code.

### The core loop, moment to moment

1. **Find a broken system.** A door that won't open. A drone that keeps crashing into the same wall.
   Sprinklers flooding the greenhouse. You see the problem before you see the code.
2. **Pull up its terminal.** Ilse's script, 8–20 lines, in a diegetic terminal in the world.
3. **VANT asks you to predict.** *"Before you run that — what's the drone going to do?"* Three or
   four options, each a real belief about the code. Committing is free.
4. **Run it.** Power draws down visibly. And then you **watch it happen in the world.** The drone
   flies. Or the drone flies one bay too far and bonks. Or the sprinklers fire in the wrong order.
   Failures are spectacle. This is the moment the whole game is built around.
5. **Rewind.** VANT's recorder. Hold the key and time scrubs backward — the drone reverses along its
   path, water un-sprays — while the terminal highlights the line executing at each moment. The
   trace scrubber is now rewinding a physical scene. Braid, for code.
6. **Fix a line. Run. It works.** The door opens. The lights in the next corridor come on.

### Power, made physical

The station has a power budget you can see: a gauge on the wall, and the lights. Running a script
costs power. A committed prediction makes the run cheaper. When power gets low the lights dim and
systems go dark; when it runs out you're in the dark with a torch until you restore a generator.

Wrong predictions cost nothing but attention, exactly as before: VANT won't let you re-answer until
you've rewound and watched.

### Bugs are choreography

Each misconception tag gets a physical failure. This is the single most important content-authoring
decision in the redesign, and it is also what makes the misconception telemetry legible to a player.

| Misconception | What the player sees |
|---|---|
| `fencepost` | Drone flies one bay too far, bonks the wall, sits there beeping |
| `loop-runs-once` | Conveyor advances one item and stops, a queue backs up behind it |
| `accumulator-reset` | Counter display flickers back to zero every tick, door never reaches its threshold |
| `assignment-as-equality` | Heater set to a value that then never changes; frost creeps across the window |
| `type-confusion` | Display reads `4metres`; door does nothing; VANT: *"It's waiting for a number."* |
| `inverted-comparison` | Sprinklers run when the soil is wet and stop when it's dry |
| `scope-leak` | Drone tries to use a value that only existed inside its landing routine and drifts |
| `return-vs-print` | Sensor prints its reading to the wall display but the door listening for a value gets nothing |
| `index-from-one` | The wrong crate gets picked, every time, one bay off |

Every one of these is funny once and clear forever.

### Progression: sectors are acts

The station is four sectors, gated physically.

- **Sector 1 — Habitation.** Lights, doors, the cold store. Variables, types, sequence, output.
  Restore power to open the airlock into Sector 2.
- **Sector 2 — Greenhouse.** Sprinklers, grow-lights, heaters. Conditionals and comparison. Get the
  crops alive to open the drone bay.
- **Sector 3 — Logistics.** Conveyors, drones, crates. Loops, lists, indexing. Get the supply line
  running to reach the reactor.
- **Sector 4 — Reactor.** Coolant routines, the emergency systems, what happened to Ilse.
  Functions, scope, recursion.

Each sector ends with a **Make** task: write your own script. Sector 1's is four lines and lights up
a corridor. Sector 3's writes a drone route.

### Make becomes automation, and automation is the long game

Once a sector is restored, you can write scripts for your own drones to keep it running — harvest the
greenhouse, restock the cold store, patrol for faults. Your code works while you walk around. This is
where the game gets legs past the four sectors: the station as an automation sandbox, and the
satisfaction of a place that runs on code you wrote. The Farmer Was Replaced lives on this loop.

---

## 4. What carries over

The architecture was built for a renderer swap. ADR-016 made the view a skin over
`(trace, stepIndex)`. That contract holds.

| Kept | Notes |
|---|---|
| Language subset, tiers, recognition pass | Spec unchanged |
| Interpreter design: stepper, oracle, recorded trace, observable-state diff | **Ported to C++**, verified against the existing 30 golden trace fixtures |
| Beginner error catalogue, 46 codes | Ported; now spoken by VANT |
| Level content: 30 levels, concept and misconception tags | Re-authored as systems in the world, same concept mapping |
| Power economy: free prediction, discounted run, scrub gate, no unchanged re-runs | Made physical, rules unchanged |
| Telemetry taxonomy, consent gate, instruments | Ported to C++; export format unchanged so `analyse.ts` still runs |
| DECISIONS.md, known issues | Continue; this redesign is a new ADR |

| Replaced | By |
|---|---|
| CodeMirror editor | Diegetic UMG terminal, monospace, syntax tint via RichText |
| SVG diagram view | The world itself. Rooms are literally rooms. |
| PixiJS isometric view | Deleted |
| Trace ribbon | VANT's recorder: hold-to-rewind with terminal line highlight |
| Prediction panel | VANT asking, in voice and on the terminal |
| Level select | Walking through a door |

The TypeScript repo becomes the **reference implementation**. It is where the fixtures live, where
the language spec is authoritative, and where any interpreter behaviour question is settled.

---

## 5. Engine decision

**Unreal Engine 5.8**, with Blender for the modular station kit.

**Why Unreal now, having argued against it in August.** The objection was tooling: Claude Code
could not author Blueprints, see the editor, or iterate quickly. Epic now ships an official MCP
plugin embedded in the editor and an official Claude Code plugin covering Blueprint graphs, UMG,
materials, Niagara, screenshots and automation tests. Third-party servers add headless builds and
in-play viewport capture. The objection is gone. What Unreal was always best at — a lit, physical,
atmospheric 3D place — is exactly what the supervisor asked for.

**Why 5.8, revising the earlier 5.7 call.** The plan had been 5.7 with a community backport of
the MCP plugin, on the belief that the official 5.8 plugin needed a source build. It does not:
the 5.8 launcher build ships it prebuilt under `Engine/Plugins/Experimental`, with the toolset
plugins beside it. The official plugin removes a third-party compile risk, and 5.8 is a launcher
build too. Revised in Phase 0, 10 September 2026.

**Why C++ for the interpreter, not Blueprints.** The interpreter is a tree-walking evaluator with
recorded traces. That is what C++ is for. Blueprints for world interaction, sequencing, UI wiring.

**Why not embed the TypeScript interpreter.** Puerts (TypeScript in Unreal) and a CEF web view were
both considered. Both would run the existing code as-is. Both add a runtime and a bridge to a
commercial game for the sake of not porting 5,000 lines that have a complete fixture suite to verify
the port against. Porting is the cleaner long-term answer, and the fixtures make it safe.

**Blender.** Modular kit: corridor segments, door frames, terminal, wall gauge, drone, crate,
conveyor segment, sprinkler head, grow-light, generator. Low-poly, clean, consistent scale.
Exported as FBX/glTF with collision. Claude Code drives Blender through the existing MCP.

---

## 6. Architecture in Unreal

```
Source/GhostInTheStack/
  Interpreter/        C++ port: lexer, parser, evaluator, trace, errors, oracle
  World/              Station systems as Actors; effect reducer; power
  Systems/            Door, Drone, Conveyor, Sprinkler, Heater… each binds effects to animation
  Recorder/           trace ↔ world-time mapping; rewind
  Companion/          VANT: dialogue, prediction prompts, hints
  Telemetry/          event log, consent gate, JSON export (format unchanged)
Content/
  Kit/                Blender-authored modular meshes
  Sectors/            Four levels
  Scripts/            Ilse's code, one .py per system, as DataAssets
```

**The contract that survives:** the interpreter produces a `Trace`. Systems consume
`(trace, stepIndex)` and play the corresponding animation state. Rewind is stepping the index down
and letting every system pose itself for that step. Because the trace is recorded, rewinding a
physical scene is the same trivial operation it was in the web version. Nothing is simulated
backward.

---

## 7. What makes it fun, stated plainly so it can be checked

- You have a body and a place. You want to see what's through the next door.
- Code moves things you can see. Run means *watch*.
- Bugs are spectacle. Failure is entertaining, not punishing.
- Rewinding a physical scene while the code highlights is a mechanic nobody else has.
- The station wakes up as you fix it. Progress is lights coming on.
- VANT has a personality. There is someone to talk to.
- Automation as the endgame. Your code runs the station.

If a feature does not serve one of those seven, it does not go in.

---

## 8. October demo: the vertical slice

Sector 1 only. First person, four systems (two doors, the cold store, corridor lights), one drone,
VANT speaking, rewind working, power visible, one Make task. Fifteen minutes of play.

That is enough to show every mechanic once and to make someone laugh when the drone bonks.

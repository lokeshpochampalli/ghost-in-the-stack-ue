# Decisions

Architecture decision records. Every non-obvious choice, written so it can be lifted into chapter 5
or 8 of the dissertation and defended out loud.

Format: what was decided, what else was considered, why this one. Append, never rewrite. If a
decision is reversed, add a new entry superseding the old one rather than editing history.

---

## ADR-001 — Step granularity is per node, with statement boundaries marked

**Decided.** The evaluator emits one `Step` per AST node evaluation, not per statement.

Every `Step` additionally carries `depth` (expression nesting), `stmtIndex` (which statement it
belongs to), and `isStatementBoundary`. The scrubber filters on `isStatementBoundary` for its
default view and expands to node detail on zoom.

**Considered:** statement granularity, which keeps the ribbon legible without any LOD work.

**Why node.** Sub-expression evaluation order is precisely what novices get wrong. `total = count +
price * 2` evaluated as a single opaque statement teaches nothing about precedence; evaluated as
five steps it teaches it directly. Choosing statement granularity would have made the trace legible
at the cost of removing the thing the trace exists to show. The ribbon's legibility problem is
solvable in the view layer (ADR-008); the pedagogy problem would not have been solvable anywhere.

Every golden fixture encodes this choice, so it had to be made before the first one was written.

---

## ADR-002 — NodeId is a structural path; diff compares observable state

**Decided.** Two changes.

`NodeId` is a structural path derived from position in the tree, not a traversal counter:
`body.3.test.left`. Inserting a line above a node no longer renames it.

`diff(a, b)` compares **observable state**, defined as: bindings visible in every live frame, text
appended to output, and effects emitted. It does not compare `nodeId`, `span`, or step index.

Alignment is by statement-boundary sequence, not raw index, because an edit changes trace length.
The diff walks the statement boundaries of both traces in order and reports the first position where
observable state diverges, plus a categorised summary of what changed.

**Considered:** diffing on `nodeId`, which is what the original spec implied.

**Why.** With a traversal counter, inserting one line renumbers everything downstream, so every step
after the insertion point differs structurally even when the program behaves identically — the
divergence view would report step 0 for almost every edit and be useless. Observable state is also
the pedagogically correct thing to compare: the player wants to know where the machine started
behaving differently, not where the tree changed shape.

---

## ADR-003 — The evaluator takes a WorldOracle and records every read

**Decided.** The evaluator's signature becomes:

```ts
evaluate(ast: Program, initialWorld: WorldState, oracle: WorldOracle, seed: number): Generator<Step>
```

where `WorldOracle` is `(world: WorldState, query: Query) => Value` — pure, synchronous, total.

The evaluator maintains an incremental fold of the effects it has emitted so far, and calls the
oracle against that folded world when a reading builtin is invoked. Every oracle response is
recorded in the step:

```ts
oracleRead: { query: Query; value: Value } | null;
```

Replay and scrubbing read the recorded value. They never re-consult the oracle.

`wait(ticks)` is an effect, not a read. It advances the folded world; the next `read_sensor` sees
the updated state.

**Considered:** making the evaluator impure and letting it hold a world reference; and dropping
reading builtins entirely so the interpreter stays a pure source-to-trace function.

**Why.** The original architecture was circular: effects flowed out of the evaluator, but
`read_sensor` needs values flowing in. The oracle resolves this by making the dependency explicit
and injectable. Purity is preserved in the sense the tests and the dissertation actually rely on —
referential transparency given `(source, initialWorld, oracle, seed)`, so a run is fully
reproducible from recorded inputs. Recording the read in the step is what makes replay independent
of the world, which is what makes scrubbing free.

This is a likely viva question. The short answer: purity was never the goal, reproducibility was,
and injection buys reproducibility without banning interaction.

---

## ADR-004 — WorldState is an open record with a per-level declared schema

**Decided.** `WorldState` is `Record<string, number | string | boolean>`.

Every level declares the keys it uses:

```ts
worldSchema: Record<string, "number" | "string" | "boolean">;
```

The level validator checks `world.initial` and `Goal.assert` against `worldSchema`. In development
the reducer throws if an effect touches an undeclared key.

**Considered:** a single closed facility type shared by all levels.

**Why.** A closed type forces the cold store, the beacon and the water reclaimer to coexist in one
interface forever, and every new level widens a type that every old level then carries. An open
record with per-level declaration keeps levels independent while keeping validation real — the
validator has something concrete to check, which a bare `Record<string, Value>` would not.

---

## ADR-005 — Predictions anchor to a source location, not a step index

**Decided.** `Prediction.atStep` is removed. Replaced by:

```ts
anchor: { line: number; occurrence: number };
```

Resolved against the trace's statement boundaries at prediction time.

If the anchor cannot be resolved — the player edited the line away under a `lines` or `region`
policy — the prediction is marked unresolvable, skipped, and logged as `prediction_unresolvable`.
It does not block level completion.

**Considered:** keeping step indices and re-authoring them whenever level source changes.

**Why.** A step index refers to a trace that does not exist until the program runs, silently
re-points whenever the author edits the level source, and can refer to a step that no longer exists
once the player edits. A source anchor survives all three. The `occurrence` field handles lines
inside loops, where one line produces many steps.

---

## ADR-006 — Prediction is a discount on execution, not a refund after it

**Decided.** The power model becomes:

```ts
power: {
  budget: number;
  runCost: number;           // full price, no prediction committed
  predictedRunCost: number;  // reduced price when a prediction is committed
}
```

Validator-enforced invariants: `budget >= runCost` and `predictedRunCost < runCost`.

Committing a prediction is free and unlocks the discounted run. Correctness is revealed at the
moment of execution, not before.

A wrong prediction costs no power. It instead **locks that prediction until the player has scrubbed
the trace from step 0 through the anchor at least once.** Logged as `scrub_gate_satisfied`. After
the gate, the prediction may be re-answered; every attempt is logged.

**Considered:** the original refund-after-run model, and charging power for wrong answers.

**Why the refund model failed.** It was circular. If the refund arrives before the run it reveals
the answer and destroys PRIMM's Run stage; if it arrives after, it cannot make the first run
affordable. The discount has neither problem.

**Why attention rather than power for wrong answers.** Charging power punishes the novice hardest
at exactly the moment they most need another attempt, and it creates a death spiral. The scrub gate
instead makes the consequence of a wrong prediction *doing the thing that fixes the wrong
prediction*. It converts a penalty into the Investigate stage. It also produces better data: the
scrub that follows a wrong answer is observable behaviour tied to a named misconception.

---

## ADR-007 — Language spec corrections

**Decided.** Five fixes to `02-LANGUAGE-SPEC.md`.

**Augmented assignment** is permitted at Tier 3 (`+=`, `-=`, `*=`, `//=`). Removed from the
exclusions list. Accumulator patterns are Act 3's core content and `total += x` is what real Python
looks like; teaching `total = total + x` exclusively would be teaching a dialect.

**Method calls are real.** The grammar gains attribute access and method-call syntax, with a
per-tier whitelist of permitted methods. Tier 3 permits `list.append(x)` and nothing else. Calling
an unwhitelisted method produces a recognition message, not a parse error.

Rejected the alternative of a builtin `append(list, item)`. It is a smaller grammar but it is not
Python, and transferability to real Python is a claim the dissertation makes. Breaking it to save
grammar work would undermine the artefact's stated contribution.

**Truthiness splits by tier.** Tier 2 covers `0`, `0.0`, `""`, `None`, `False`. `[]` moves to
Tier 3, where lists exist.

**String methods:** none at any tier. The garbled line in the original spec is resolved this way.

**Excluded syntax needs a recogniser, not a rejecter.** This is real Phase 1 scope and the original
acceptance criteria did not cover it. You cannot say "dictionaries are not part of this station's
system" without lexing `{` and parsing far enough to know it is a dict rather than a set. The parser
gains an explicit recognition pass with a case and an acceptance test for each of: dict and set
literals, f-string prefixes, `import`, `class`, `try`/`except`, `with`, `lambda`, comprehensions
(a `for` inside a bracket), tuples, slicing, `global`/`nonlocal`, keyword arguments, default
arguments, and comparison chaining.

Comparison chaining deserves note: `1 < x < 10` is valid Python that we deliberately do not
implement. It is recognised and refused rather than implemented incorrectly, because silently
evaluating it as `(1 < x) < 10` would teach a falsehood.

---

## ADR-008 — Two caps, and a three-level scrubber LOD

**Decided.** Separate the safety net from the design budget.

- **Safety cap:** 50,000 steps. Protects against runaway loops. Hitting it is an error path with no
  latency target.
- **Pedagogical cap:** 2,000 steps, per-level configurable. Exceeding it is a diagnosable outcome
  presented to the player, not a crash.

The scrubber ribbon has three levels of detail:

1. **Statement notches** when the trace has 300 or fewer statement boundaries
2. **Density bands** above that, with notches on hover
3. **Collapsed loops** — an entire loop's iterations render as one expandable notch labelled with
   the iteration count

The revised performance target: parse and trace a 40-line program producing 2,000 steps or fewer in
under 50 ms.

**Why.** The original defaults were mutually incompatible: one notch per step at a 50,000 cap is
0.03px per notch on a 1500px ribbon, and the signature element did not survive its own
configuration.

Loop collapsing earns its place on pedagogy rather than performance. A notch reading "40 iterations,
expand" is a more useful representation of a loop than forty identical notches, because it makes the
repetition itself visible as a single object. That is the concept being taught.

---

## ADR-009 — Trace access goes through an accessor, always

**Decided.** Consumers never index the raw trace array. All access goes through
`traceAt(trace, i): Step` and `statementBoundaries(trace): number[]`.

The initial implementation stores full snapshots per step. When profiling shows it matters — Act 4
recursion is the expected trigger — the representation changes to keyframes every 500 steps plus
deltas, reconstructed on demand behind the same accessor.

**Why.** The representation decision is genuinely premature now, but the *coupling* decision is not.
Fixing the accessor boundary today makes the later change a single-file edit instead of a refactor
across the renderer, the scrubber, the diff and the telemetry.

---

## ADR-010 — A step can produce multiple outputs and multiple effects

**Decided.** `output: string[]` and `effects: Effect[]`, both possibly empty. The original
single-valued fields could not represent a step that both prints and emits.

---

## ADR-011 — Prediction options are shuffled deterministically

**Decided.** Option order is shuffled via `src/core/rng.ts`, seeded from
`hash(sessionId + predictionId)`. The session seed is recorded in `session_start` and the
presentation order in `prediction_shown`.

**Why.** Options must be shuffled or position becomes a confound in the telemetry — an unshuffled
correct answer that always sits first produces meaningless accuracy data. But `Math.random()` is
banned project-wide because research runs must be replayable. Seeding from the session and
prediction ids gives both: shuffled across participants, reproducible for any given participant.

---

## ADR-012 — Make is a per-act mechanic, and it gets a goal kind

**Decided.** Two changes.

A new goal kind validates player-written programs:

```ts
| { kind: "tests"; cases: TestCase[] }

interface TestCase {
  label: string;                        // shown to the player
  expectOutput?: string[];
  expectWorld?: Partial<WorldState>;
}
```

Failures are presented in the station's voice — "the beacon expected three pulses and got one" —
never as assertion output. Test labels are visible before the player writes; only the expected
values are hidden.

**Make levels appear in every act, not only at the end.** Each act closes with one `EditPolicy:
free` level using `goal: tests`. Act 1's is four lines long.

**Why the second change matters more than the first.** The original plan deferred all original
authorship to Act 4, which contradicts the framework the project is named after. PRIMM is a cycle
run repeatedly at increasing scope, not a five-stage progression through a game. Sentance and
Waite's own materials run the full cycle within a single lesson. Reaching Make only once, at the
end, would have meant the artefact implemented four fifths of the framework it claims to
operationalise — an obvious and fair line of attack in a viva.

---

## ADR-013 — Minimal telemetry lands in Phase 3, not Phase 8

**Decided.** The telemetry core — append-only event log, IndexedDB store, JSON export, consent gate
— moves into Phase 3. The research instruments (pre/post test, MEEGA+, SUS, IMI) and the analysis
script stay in Phase 8.

**Why.** Every playtest from Phase 3 onward then produces evidence. Under the original ordering,
Phase 7's acceptance criterion — a first-time player completes Act 1 unassisted in under 45 minutes
— would have generated no recorded data at all, which is a lost pilot study. The core is cheap,
independently testable, and has no dependency on the instruments.

The consent gate is a hard requirement, not a Phase 8 concern: no event is written before consent is
recorded, and there is a test that proves it.

---

## ADR-014 — Edit events capture the code, with consent

**Decided.** The `edit` payload becomes:

```ts
{ line: number; before: string | null; after: string | null; hashed: boolean }
```

Full text when the participant has consented to code capture; hashes only, with `hashed: true`, when
they have not.

**Why.** Hashes tell you that a line changed and nothing else, which means the Modify stage produces
no analysable data and the entire misconception story rests on multiple-choice selections. The
difference is between reporting "37% of edits were reverted" and reporting "players holding
parallel-assignment edit the second assignment first." The second is a finding; the first is a
statistic.

Requesting code capture is an ordinary thing to ask for and costs one sentence in the participant
information sheet. It must be in the sheet before the ethics application is submitted, not added
afterwards.

---

## ADR-015 — Golden traces are compact text snapshots

**Decided.** Fixtures keep `source.py` as input. Expected traces serialise to one line per step via
Vitest `toMatchFileSnapshot`:

```
003  assign     L4:0-14   d0  base_temp=4
004  binop      L6:12-31  d1  base_temp-adjust -> 2
005  assign     L6:0-31   d0  base_temp=2
006  call       L8:0-17   d0  print("2")
```

**Why.** Raw `expected-trace.json` with full frame snapshots per step makes a fifteen-line fixture
into hundreds of lines of JSON. Any evaluator change churns every fixture simultaneously, nobody
reviews the diff, and the fixtures stop catching regressions — which defeats both their engineering
purpose and their purpose as dissertation evidence. The text format is reviewable at a glance,
produces legible diffs, and drops into the appendix without reformatting.

---

## ADR-016 — Two renderers, one contract

**Decided.** Both views are built. Neither is cut.

- `render/DiagramView` — SVG and DOM. Frames as nested rooms, bindings as labelled objects.
  Keyboard-navigable, screen-reader legible, inherits the palette. This is the **pedagogical**
  view and it ships in Phase 6.
- `render/IsometricView` — PixiJS. The same facility rendered in the station's visual language.
  This is the **presentation** view and it ships in Phase 10.

Both consume `(trace, stepIndex)` and are stateless with respect to it. A settings toggle switches
between them. The evaluation study runs on `DiagramView` so that accessibility is not a confound.

**Why both.** The isometric view is a genuine commercial asset and there is no reason to lose it.
But the view the research is conducted on must be inspectable, keyboard-operable and free of
rendering-engine variance, and it must be the one an examiner can reason about. Separating them by
purpose gets both without compromising either, and the shared contract means the second renderer is
a skin rather than a second implementation.

---

## ADR-017 — The prediction gate is our answer to Sorva

**Decided.** This is a dissertation argument, recorded here because it shapes the design.

Sorva, Karavirta and Malmi's review of generic program visualisation systems is notably
unenthusiastic about their measured learning gains. We cite it, and building an elaborate visualiser
while citing it looks like an unforced error. It is not.

The most plausible explanation for those weak results is that generic visualisation systems are
**passive**. A learner can watch Python Tutor step through a program indefinitely without ever
committing to a belief about what happens next, and watching is not the same as predicting. The
literature that does show gains — PRIMM, and the prediction work underneath it — has commitment as
its active ingredient.

Our visualiser is never reachable without a committed prediction. The gate is the variable those
systems lack. That claim should be stated explicitly in the discussion chapter and it is the
strongest available answer if an examiner raises Sorva.

It also implies a possible extension study: the same artefact with the gate removed is a clean
control condition. Worth noting in future work even if there is no time to run it.

---

## ADR-018 — instruments module contract

**Decided.** `src/instruments/` was in the repo layout with no contract. It is now defined.

Each instrument exports a pure descriptor — items, scale, scoring function, and the events it emits
— consumed by a single generic renderer in `src/ui/`. Instruments never touch storage directly; they
emit telemetry events like everything else.

```
instruments/
  tracingTest.ts    pre/post code-tracing items, scored
  meegaPlus.ts      MEEGA+ items and dimensions
  sus.ts            System Usability Scale
  imi.ts            Intrinsic Motivation Inventory, short form
  types.ts
```

Keeping the items as data rather than as components means the exact instrument used can be printed
into the dissertation appendix directly from source.

---

## ADR-019 — Ethics scope must be confirmed before Phase 3

**Open action, not yet decided. Blocks nothing in code but gates the whole evaluation.**

`05-EVALUATION-AND-DISSERTATION.md` reframes the methodology from the approved proposal's
"primarily qualitative" to Design Science Research, and the design now includes instrumented
behavioural telemetry (ADR-013) and captured code diffs (ADR-014).

If the ethics application has already been approved describing a qualitative study, this likely
requires an amendment rather than a justifying paragraph in the methodology chapter. If it has not
yet been submitted, the telemetry and the code capture must be described in it from the start.

This is the longest-lead item in the project and it depends on no code. Confirm with Joe Yuen and
the ethics committee before Phase 3 lands, since Phase 3 is the point at which the software starts
recording.

---

## Resolution order

Decisions that block code, in the order they block it:

ADR-001 (step granularity) → ADR-002 (NodeId and diff) → ADR-007 (language spec) →
ADR-010 (step shape) → ADR-006 (power economy) → ADR-003 (oracle) → ADR-004 (WorldState) →
ADR-005 (anchors) → ADR-012 (Make) → the rest.

ADR-019 runs in parallel and is not gated on any of it.

---

## ADR-020 — Running unchanged source is not a thing

**Decided.** Closes the gap ADR-006 left open: whether one commitment unlocks unlimited discounted
runs.

Neither. The question dissolves, because execution is deterministic.

1. **Re-running unchanged source is free and produces no new trace.** The source has not changed, so
   the trace cannot have changed. The player scrubs the trace they already have. The run button is
   disabled with the reason stated: nothing has changed, so nothing new will happen.
2. **A run whose source differs from the last run requires a fresh commitment on every prediction
   anchored to a changed statement.** Predictions anchored to untouched statements stay satisfied.
   The anchor machinery from ADR-005 already identifies which is which.
3. **Make levels are exempt.** A level with `EditPolicy: free` and `goal: tests` carries no
   predictions, charges a flat `runCost`, and gets a generous budget.

**Considered:** persistent commitment with unlimited discounted runs, which permits brute-forcing;
and one commitment per run, which starves the Modify stage on levels with few predictions.

**Why this instead.** Both alternatives assumed that repeatedly running the same program is a thing
a player might do. It is not, because the machine is deterministic and the output is already sitting
in the trace. Removing the affordance removes the entire problem class rather than balancing against
it, and it teaches determinism — which is itself one of the notional-machine properties novices most
often fail to hold.

It also gives Phase 5's acceptance criterion a stronger form. The old criterion was that a level
cannot be brute-forced by running repeatedly. The new one is that repeated running is not
expressible: the button is disabled and says why.

**Why Make is exempt.** PRIMM's Make stage has no Predict step. Gating original authorship behind
predictions would be implementing the framework wrongly in order to be consistent with a mechanic
the framework does not apply at that stage.

---

## ADR-021 — The pedagogical cap counts statement executions, not steps

**Decided.** Two caps, measured in different units.

- **Safety cap:** 50,000 raw steps. A project constant, not level-configurable. Runaway protection.
- **Pedagogical cap:** 2,000 **statement executions**, per-level configurable as
  `pedagogicalStatementCap`. Counted on `isStatementBoundary`.

Supersedes the step-counted pedagogical cap in ADR-008. The three-level scrubber LOD in ADR-008 is
unaffected.

**Why.** ADR-001 made steps a node-level unit, which costs roughly five to eight steps per
statement. A cap of 2,000 steps is therefore about 250 to 400 statement executions, and a 20×20
nested loop exceeds it — meaning Act 3 would have been raising the cap on nearly every level and the
default would have been noise rather than a budget.

Worse, a step-counted cap is coupled to expression granularity. Any future change to what counts as
a node silently retunes every level in the game. Statement executions are stable against that, and
they are the unit an author actually reasons in: "this level runs about three hundred statements" is
a sentence a level designer can hold in their head, and "this level produces about two thousand
steps" is not.

At the new default a 20×20 nested loop runs roughly 1,200 statement executions and fits comfortably.

---

## ADR-022 — Performance targets split by interaction frequency

**Decided.** Supersedes the single parse-and-trace target in `01-ARCHITECTURE.md`, which ADR-021
invalidated by changing the unit the cap is counted in.

Three targets, in the units of ADR-021:

| Interaction | Budget |
|---|---|
| Parse and trace a typical level (300 statement executions or fewer) | under 50 ms |
| Parse and trace a cap-sized level (2,000 statement executions) | under 400 ms, with a working indicator past 150 ms |
| Scrub step change to repaint | under 16 ms, at any trace size |

**Considered:** raising the single budget to cover a cap-sized level, which would have made the
typical case unmeasured; and declaring 50 ms a common-case target with no worst-case budget at all,
which leaves Act 3 unbounded.

**Why split.** The two costs have completely different interaction profiles and were only ever
sharing a number by accident. Tracing happens once per edit. Scrubbing happens continuously, at the
rate the player drags the ribbon, and it is the core mechanic. A budget should be tight where the
interaction is continuous and generous where it is one-off, so 16 ms on scrub is the
non-negotiable figure and the trace budgets are allowed to breathe.

The 400 ms figure assumes the naive full-snapshot trace representation. If it is missed, that is the
profiling signal ADR-009 anticipated, and the response is to switch to keyframes plus deltas behind
the existing accessor — not to loosen the budget.

ADR-023 — Building is complete; the remaining time is for the report

Decided. Resolves KI-64. The artefact is frozen at commit 2575935. PHASES.md is closed and its run protocol is historical.

Priority to submission: (1) a manual browser pass closing KI-23, 36, 39, 45, 60 and 62; (2) the ADR-019 ethics position; (3) KI-44 instrument verification; (4) the report. KI-43 and KI-53 defer to after submission — the report quotes level prose only as appendix material, so voice gates the October demo rather than the dissertation.

The freeze covers features, not defects. Bugs surfaced by verification may be fixed, because verification that cannot act on what it finds is not verification. Every such fix is logged as a resolved KI entry. New capability, new dependencies and new content are out of scope until submission.

Considered: continuing to Phase 11-style work, and freezing absolutely with defects recorded but unfixed.

Why. Phases 0–10 completed sequentially because the run protocol asked for exactly that, and by Phase 10 the binding constraint had stopped being code. Building further would have produced an artefact nobody could write up. Freezing absolutely would have meant shipping a known crash into a demo to preserve a rule whose purpose was to stop scope growth, not to stop repair.

The fair viva question is whether Phase 10 should have been built at all before the report. Honestly: no. The presentation-layer isometric view is the one phase that could have waited, and the eleven days it did not consume would have been useful. It was built because the protocol was sequential and nothing in it said stop. The cost was low and the phase is real work, but the sequencing was wrong and the correct answer is to say so.

ADR-024 — A browser harness is approved for measurement

Decided. playwright and axe-core are approved as devDependencies, notwithstanding ADR-023's freeze and CLAUDE.md's dependency rule. Resolves the ruling KI-23 has been waiting on since Phase 3.

Scope is measurement and verification only. No product code changes beyond the defect fixes ADR-023 already permits. Nothing in src/ may import either package.

Why. ADR-023 permits verification, and verification that cannot observe the rendered page is not verification of anything the player experiences. Without it, chapter 5 reports UNMEASURED against every ADR-022 target and chapter 9 concedes that no accessibility claim was ever checked against a rendered DOM. Both are avoidable with two devDependencies.

axe-core additionally closes KI-61 properly. That entry records the honest weakness of contrast.ts: it audits a hand-written list of colour pairs rather than the page, and it produced a false positive on a pair the CSS never draws. Auditing the rendered DOM removes the class of error rather than patching the instance.

What it does not close. Automated checks cannot substitute for a person. KI-62's screen-reader claim, KI-42's playtest, and KI-43/53's voice reading all remain open and all still need a human. The harness reduces the chance the manual pass is spent discovering crashes instead of judging the experience.
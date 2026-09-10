# The interpreter port

Phase 1 ported the reference interpreter to C++ under `Source/GhostInTheStack/Interpreter/`.
The thirty golden fixtures in `Fixtures/` are the spec; the port reproduces every one byte for
byte, checked by the `GhostInTheStack.Interpreter.Golden.*` automation tests. This note records
what the port is, how to run its tests, and where it knowingly differs from the reference.

## Layout

| File | Holds |
|---|---|
| `GitsTypes.h`, `GitsValue.cpp` | positions and spans, `FGitsValue` (int, float, str, bool, None, list, function), the world record, effects and the reducer, queries, steps, traces, the caps |
| `GitsDiagnostics.h/.cpp` | the 46-code catalogue, each with a summary, a what and a check, in the not-here and not-yet registers |
| `GitsLexer.h/.cpp` | tokens with spans, INDENT/DEDENT/NEWLINE, comments preserved, lexical diagnostics |
| `GitsAst.h/.cpp` | the node struct, structural NodeIds (`body.3.test.left`), the unparser trace labels use |
| `GitsParser.h/.cpp` | recursive descent, the tier gate, the method whitelist, the twenty recognition cases, silent recovery |
| `GitsEvaluator.h/.cpp` | the evaluator: one step per node, frames, builtins, station effects, the oracle, both caps |
| `GitsTrace.h/.cpp` | accessors, replay oracle, observable-state diff, the golden serialiser |
| `Tests/` | 45 automation tests: golden traces, parser and recognition, diagnostics, lexer, evaluator, caps, oracle, diff, tier 4, values |

Everything is plain C++ structs with a `Gits` prefix; nothing is a UObject yet. Phase 2 wraps
what the world needs in reflected types.

## Running the tests

Headless, without opening the editor:

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\ghost-in-the-stack-ue\GhostInTheStack\GhostInTheStack.uproject" -ExecCmds="Automation RunTests GhostInTheStack.Interpreter; Quit" -unattended -nopause -nullrhi -NoSound -nosplash -log=GitsTests.log
```

`Tools/run_interpreter_tests.cmd` wraps that and prints the pass and fail counts from
`Saved/Logs/GitsTests.log`. In the editor, the same tests run from Session Frontend or through the
MCP `AutomationTestToolset` (discover, list with the `GhostInTheStack.Interpreter` filter, run,
read results). A failing golden test writes what it produced to `Saved/Tests/Golden/<name>.actual.txt`
for diffing against `Fixtures/<name>.trace.txt`.

## What the fixtures could not tell the port

Three inputs the trace files do not carry were taken from the reference test harness and live in
`Tests/GitsGoldenTests.cpp`: the `-- covers:` line of each fixture, the per-fixture statement cap
(40 for `outcome-statement-cap`, 4000 for `outcome-call-depth`), and the fixture oracle (the
cold store reads 4 plus the clock; anything else reads the clock). The exact serialisation rules
that the fixtures never exercise (several extras on one step join with a single space; the
statement cap fires as soon as the count reaches it) were read from the reference serialiser.
Which diagnostic code each malformed input gets, and the recovery behaviour, come from the
reference acceptance tests. The evaluator's behaviour itself was written from the fixtures and
`LANGUAGE-SPEC.md`.

## Known differences from the reference

- **Eager, not a generator.** The reference yields steps lazily; the port evaluates recursively
  and appends steps to the trace as it goes. The trace is identical and the caps bound the work,
  and a recorded trace is what the game consumes (rewind is index stepping). A generator can be
  reintroduced behind the same `FGitsTrace` if a live-stepping view ever needs it.
- **No exceptions.** Unreal builds without C++ exceptions. Runtime errors set the outcome and
  unwind through status returns; the observable result is the same.
- **Integers are 64-bit.** Python's are unbounded. `2 ** 70` wraps here. No level content goes
  near that, and it is the one place the transferability claim has a footnote. Say if an ADR is
  wanted; the fix is a bignum, and it is not small.
- **Float formatting follows Python, not JavaScript.** `1e16` prints as `1e+16`, infinity as `inf`.
  The reference printed JavaScript's `Infinity`. Every fixture value is unaffected.
- **A few conveniences Python has and the spec does not mention** are allowed rather than refused:
  `in` on strings, `+` and `*` on lists, iterating a string. None produce a recognition message
  because none is on the excluded list.
- **`range` outside a `for` header** is a runtime error with code `bad-range`, as the reference
  chose. Whether `range` is shadowed by a user function is checked before treating the header
  specially, so `def range` still works.

## Dependencies on the reference repository

None at runtime. The fixtures and the language spec are copied into this repo and are the only
inputs the tests read. The reference stays the place where any interpreter behaviour question is
settled, and any change to a fixture is a language-spec decision, not a port decision.

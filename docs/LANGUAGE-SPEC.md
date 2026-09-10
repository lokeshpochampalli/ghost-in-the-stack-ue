# 02 — Language specification

A strict subset of Python 3. Anything valid here is valid Python with identical semantics. That
transferability is a selling point for the game and a claim the dissertation makes, so do not invent
syntax and do not deviate from Python's behaviour.

Features are gated by tier. A level declares the highest tier it uses; the parser refuses anything
above the player's unlocked tier with a friendly message rather than a syntax error.

## Tier 1 — Surface systems

**Literals** — `int`, `float`, `str` (single and double quoted, no f-strings yet), `bool`, `None`

**Variables** — assignment, reassignment, identifiers matching `[A-Za-z_][A-Za-z0-9_]*`

**Operators**
- Arithmetic: `+ - * / // % **`
- Comparison: `== != < > <= >=`
- Unary: `-x`, `not x`
- String concatenation with `+`, repetition with `*`

**Builtins** — `print()`, `len()`, `int()`, `float()`, `str()`, `bool()`

**Comments** — `#` to end of line. Comments are load-bearing narrative; the lexer must preserve them
with spans so the UI can style Ilse's notes distinctly.

## Tier 2 — Control

- `if` / `elif` / `else`
- Boolean operators `and`, `or`, `not`, with Python's short-circuit semantics
- Truthiness following Python: `0`, `0.0`, `""`, `None`, `False` are falsy

`[]` is also falsy in Python, but lists do not exist until Tier 3, so that case is specified there.

## Tier 3 — Repetition

- `while <expr>:`
- `for <name> in range(...)` — one, two and three argument forms. **`range` is legal only here.**
  Python's `range` is lazy and prints as `range(0, 3)`; ours would have to be a list and would print
  as `[0, 1, 2]`. Rather than ship that small lie, `range` outside a `for` header is a runtime error
- `for <name> in <list>`
- `break`, `continue`
- List literals, indexing (including negative indices), `len()`, `in`
- **Augmented assignment** — `+=`, `-=`, `*=`, `//=`. `/=`, `%=` and `**=` are recognised and
  refused (case 20), so they cannot be mistaken for a missing operator
- **Method calls** — `list.append(x)` and nothing else at this tier
- Nested loops
- Truthiness extends to `[]`, which is falsy

### Augmented assignment (ADR-007)

Permitted from Tier 3. Accumulator patterns are Act 3's core content and `total += x` is what real
Python looks like; teaching `total = total + x` exclusively would be teaching a dialect.

Do not introduce it before Tier 3. Act 1 and Act 2 predictions depend on the player reading
`x = x + 1` as an instruction rather than an equation — the `assignment-as-equality` misconception —
and the compound form hides exactly the thing being probed.

### Method calls and the method whitelist (ADR-007)

The grammar includes attribute access and method-call syntax. What is permitted is controlled by a
**per-tier whitelist**, not by the grammar.

| Tier | Permitted methods |
|---|---|
| 1 | none |
| 2 | none |
| 3 | `list.append(x)` |
| 4 | `list.append(x)` |

Calling a method that is not on the whitelist produces a recognition message naming the method, not a
parse error and not an attribute error.

The alternative — a builtin `append(list, item)` — is a smaller grammar, but it is not Python, and
transferability to real Python is a claim the dissertation makes. Breaking that claim to save grammar
work would undermine the artefact's stated contribution.

## Tier 4 — Abstraction

- `def name(params):` with positional parameters only
- `return`, including bare `return`
- Local scope: names bound in a function are not visible outside it
- Recursion, with a call-depth cap of 100 producing a friendly message rather than a stack overflow
- Nested calls

### The scope rule, in full

**Local, then module.** A name is looked up in the current frame first and in the module frame
second. Assignment always binds in the CURRENT frame, so `count = 99` inside a function leaves the
module's `count` alone — which is the `scope-leak` misconception, and the machine has to do it
truthfully for the trace to show it.

That is the whole rule, and it is one sentence on purpose. There is no `global` (recognition case 12)
and **a function defined inside another function is refused**: nested definitions need closures, and
closures need a second rule on top of this one. A player who meets closures later meets them in real
Python, having lost nothing.

`return` outside a function is refused with its own message rather than a generic one — it is a thing
beginners write, and "there is nothing here to hand it back to" is more use than a syntax error.

A function is a value: it can be bound to a name, and it prints as `<function name>`. Python appends
a machine address; ours does not, because an address would be machine noise in a trace that has to
replay identically.

## Explicitly excluded

Do not implement these. If a player types one, produce a message saying it is not part of this
station's system rather than a parse error. See the recognition pass below.

Classes, `import`, `try`/`except`, `with`, comprehensions, generators, `lambda`, decorators,
`global`/`nonlocal`, keyword and default arguments, `*args`/`**kwargs`, tuples, sets, dictionaries,
slicing, f-strings, `while...else`, `for...else`, multiple assignment targets, `is`, `pass`, `del`,
`assert`, `yield`, unary `+`, **a function defined inside another function**, and **all string
methods at every tier**.

Dictionaries and f-strings are the two most likely candidates for a later addition. Do not add them
before Act 4 ships.

## The recognition pass

Excluded syntax needs a **recogniser, not a rejecter**. This is real parser scope and it has its own
acceptance criteria in Phase 1.

You cannot say "dictionaries are not part of this station's system" without lexing `{` and parsing far
enough to know it is a dict rather than a set. So the parser gains an explicit recognition pass: when
the normal grammar fails, it attempts to match the construct against the enumerated excluded forms and
emits a named diagnostic from the `errors.ts` catalogue.

Every case below needs an implementation and an acceptance test asserting a **named** message rather
than a generic syntax error:

| # | Construct | Recognised by |
|---|---|---|
| 1 | Dict literal | `{` with a `:` before the matching `}` |
| 2 | Set literal | `{` without a `:` before the matching `}` |
| 3 | f-string | `f` or `F` immediately preceding a quote |
| 4 | `import` | leading `import` or `from` |
| 5 | `class` | leading `class` |
| 6 | `try` / `except` | leading `try`, `except`, `finally` or `raise` |
| 7 | `with` | leading `with` |
| 8 | `lambda` | `lambda` in expression position |
| 9 | Comprehension | `for` appearing inside `[`, `(` or `{` |
| 10 | Tuple | comma-separated expression list in value position, or parenthesised pair |
| 11 | Slicing | `:` inside a subscript |
| 12 | `global` / `nonlocal` | leading keyword |
| 13 | Keyword argument | `name=` in an argument list |
| 14 | Default argument | `=` in a parameter list |
| 15 | Comparison chaining | two comparison operators at the same precedence level |
| 16 | Above-tier construct | any construct valid in the spec but above the level's declared tier |
| 17 | Unwhitelisted method | method call whose name is not on the tier's whitelist |
| 18 | Tab indentation | a tab character in leading whitespace |
| 19 | Chained assignment | a second `=` after a completed assignment, as in `a = b = 1` |
| 20 | Other excluded keyword | `is`, `pass`, `del`, `assert`, `yield`, `as`, a decorator, `*`/`**` unpacking, two statements separated by `;`, or a unary `+` |

Comparison chaining deserves note. `1 < x < 10` is valid Python that we deliberately do not
implement. It is recognised and refused rather than implemented incorrectly, because silently
evaluating it as `(1 < x) < 10` would teach a falsehood.

Cases 16 and 17 are not "excluded syntax" — they are correct Python the player has not unlocked yet.
They use the same mechanism and a different message register: *not yet* rather than *not here*.

Cases 19 and 20 were added in Phase 1. They cover constructs already named in the excluded list above
but with no enumerated case of their own, and they exist for the same reason as the rest: without
them, `a = b = 1` and a stray `pass` fall through to a generic message, which is precisely the
outcome ADR-007 was written to prevent. Case 20 carries the name of what was found in its message.

**The tier gate applies to expressions as well as statements.** `and`, `or` and `not` arrive at Tier
2, so a Tier 1 level refuses them with case 16 even though they appear mid-expression rather than at
the start of a line. Comparison operators are Tier 1 and are never gated.

## Indentation

Four spaces. Tabs are rejected with a message explaining why (recognition case 18). Inconsistent
indentation is a diagnostic that names the expected level. The editor should render indentation
guides, since indentation errors are among the most common novice blockers.

## Station builtins

Level-specific functions injected into the global environment. They are declared in the level JSON so
the parser knows they exist.

Station builtins come in two kinds, and the distinction is architectural (ADR-003):

- **Effect builtins** emit an `Effect` and return `None`. The evaluator records the effect in the step
  and folds it into its running world.
- **Reading builtins** return a value obtained by calling the injected `WorldOracle` against the
  evaluator's folded world. The response is recorded in the step's `oracleRead` field, so replay and
  scrubbing never re-consult the oracle.

Neither kind mutates the world directly. Effects are applied by the world reducer.

Examples, extend as levels demand:

```
open_valve(id)          effect  — set a valve open
close_valve(id)         effect  — set a valve closed
set_heater(level)       effect  — 0-10
log(message)            effect  — writes to the station log, visible in the UI
wait(ticks)             effect  — advance the world clock
read_sensor(id) -> int  read    — current reading, deterministic per world state
```

`wait(ticks)` is an effect, not a read. It advances the folded world, so the next `read_sensor` sees
the updated state.

Phase 2 makes all six available on every run. Phase 4 declares them per level in the level JSON, so a
facility can withhold the ones it does not have.

## Semantics that must match Python exactly

These are the ones novices get wrong and therefore the ones our predictions probe, so they have to be
right.

- `/` is float division, `//` is floor division, and `//` floors toward negative infinity
- `%` follows Python's sign rules, not C's: `-7 % 3` is `2`
- `range(a, b)` excludes `b`
- Strings are immutable
- Assignment binds a name to a value; it does not create a link between names
- `list.append(x)` mutates in place and returns `None`
- Augmented assignment on a number rebinds the name; `x += 1` and `x = x + 1` are equivalent here
- Comparison chaining is **not** supported in this subset even though Python allows it — recognised
  and refused with a message rather than implemented wrongly
- `True` and `False` behave as `1` and `0` in arithmetic, as they do in Python
- `int()` truncates toward zero, so `int(-3.9)` is `-3`, not `-4`
- `and` and `or` return one of their operands, not a boolean: `0 or "fallback"` is `"fallback"`
- A negative index counts from the end: `readings[-1]` is the last item

Write a golden trace fixture for every bullet above.

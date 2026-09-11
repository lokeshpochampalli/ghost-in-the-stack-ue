"""Curriculum coverage over the four sector content scripts, offline.

Phase 9's acceptance: every concept tag covered by at least two systems, every misconception tag
choreographed (a distractor with that tag) in at least two. Reads Tools/setup_sector*_content.py
with the same tag lists as Companion/GitsTags.cpp. Exit code 1 if anything is short.

    python Tools/check_curriculum.py
"""
import re, sys, os, glob

CONCEPTS = ["variable-assignment", "reassignment", "data-types", "type-coercion", "arithmetic", "string-ops", "output",
            "conditional", "elif-chain", "boolean-logic", "comparison", "truthiness", "while-loop", "for-loop", "range",
            "accumulator", "nested-loop", "augmented-assignment", "list-literal", "indexing", "list-mutation",
            "function-def", "parameters", "return-value", "local-scope", "call-stack", "recursion"]
MISCONCEPTIONS = ["assignment-as-equality", "parallel-assignment", "sequence-ignored", "type-confusion", "branch-both",
                  "inverted-comparison", "operator-confusion", "loop-runs-once", "fencepost", "accumulator-reset",
                  "index-from-one", "scope-leak", "return-vs-print", "arg-param-identity", "recursion-no-return"]

here = os.path.dirname(os.path.abspath(__file__))
concept_systems = {c: [] for c in CONCEPTS}
misc_systems = {m: [] for m in MISCONCEPTIONS}
unknown = []
systems = 0
for path in sorted(glob.glob(os.path.join(here, "setup_sector*_content.py"))):
    text = open(path, encoding="utf-8").read()
    # each system: ensure_script("NAME", ... concepts=[...] ... ) up to the next ensure_script
    chunks = re.split(r'(?=ensure_script\(")', text)
    for chunk in chunks:
        m = re.match(r'ensure_script\("([^"]+)"', chunk)
        if not m:
            continue
        name = m.group(1)
        systems += 1
        cm = re.search(r"concepts=\[([^\]]*)\]", chunk)
        for c in re.findall(r'"([^"]+)"', cm.group(1) if cm else ""):
            (concept_systems.get(c) if c in concept_systems else unknown).append((name, c)) if c not in concept_systems else concept_systems[c].append(name)
        for o in re.findall(r'option\("[a-z]", "[^"]*", "([a-z-]+)"', chunk):
            if o in misc_systems:
                if name not in misc_systems[o]:
                    misc_systems[o].append(name)
            else:
                unknown.append((name, o))

short = []
print(f"{systems} systems\n")
print("CONCEPTS")
for c in CONCEPTS:
    n = len(concept_systems[c])
    flag = "" if n >= 2 else "   <-- short"
    if n < 2: short.append(("concept", c, n))
    print(f"  {c:24s} {n:2d}  {', '.join(concept_systems[c])}{flag}")
print("\nMISCONCEPTIONS")
for m in MISCONCEPTIONS:
    n = len(misc_systems[m])
    flag = "" if n >= 2 else "   <-- short"
    if n < 2: short.append(("misconception", m, n))
    print(f"  {m:24s} {n:2d}  {', '.join(misc_systems[m])}{flag}")
if unknown:
    print("\nUNKNOWN TAGS:", unknown)
if short or unknown:
    print("\nSHORT:", short)
    sys.exit(1)
print("\nevery concept in two or more systems; every misconception choreographed in two or more.")

#!/usr/bin/env python3
"""Sanity checks on a codegen output folder (default: generated/). Run after every codegen.

Fails (exit 1) when any of these appear - each one has caused a real WWE 2K14 bug:
  1. "branch ... outside function": a function boundary entry in config/functions.toml split a
     function, and codegen turned a conditional branch (e.g. a bdzf switch case) into a bare
     `return`. This silently broke the legal-screen state machine (game stuck forever).
  2. goto to a label that is not in the same function (does not compile).
  3. compiler register save/restore helpers (__savegprlr_*, __restgprlr_*, __savefpr_*, __restfpr_*)
     that lost their special name because a boundary entry split them (corrupts registers).

Usage: tools/check-generated.py [generated_dir]
"""
import glob
import re
import sys

out = sys.argv[1] if len(sys.argv) > 1 else "generated"
files = sorted(glob.glob(f"{out}/*_recomp.*.cpp"))
if not files:
    sys.exit(f"no *_recomp.*.cpp files in {out}/ - run codegen first")

outside, bad_goto = [], set()
for path in files:
    text = open(path, encoding="utf-8", errors="replace").read()
    for body in re.split(r"(?=PPC_FUNC_IMPL\(__imp__)", text)[1:]:
        name = re.match(r"PPC_FUNC_IMPL\(__imp__(\w+)\)", body).group(1)
        for target in re.findall(r"branch to 0x([0-9A-F]+) outside function", body):
            outside.append((name, target))
        labels = set(re.findall(r"^loc_([0-9A-F]{8}):", body, re.M))
        for target in set(re.findall(r"goto loc_([0-9A-F]{8})", body)):
            if target not in labels:
                bad_goto.add((name, target))

init = "".join(open(p, encoding="utf-8").read() for p in glob.glob(f"{out}/*_init.cpp"))
helpers = set(re.findall(r"__(?:save|rest)(?:gprlr|fpr|vmx)_\d+", init))

print(f"functions files: {len(files)}")
print(f"branches turned into returns ('outside function'): {len(outside)} {outside[:8]}")
print(f"gotos to labels outside their function: {len(bad_goto)} {sorted(bad_goto)[:8]}")
print(f"register save/restore helpers found by name: {len(helpers)} (expected > 0)")
sys.exit(1 if outside or bad_goto or not helpers else 0)

#!/usr/bin/env python3
import hashlib
import sys
from pathlib import Path

if len(sys.argv) != 3:
    raise SystemExit(f"usage: {sys.argv[0]} INPUT_IMAGE OUTPUT_IMAGE")

source, target = map(Path, sys.argv[1:])
if target.exists():
    raise SystemExit(f"refusing to overwrite: {target}")
data = source.read_bytes()
old = b"release@w200ds"
new = b"v0.2ga1@w200ds"
expected_input = "4bf47964827f138c6941b0e0c90f8500763f3b7b9b2ac60e52770c9590152ff7"
expected_output = "19532e425e2893cdd983df069db48ec90699ace4c065cf745af3a56ca3ca90b7"
if hashlib.sha256(data).hexdigest() != expected_input:
    raise SystemExit("unexpected post-linked Image SHA-256")
if len(old) != len(new) or data.count(old) != 2 or data.count(new):
    raise SystemExit("unexpected build banner layout")
marked = data.replace(old, new)
if hashlib.sha256(marked).hexdigest() != expected_output:
    raise SystemExit("marked Image SHA-256 mismatch")
target.write_bytes(marked)
print(f"MARKED_IMAGE={target}")
print(f"MARKED_SHA256={expected_output}")


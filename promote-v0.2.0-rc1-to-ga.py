#!/usr/bin/env python3
import hashlib
import sys
from pathlib import Path

if len(sys.argv) != 3:
    raise SystemExit(f"usage: {sys.argv[0]} RC1_IMAGE GA_IMAGE")

source, target = map(Path, sys.argv[1:])
if target.exists():
    raise SystemExit(f"refusing to overwrite: {target}")

rc1 = b"v0.2rc1@w200ds"
base = b"release@w200ds"
ga = b"v0.2ga1@w200ds"
expected_rc1 = "cea4afc177e56e93c8fbc7ed794767595b6c0d826e7e40b2e2f99d514b3cebfb"
expected_base = "4bf47964827f138c6941b0e0c90f8500763f3b7b9b2ac60e52770c9590152ff7"
expected_ga = "19532e425e2893cdd983df069db48ec90699ace4c065cf745af3a56ca3ca90b7"

data = source.read_bytes()
if len(data) != 39854288 or hashlib.sha256(data).hexdigest() != expected_rc1:
    raise SystemExit("unexpected v0.2.0-rc1 Image identity")
if len(rc1) != len(base) or len(base) != len(ga):
    raise SystemExit("release markers are not equal length")
if data.count(rc1) != 2 or data.count(base) or data.count(ga):
    raise SystemExit("unexpected RC1 marker layout")

restored = data.replace(rc1, base)
if hashlib.sha256(restored).hexdigest() != expected_base:
    raise SystemExit("RC1 reverse promotion did not recover the accepted post-linked Image")

marked = restored.replace(base, ga)
diffs = [index for index, pair in enumerate(zip(data, marked)) if pair[0] != pair[1]]
if len(marked) != len(data) or len(diffs) != 4:
    raise SystemExit("GA Image differs from RC1 outside the four marker bytes")
if marked.count(ga) != 2 or marked.count(rc1) or marked.count(base):
    raise SystemExit("unexpected GA marker layout")
if hashlib.sha256(marked).hexdigest() != expected_ga:
    raise SystemExit("GA Image SHA-256 mismatch")

target.write_bytes(marked)
print(f"GA_IMAGE={target}")
print(f"GA_IMAGE_SHA256={expected_ga}")
print("RC1_TO_GA_DIFF_BYTES=4")
print("RC1_TO_GA_DIFF_OFFSETS=" + ",".join(map(str, diffs)))

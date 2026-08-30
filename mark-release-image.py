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
old = b"kerneldev@TanukiDesktop"
new = b"k11f0@r2c@TanukiDesktop"
expected_input = "63d853aa9ee85e339ccd2c4c7fac2435888eb45b0c5f6d141477bec8bb0caae3"
expected_output = "244e6dc3efab05ecbfc3de6ded31714a0cb87e2a7c7e02bcfaff1f1603f17e06"
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


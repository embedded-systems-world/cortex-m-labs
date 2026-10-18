#!/usr/bin/env python3
"""Build-independent QEMU smoke test for real Cortex-M exception behavior."""
import os
import subprocess
import sys
from pathlib import Path

base = Path(__file__).resolve().parent
qemu = os.environ.get("QEMU", "qemu-system-arm")
for name, expected in (("preempt", "MABbam"), ("equal", "MAaBbm")):
    cmd = [
        qemu, "-M", "lm3s6965evb", "-nographic", "-monitor", "none",
        "-semihosting-config", "enable=on,target=native",
        "-kernel", str(base / "build" / f"{name}.elf"),
    ]
    try:
        run = subprocess.run(cmd, capture_output=True, text=True, timeout=20)
    except subprocess.TimeoutExpired:
        print(f"FAIL {name}: QEMU did not exit (check semihosting support)")
        sys.exit(1)
    output = run.stdout + run.stderr
    print(f"----- {name} -----\n{output.strip()}")
    if run.returncode != 0 or f"Trace: {expected}" not in output or "PASS:" not in output:
        print(f"FAIL {name}: expected Trace: {expected}; exit={run.returncode}")
        sys.exit(1)
print("PASS: both Cortex-M3 interrupt ordering experiments completed in QEMU")

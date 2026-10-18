#!/usr/bin/env python3
"""Run both QEMU exception scenarios and verify observed state."""
from pathlib import Path
import os
import re
import subprocess

base = Path(__file__).resolve().parent
qemu = os.environ.get("QEMU", "qemu-system-arm")
experiments = (
    ("tail", "MTtUum", "YES"),
    ("deferred", "MTtmUun", "NO"),
)
for name, expected_trace, pc_relation in experiments:
    cmd = [
        qemu, "-M", "lm3s6965evb", "-nographic", "-monitor", "none",
        "-semihosting-config", "enable=on,target=native",
        "-kernel", str(base / "build" / f"{name}.elf"),
    ]
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=20)
    except subprocess.TimeoutExpired:
        raise SystemExit(f"FAIL {name}: QEMU timed out")
    output = result.stdout + result.stderr
    print(f"---- {name} ----\n{output.strip()}")
    for fragment in (
        f"Trace: {expected_trace}",
        "Timer EXC_RETURN: 0xFFFFFFF9",
        "UART  EXC_RETURN: 0xFFFFFFF9",
        f"Stacked PC identical: {pc_relation}",
        "PASS: ordering and exception-state checks",
    ):
        if fragment not in output:
            raise SystemExit(f"FAIL {name}: missing {fragment!r}")
    pcs = re.findall(r"(?:Timer|UART)\s+stacked PC: 0x([0-9A-F]{8})", output)
    if len(pcs) != 2 or (pcs[0] == pcs[1]) != (pc_relation == "YES"):
        raise SystemExit(f"FAIL {name}: saved PC snapshots inconsistent")
    if result.returncode != 0:
        raise SystemExit(f"FAIL {name}: exit code {result.returncode}")
print("PASS: both Cortex-M3 experiments matched their expected outcomes")

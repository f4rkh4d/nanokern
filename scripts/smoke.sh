#!/usr/bin/env bash
# boots the kernel headless in qemu for a few seconds. captures two independent proofs:
#   1. vga memory at 0xb8000 contains the banner. = long mode came up.
#   2. debugcon stream contains roughly equal counts of A, B, C from the demo tasks.
#      = the preemptive scheduler is rotating fairly across all three.
# exits nonzero on any failure.

set -euo pipefail

here="$(cd "$(dirname "$0")/.." && pwd)"
elf="$here/build/nanokern.elf"
[ -f "$elf" ] || { echo "missing $elf, run make first" >&2; exit 2; }

mon="$(mktemp -u /tmp/nanokern-mon.XXXXXX)"
vga="$(mktemp -u /tmp/nanokern-vga.XXXXXX)"
log="$(mktemp -u /tmp/nanokern-log.XXXXXX)"
trap 'rm -f "$mon" "$vga" "$log"' EXIT

qemu-system-x86_64 -kernel "$elf" -display none -m 128M -no-reboot \
    -monitor "unix:$mon,server,nowait" -debugcon "file:$log" -serial null &
qpid=$!
sleep 4

{
    printf 'pmemsave 0xb8000 4000 "%s"\n' "$vga"
    sleep 0.5
    printf 'quit\n'
} | nc -U "$mon" > /dev/null 2>&1 || true

sleep 0.5
kill "$qpid" 2>/dev/null || true
wait "$qpid" 2>/dev/null || true

[ -s "$vga" ] || { echo "no vga dump, kernel never wrote to 0xb8000" >&2; exit 1; }
[ -s "$log" ] || { echo "no debugcon output, kernel never printed" >&2; exit 1; }

python3 - "$vga" "$log" <<'PY'
import sys
vga_path, log_path = sys.argv[1], sys.argv[2]

# 1. vga banner check
data = open(vga_path, "rb").read()
got_banner = False
for r in range(25):
    line = "".join(chr(data[r*160 + c*2]) if 32 <= data[r*160 + c*2] < 127 else " "
                   for c in range(80)).rstrip()
    if line:
        print(f"{r:2d}| {line}")
    if "nanokern" in line:
        got_banner = True
if not got_banner:
    sys.exit("missing banner, boot probably failed")

# 2. scheduler fairness check
log = open(log_path, "rb").read().decode("ascii", "ignore")
a, b, c = log.count("A"), log.count("B"), log.count("C")
print(f"[debugcon] A={a} B={b} C={c}  total={a+b+c}")
if min(a, b, c) < 30:
    sys.exit(f"scheduler unfair or stalled: A={a} B={b} C={c}")
spread = max(a, b, c) - min(a, b, c)
if spread > 10:
    sys.exit(f"scheduler unfair: spread {spread} too large for round-robin")
print("[ok] nanokern booted, vga live, scheduler fair across 3 tasks")
PY

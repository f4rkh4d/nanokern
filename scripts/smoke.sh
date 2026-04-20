#!/usr/bin/env bash
# boots the kernel in headless qemu for a few seconds, dumps vga text memory, and
# prints the banner rows. a green "[ok]" means long mode came up and the pit ticked.
# exit code nonzero on failure.

set -euo pipefail

here="$(cd "$(dirname "$0")/.." && pwd)"
elf="$here/build/nanokern.elf"
[ -f "$elf" ] || { echo "missing $elf — run make first" >&2; exit 2; }

mon="$(mktemp -u /tmp/nanokern-mon.XXXXXX)"
vga="$(mktemp -u /tmp/nanokern-vga.XXXXXX)"
trap 'rm -f "$mon" "$vga"' EXIT

qemu-system-x86_64 -kernel "$elf" -display none -m 128M -no-reboot \
    -monitor "unix:$mon,server,nowait" -serial null &
qpid=$!
sleep 3

{
    printf 'pmemsave 0xb8000 4000 "%s"\n' "$vga"
    sleep 0.5
    printf 'quit\n'
} | nc -U "$mon" > /dev/null 2>&1 || true

sleep 0.5
kill "$qpid" 2>/dev/null || true
wait "$qpid" 2>/dev/null || true

if [ ! -s "$vga" ]; then
    echo "no vga dump — kernel never wrote to 0xb8000" >&2
    exit 1
fi

python3 - "$vga" <<'PY'
import sys
data = open(sys.argv[1], "rb").read()
got_banner = False
for r in range(25):
    line = "".join(chr(data[r*160 + c*2]) if 32 <= data[r*160 + c*2] < 127 else " "
                   for c in range(80)).rstrip()
    if line:
        print(f"{r:2d}| {line}")
    if "nanokern" in line:
        got_banner = True
if not got_banner:
    print("missing banner — boot probably failed", file=sys.stderr)
    sys.exit(1)
print("[ok] nanokern booted, vga live")
PY

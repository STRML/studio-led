#!/bin/bash
# Which SMC key drives the front LED? Run as root on the Mac and watch the LED:
#
#   sudo ./led-probe.sh
#
# Each step writes one LS* key, holds 5 s, and restores the value it read first.
# The trap restores every key on exit or Ctrl-C.
set -u
cd "$(dirname "$0")"
[ "$(id -u)" = 0 ] || { echo "run with sudo"; exit 1; }

read_key() { ./smcdump "$1" | awk -v k="$1" '$1 == k { print $NF }'; }

KEYS="LSOF LSLN LSLB LSLF LS0C LS0P LS0S LS1C LS1P LS1S"
ORIG=$(mktemp)
for k in $KEYS; do echo "$k $(read_key "$k")" >> "$ORIG"; done
orig() { awk -v k="$1" '$1 == k { print $2 }' "$ORIG"; }
restore() { for k in $KEYS; do ./smcwrite "$k" "$(orig "$k")" > /dev/null; done; echo "restored all keys"; }
trap restore EXIT

step() {  # key value note
  echo; echo ">>> $1 = $2  ($3), holding 5 s. Watch the LED."
  ./smcwrite "$1" "$2"
  sleep 5
  ./smcwrite "$1" "$(orig "$1")" > /dev/null
  sleep 2
}

step LSOF 01 "flag, maybe LED off"
step LSLN 0000 "level, maybe normal brightness"
step LSLB 0000 "level"
step LSLF ffff "level"
step LS0C 0000 "channel 0"
step LS0P 0000 "channel 0"
step LS0S 0000 "channel 0"
step LS1C 0000 "channel 1"
step LS1P ffff "channel 1"
step LS1S 0000 "channel 1"

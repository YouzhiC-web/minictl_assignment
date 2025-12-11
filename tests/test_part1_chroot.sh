#!/usr/bin/env bash
set -euo pipefail

echo "=== Part 1 Test: chroot sandbox ==="

: "${ROOTFS:?Please set ROOTFS to the root filesystem path}"
: "${MINICTL:?Please set MINICTL to the path of the minictl binary}"

if [ ! -x "$MINICTL" ]; then
  echo "ERROR: $MINICTL not found or not executable"
  exit 1
fi

if [ ! -d "$ROOTFS" ]; then
  echo "ERROR: ROOTFS directory '$ROOTFS' does not exist"
  exit 1
fi

echo "-- Running: minictl chroot ROOTFS /bin/sh -c 'pwd; ls /'"
set -x
OUT=$("$MINICTL" chroot "$ROOTFS" /bin/sh -c 'pwd; ls /')
set +x

echo
echo "Output from container:"
echo "----------------------"
echo "$OUT"
echo "----------------------"

# Basic sanity checks:
#  - pwd should be "/"
#  - ls / should not match host root exactly (manual check)
if echo "$OUT" | head -n1 | grep -qx "/"; then
  echo "PASS: Inside chroot, pwd is '/'"
else
  echo "FAIL: Expected 'pwd' to print '/', got:"
  echo "$OUT" | head -n1
  exit 1
fi

echo "NOTE: Please manually verify that the 'ls /' output corresponds to your ROOTFS."
echo "Part 1 basic chroot test completed."

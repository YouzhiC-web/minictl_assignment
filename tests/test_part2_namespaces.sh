#!/usr/bin/env bash
set -euo pipefail

echo "=== Part 2 Test: namespaces (UTS, PID, MOUNT, USER) ==="

: "${ROOTFS:?Please set ROOTFS to the root filesystem path}"
: "${MINICTL:?Please set MINICTL to the path of the minictl binary}"

if [ ! -x "$MINICTL" ]; then
  echo "ERROR: $MINICTL not found or not executable"
  exit 1
fi

HOSTNAME_TEST="studentbox-$(date +%s)"

echo
echo "-- Test 1: hostname isolation (UTS namespace)"
set -x
OUT_HOST=$("$MINICTL" run --hostname="$HOSTNAME_TEST" "$ROOTFS" /bin/hostname)
set +x

echo "hostname inside container: '$OUT_HOST'"
if [ "$OUT_HOST" = "$HOSTNAME_TEST" ]; then
  echo "PASS: Hostname inside container matches requested name"
else
  echo "FAIL: Expected hostname '$HOSTNAME_TEST', got '$OUT_HOST'"
  exit 1
fi

echo
echo "-- Test 2: PID namespace (process runs as PID 1)"

CMD='echo "$$"'
set -x
OUT_PID=$("$MINICTL" run --hostname="$HOSTNAME_TEST" "$ROOTFS" /bin/sh -c "$CMD")
set +x

echo "PID reported inside container: '$OUT_PID'"
if [ "$OUT_PID" = "1" ]; then
  echo "PASS: Inside container, shell PID is 1"
else
  echo "FAIL: Expected PID 1 inside container, got '$OUT_PID'"
  exit 1
fi

echo
echo "-- Test 3: User namespace (rootless uid mapping)"

set -x
OUT_ID=$("$MINICTL" run --hostname="$HOSTNAME_TEST" "$ROOTFS" /usr/bin/id 2>/dev/null || true)
set +x

if [ -z "$OUT_ID" ]; then
  echo "WARNING: 'id' command not found inside ROOTFS or failed to run."
  echo "Please verify manually that 'id' shows uid=0 gid=0 when available."
else
  echo "id inside container: $OUT_ID"
  if echo "$OUT_ID" | grep -q 'uid=0'; then
    echo "PASS: Inside container, uid appears as 0 (root)"
  else
    echo "FAIL: Expected uid=0 inside container, got:"
    echo "$OUT_ID"
    exit 1
  fi
fi

echo
echo "NOTE: On the host, run 'ps aux | grep minictl' to verify the process runs as your normal user."
echo "Part 2 basic namespace tests completed."

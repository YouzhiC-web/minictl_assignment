#!/usr/bin/env bash
set -euo pipefail

echo "=== Part 3 Test: cgroup CPU and memory limits ==="

: "${ROOTFS:?Please set ROOTFS to the root filesystem path}"
: "${MINICTL:?Please set MINICTL to the path of the minictl binary}"

if [ ! -x "$MINICTL" ]; then
  echo "ERROR: $MINICTL not found or not executable"
  exit 1
fi

# We assume cpu_hog and mem_hog are compiled and placed inside ROOTFS/usr/local/bin
CPU_HOG=/usr/local/bin/cpu_hog
MEM_HOG=/usr/local/bin/mem_hog

echo
echo "Make sure you have copied 'cpu_hog' and 'mem_hog' into:"
echo "  $ROOTFS$CPU_HOG"
echo "  $ROOTFS$MEM_HOG"
echo

if [ ! -x "$ROOTFS$CPU_HOG" ] || [ ! -x "$ROOTFS$MEM_HOG" ]; then
  echo "ERROR: cpu_hog or mem_hog not found inside ROOTFS."
  echo "Compile them and copy into the rootfs before running this test."
  exit 1
fi

echo
echo "-- Test 1: Memory limit (mem_hog should be killed or fail allocation)"

set -x
$MINICTL run --mem-limit=64M "$ROOTFS" "$MEM_HOG" || true
set +x

echo "If your memory cgroup is working, mem_hog should stop after reaching ~64 MB."

echo
echo "-- Test 2: CPU limit (cpu_hog under 10% of a CPU)"

echo "This is a qualitative test. We run cpu_hog with a low CPU limit."
echo "Observe 'top' or 'htop' in another terminal."

set -x
$MINICTL run --cpu-limit=10 "$ROOTFS" "$CPU_HOG" &
PID=$!
set +x

echo "Let cpu_hog run for ~10 seconds"
echo "In another shell, check that its CPU usage is significantly below 100%."
echo "When you're done observing CPU, run: pkill -f cpu_hog"

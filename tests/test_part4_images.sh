#!/usr/bin/env bash
set -euo pipefail

echo "=== Part 4 Test: run-image ==="

: "${MINICTL:?Please set MINICTL to the path of the minictl binary}"

if [ ! -x "$MINICTL" ]; then
  echo "ERROR: $MINICTL not found or not executable"
  exit 1
fi

IMAGE_NAME=${1:-demo}

echo
echo "This test assumes you have created an image under:"
echo "  images/$IMAGE_NAME/rootfs/"
echo "  images/$IMAGE_NAME/config.txt"
echo
echo "Example config.txt:"
echo "  entrypoint=/bin/sh"
echo "  args=hello-from-image"
echo

echo "-- Running: minictl run-image $IMAGE_NAME"
set -x
$MINICTL run-image "$IMAGE_NAME"
set +x

echo
echo "If implemented correctly, the above command should execute the entrypoint"
echo "and arguments defined in images/$IMAGE_NAME/config.txt inside a container."

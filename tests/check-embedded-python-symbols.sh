#!/bin/sh
# D-03 symbol gate: the destinyEmbedded archive must import no CPython
# symbols beyond the pinned interop whitelist (see
# docs/destiny-python-seam.md for the provenance of each entry).
#
# usage: check-embedded-python-symbols.sh <archive> <whitelist>

set -eu

ARCHIVE="$1"
WHITELIST="$2"

ACTUAL="$(mktemp)"
EXPECTED="$(mktemp)"
trap 'rm -f "$ACTUAL" "$EXPECTED"' EXIT

nm -u "$ARCHIVE" | awk '{print $NF}' | grep -E '^__?Py' | grep -v '^__Z' | sort -u > "$ACTUAL"
grep -v '^#' "$WHITELIST" | grep -v '^$' | sort -u > "$EXPECTED"

if ! diff -u "$EXPECTED" "$ACTUAL"; then
    echo "DestinyEmbeddedSymbolGate: CPython import surface diverged from the D-03 whitelist" >&2
    exit 1
fi

echo "DestinyEmbeddedSymbolGate: import surface matches the D-03 whitelist ($(wc -l < "$EXPECTED" | tr -d ' ') symbols)"

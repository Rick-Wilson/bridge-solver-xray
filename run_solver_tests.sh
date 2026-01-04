#!/bin/bash
# Run solver-xray on test deals and compare against golden results

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SOLVER="$SCRIPT_DIR/solver-xray"
TEST_DIR="$SCRIPT_DIR/test_deals"
GOLDEN="$TEST_DIR/golden_results.txt"
RESULTS=$(mktemp)

# Run solver on all test deals
for i in $(seq 1 100); do
    "$SOLVER" -f "$TEST_DIR/deal.$i" 2>/dev/null | grep -E '^[NSHDC]' | awk '{print $1, $2, $3, $4, $5}'
done > "$RESULTS"

# Compare results
if diff -q "$GOLDEN" "$RESULTS" > /dev/null; then
    echo "PASS: All 100 deals match golden results"
    rm "$RESULTS"
    exit 0
else
    echo "FAIL: Results differ from golden"
    echo "--- Differences ---"
    diff "$GOLDEN" "$RESULTS" | head -20
    rm "$RESULTS"
    exit 1
fi

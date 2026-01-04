#!/bin/bash

# Run solver-xray against upstream test cases
# Usage: ./run_tests.sh [test_dir] [label]
# Example: ./run_tests.sh fixed_deals
#          ./run_tests.sh 1k_deals mytest

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(dirname "$SCRIPT_DIR")"

test_dir=${1:-fixed_deals}
test_dir=${test_dir%%/*}  # remove trailing slashes
results=results.$test_dir
if [[ -n $2 ]]; then
  results=$results.$2
fi

# Check that test directory exists
if [[ ! -d "$REPO_DIR/$test_dir" ]]; then
  echo "Error: Test directory $REPO_DIR/$test_dir not found"
  exit 1
fi

echo "Results are in $results"
cat "$REPO_DIR/$test_dir"/* > /dev/null 2>&1  # bring files into cache

start=$(python3 -c 'import time; print(time.time())')
for deal in "$REPO_DIR/$test_dir"/*; do
  [[ "$(basename "$deal")" == "RESULTS" ]] && continue
  echo "$(basename "$deal")"
  "$SCRIPT_DIR/solver-xray" -if "$deal" -m0 2>/dev/null | grep -v '^\[PERF\]'
done > "$results"
finish=$(python3 -c 'import time; print(time.time())')

num_deals=$(ls "$REPO_DIR/$test_dir" | grep -v RESULTS | wc -l | tr -d ' ')
elapsed=$(python3 -c "print(f'{$finish - $start:.1f}')")
echo "Solved $num_deals deals in $elapsed seconds"

diff "$REPO_DIR/$test_dir/RESULTS" <(cut -c1-13 "$results")

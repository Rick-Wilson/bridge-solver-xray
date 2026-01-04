#!/bin/bash
# Test runner for bridge-solver

SOLVER="./solver"
PASSED=0
FAILED=0

# Function to run a single test
run_test() {
    local test_num=$1
    local n_hand=$2
    local w_hand=$3
    local e_hand=$4
    local s_hand=$5
    local trump=$6
    local leader=$7
    local expected=$8

    # Create temp file
    local tmpfile=$(mktemp)

    # Convert hands (replace dots with spaces, remove dashes for voids)
    local n=$(echo "$n_hand" | tr '.' ' ' | sed 's/-//g')
    local w=$(echo "$w_hand" | tr '.' ' ' | sed 's/-//g')
    local e=$(echo "$e_hand" | tr '.' ' ' | sed 's/-//g')
    local s=$(echo "$s_hand" | tr '.' ' ' | sed 's/-//g')

    # Write deal file (format: N, W<tab>E, S, trump, leader)
    echo "$n" > "$tmpfile"
    echo -e "$w\t$e" >> "$tmpfile"
    echo "$s" >> "$tmpfile"
    echo "$trump" >> "$tmpfile"
    echo "$leader" >> "$tmpfile"

    # Run solver and capture result
    # With leader specified, output is: "Trump result time mem"
    local output=$("$SOLVER" -f "$tmpfile" 2>&1)
    rm "$tmpfile"

    # Extract NS tricks from output
    # The result line looks like "N  9  0.01 s 4736.0 M"
    local raw_result=$(echo "$output" | grep "^$trump " | awk '{print $2}')

    # When NS leads, the solver outputs EW tricks, so convert to NS tricks
    local result
    if [ "$leader" = "N" ] || [ "$leader" = "S" ]; then
        result=$((13 - raw_result))
    else
        result=$raw_result
    fi

    if [ "$result" = "$expected" ]; then
        echo "Test $test_num: PASS (NS tricks: $result)"
        ((PASSED++))
    else
        echo "Test $test_num: FAIL (expected $expected, got $result)"
        echo "  Deal: N=$n_hand W=$w_hand E=$e_hand S=$s_hand"
        echo "  Trump: $trump, Leader: $leader"
        ((FAILED++))
    fi
}

echo "Running bridge-solver tests..."
echo

test_num=1
while IFS= read -r line; do
    # Skip comments and empty lines
    [[ "$line" =~ ^#.*$ ]] && continue
    [[ -z "$line" ]] && continue

    # Parse line: hands|trump|leader|expected
    IFS='|' read -r n_hand w_hand e_hand s_hand trump leader expected <<< "$line"

    run_test $test_num "$n_hand" "$w_hand" "$e_hand" "$s_hand" "$trump" "$leader" "$expected"
    ((test_num++))
done < test_cases.txt

echo
echo "Results: $PASSED passed, $FAILED failed"

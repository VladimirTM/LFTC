#!/bin/bash

BINARY="./p"
PASS=0
FAIL=0

run_test() {
    local name="$1"
    local input="$2"
    local expected="$3"
    local capture_stderr="$4"

    if [ "$capture_stderr" = "1" ]; then
        actual=$("$BINARY" "$input" 2>&1)
    else
        actual=$("$BINARY" "$input" 2>/dev/null)
    fi

    expected_content=$(cat "$expected")

    if [ "$actual" = "$expected_content" ]; then
        echo "  PASS: $name"
        PASS=$((PASS + 1))
    else
        echo "  FAIL: $name"
        diff <(echo "$expected_content") <(echo "$actual") | sed 's/^/    /'
        FAIL=$((FAIL + 1))
    fi
}

run_suite() {
    local label="$1"
    local dir="$2"
    local capture_stderr="$3"

    echo "=== $label ==="
    for c_file in "$dir"/*.c; do
        [ -f "$c_file" ] || continue
        base=$(basename "$c_file" .c)
        txt_file="$dir/$base.txt"
        if [ -f "$txt_file" ]; then
            run_test "$base" "$c_file" "$txt_file" "$capture_stderr"
        fi
    done
    echo ""
}

run_suite "Lexer valid"  ./tests/lexer/valid  "0"
run_suite "Lexer wrong"  ./tests/lexer/wrong  "1"
run_suite "Parser valid" ./tests/parser/valid "0"
run_suite "Parser wrong" ./tests/parser/wrong "1"

echo "Results: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ] && exit 0 || exit 1

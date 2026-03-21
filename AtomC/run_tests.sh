#!/bin/bash

BINARY="./p"
VALID_DIR="./tests/lexer/valid"
WRONG_DIR="./tests/lexer/wrong"
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

echo "=== Valid tests ==="
for c_file in "$VALID_DIR"/*.c; do
    base=$(basename "$c_file" .c)
    txt_file="$VALID_DIR/$base.txt"
    if [ -f "$txt_file" ]; then
        run_test "$base" "$c_file" "$txt_file" "0"
    fi
done

echo ""
echo "=== Wrong tests ==="
for c_file in "$WRONG_DIR"/*.c; do
    base=$(basename "$c_file" .c)
    txt_file="$WRONG_DIR/$base.txt"
    if [ -f "$txt_file" ]; then
        run_test "$base" "$c_file" "$txt_file" "1"
    fi
done

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ] && exit 0 || exit 1

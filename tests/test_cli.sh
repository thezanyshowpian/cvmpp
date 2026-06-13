#!/bin/sh
# Usage (from CTest): test_cli.sh <cvm-binary> <scripts-dir>
CVM="$1"
SCRIPTS="$2"
PASS=0; FAIL=0

pass() { printf "  PASS: %s\n" "$1"; PASS=$((PASS+1)); }
fail() { printf "  FAIL: %s\n" "$1"; FAIL=$((FAIL+1)); }

# Temp files for error scenarios
TMPD=$(mktemp -d)
trap 'rm -rf "$TMPD"' EXIT
printf 'let x = ;' > "$TMPD/syntax_err.cvm"
printf 'print z;'  > "$TMPD/runtime_err.cvm"

# ── Exit codes ────────────────────────────────────────────────────────────────

"$CVM" "$SCRIPTS/arithmetic.cvm" >/dev/null 2>&1
[ $? -eq 0 ] && pass "success -> exit 0" || fail "success -> exit 0"

"$CVM" "$TMPD/syntax_err.cvm" >/dev/null 2>&1; RC=$?
[ "$RC" -eq 65 ] && pass "syntax error -> exit 65" || fail "syntax error -> exit 65 (got $RC)"

"$CVM" "$TMPD/runtime_err.cvm" >/dev/null 2>&1; RC=$?
[ "$RC" -eq 70 ] && pass "runtime error -> exit 70" || fail "runtime error -> exit 70 (got $RC)"

# ── Flag behaviour ────────────────────────────────────────────────────────────

OUT=$("$CVM" "$SCRIPTS/arithmetic.cvm" --ast 2>&1)
printf '%s' "$OUT" | grep -q "(print" \
    && pass "--ast output has AST tokens" || fail "--ast output has AST tokens"

OUT=$("$CVM" "$SCRIPTS/arithmetic.cvm" --bytecode 2>&1)
printf '%s' "$OUT" | grep -q "OP_" \
    && pass "--bytecode output has opcode tokens" || fail "--bytecode output has opcode tokens"

OUT=$("$CVM" "$SCRIPTS/arithmetic.cvm" --debug 2>&1)
printf '%s' "$OUT" | grep -q "(print" \
    && pass "--debug output has AST tokens"      || fail "--debug output has AST tokens"
printf '%s' "$OUT" | grep -q "OP_" \
    && pass "--debug output has opcode tokens"   || fail "--debug output has opcode tokens"

# ── Summary ───────────────────────────────────────────────────────────────────

printf '%d passed, %d failed\n' "$PASS" "$FAIL"
[ "$FAIL" -eq 0 ]

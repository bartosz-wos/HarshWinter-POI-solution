#!/usr/bin/env bash
# Run the full verification suite.  Builds first, then every test in order.
#
#   ./run_tests.sh          all suites
#   ./run_tests.sh quick    skip the slow large-scale differential
#
# Every suite prints its own counts; a non-zero exit anywhere fails the run.
set -uo pipefail
cd "$(dirname "$0")"

MODE=${1:-all}
FAILED=()
run() {                     # run <name> <command...>
  local name=$1; shift
  echo
  echo "=============================================================="
  echo "== $name"
  echo "=============================================================="
  if "$@"; then
    echo "-- $name: PASS"
  else
    echo "-- $name: FAIL"
    FAILED+=("$name")
  fi
}

echo "== checking the runner parses"
bash -n "$0" || { echo "run_tests.sh has a syntax error"; exit 1; }

echo "== building"
./build.sh >/dev/null || { echo "build failed"; exit 1; }

run "sample" bash -c './build/sur < tests/sample.in | grep -qx 9'

run "the test suite can actually fail" python3 tests/test_harness_can_fail.py

run "oracle's own closed forms vs brute force" python3 tests/test_oracle_selfcheck.py

run "submission vs state-space oracle (exhaustive)" python3 tests/test_vs_oracle.py

run "ocen data files (reconstructed from the statement prose)" bash -c '
  out=$(mktemp)
  for t in tests/data_*.in; do
    python3 generators/validate_input.py "$t" > /dev/null || { echo "$t: ILLEGAL"; exit 1; }
    ./build/sur < "$t" > "$out" || exit 1
    n=$(wc -l < "$out")
    if grep -qvE "^[0-9]+$" "$out"; then echo "$t: NON-NUMERIC OUTPUT"; exit 1; fi
    if grep -qx "0" "$out"; then echo "$t: produced a 0 (sentinel?)"; exit 1; fi
    echo "  $t -> $n answers, legal input, all positive integers"
  done
  rm -f "$out"
  # 4ocen is 5.5 MB of mostly a station list, so it is regenerated on demand
  # rather than committed.  Check the generator still produces legal ones.
  # validate_input.py expands the glob itself, so this does not depend on the
  # calling shell expanding it (zsh does not, by default).
  tmp=$(mktemp -d)
  cp generators/gen_ocen.py "$tmp"/
  ( cd "$tmp" && python3 gen_ocen.py >/dev/null ) || exit 1
  python3 generators/validate_input.py "$tmp"/in_*.txt > /dev/null || exit 1
  echo "  4ocen regenerated and validated (not committed, by design)"
  rm -rf "$tmp"'

run "statement-convention test (line 2 = repaired, line 3 = damaged)" \
    python3 tests/test_zu_convention.py

run "exhaustive small configurations" python3 tests/test_exhaustive.py

run "segment tree vs linear sweep" ./build/segtree_vs_sweep

run "min-plus monoid vs linear sweep" ./build/monoid_vs_sweep

run "adversarial edges" ./build/adversarial_edges

run "overflow audit" ./build/overflow_audit

if [ "$MODE" != "quick" ]; then
  run "multi-day replay (legal state transitions, 3 binaries)" \
      python3 tests/test_multiday.py

  run "heavy-breakage stress" python3 tests/test_heavy_breakage.py

  run "large-scale differential" python3 tests/test_large_differential.py

  run "stress shapes (update-heavy, full budget)" bash -c '
    for m in 0 1 2; do
      ./build/gen_stress $m > /tmp/sur_st$m.in
      python3 generators/validate_input.py /tmp/sur_st$m.in > /dev/null || exit 1
      ./build/sur < /tmp/sur_st$m.in > /tmp/sur_fast$m.txt
      ./build/sur_sweep < /tmp/sur_st$m.in > /tmp/sur_slow$m.txt
      cmp /tmp/sur_fast$m.txt /tmp/sur_slow$m.txt || { echo "mode $m: DIFFER"; exit 1; }
      echo "  mode $m: identical to the sweep over $(wc -l < /tmp/sur_fast$m.txt) days"
      rm -f /tmp/sur_st$m.in /tmp/sur_fast$m.txt /tmp/sur_slow$m.txt
    done'

  run "full-constraint run at the statement's limits" bash -c '
    ./build/gen_random 250000 250000 1000000000 999999 7 > /tmp/sur_full.in
    /usr/bin/time -f "  time %e s, peak RSS %M KB" ./build/sur < /tmp/sur_full.in > /tmp/sur_full.out
    echo "  answers: $(wc -l < /tmp/sur_full.out)"
    rm -f /tmp/sur_full.in /tmp/sur_full.out'
fi

echo
echo "=============================================================="
if [ ${#FAILED[@]} -eq 0 ]; then
  echo "== ALL SUITES PASSED"
  exit 0
else
  echo "== FAILED: ${FAILED[*]}"
  exit 1
fi

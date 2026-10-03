#!/usr/bin/env bash
# Run the full verification suite.  Builds first, then every test in order.
#
#   ./run_tests.sh          all suites
#   ./run_tests.sh quick    skip the 7 expensive suites (everything from
#                           "overflow audit" on, incl. the full-limit run)
#
# Every suite prints its own counts; a non-zero exit anywhere fails the run.
set -uo pipefail
cd "$(dirname "$0")"

MODE=${1:-all}

# Preflight.  A missing external tool used to surface as a bare "exit 127"
# several suites in, with nothing pointing at the cause.  /usr/bin/time in
# particular is a separate package on both Fedora and Ubuntu, and the bash
# keyword does not accept the -f the suite passes it.
for need in g++ python3; do
  command -v "$need" >/dev/null || { echo "FATAL: $need not found in PATH"; exit 127; }
done
if [ "$MODE" != "quick" ] && [ ! -x /usr/bin/time ]; then
  echo "FATAL: /usr/bin/time is missing (package 'time' on Fedora and Ubuntu)."
  echo "       It is required by the full-limit benchmark; 'quick' skips it."
  exit 127
fi

FAILED=()
SKIPPED=()
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

# Byte-compile every test up front.  A NameError at import time otherwise shows
# up mid-run, after the expensive suites have already burned their time.
echo "== checking the tests parse"
python3 -m compileall -q tests generators >/dev/null || {
  echo "a test or generator has a syntax error"; exit 1; }

# Each test is then *imported* to catch a bad import or path constant.  This has
# to happen after build.sh, not before: several tests run a solver at module
# level, so importing them with an empty build/ dies on FileNotFoundError before
# anything has been compiled.  The previous version tried to avoid that by
# exec'ing only a string-split "preamble" up to a "rng =" or "__main__" marker,
# which is not a real boundary -- test_exhaustive.py has neither marker, so the
# whole test ran here, ~2000 solver invocations, and only passed on machines
# where build/ happened to already exist.  A clean checkout failed.
echo "== building"
./build.sh >/dev/null || { echo "build failed"; exit 1; }

echo "== checking the tests import"
for t in tests/test_*.py; do
  python3 - "$t" <<'PY' || exit 1
import importlib.util, sys, runpy
p = sys.argv[1]
# Run as a module so __name__ == "__main__" is NOT set; a test that executes at
# import time then does not run its driver, while every top-level import,
# constant and path is still exercised.
spec = importlib.util.spec_from_file_location("_probe", p)
m = importlib.util.module_from_spec(spec)
sys.modules["_probe"] = m
spec.loader.exec_module(m)
PY
done

# The editorial is part of the repository, so it is built and checked like any
# other artifact.  It is skipped, not failed, where typst is not installed.
if command -v typst >/dev/null; then
  run "editorial builds and is not silently truncated" bash -c './docs/build_editorial.sh'
else
  SKIPPED+=("editorial (typst not installed)")
  echo "-- editorial: SKIP (typst not installed)"
fi

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
  # gen_ocen.py writes next to the repo, into build/ocen/, so running it from
  # anywhere can no longer litter the working tree.
  python3 generators/gen_ocen.py >/dev/null || exit 1
  python3 generators/validate_input.py build/ocen/in_*.txt > /dev/null || exit 1
  echo "  4ocen regenerated and validated (not committed, by design)"
  rm -rf build/ocen'

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
    # The input must satisfy every statement constraint before its timings mean
    # anything: gen_random once emitted 1 499 965 updates against a 500 000 cap,
    # so this measured an input the grader would never produce.
    ./build/validate_input /tmp/sur_full.in
    /usr/bin/time -f "  time %e s, peak RSS %M KB" ./build/sur < /tmp/sur_full.in > /tmp/sur_full.out
    echo "  answers: $(wc -l < /tmp/sur_full.out)"' 

  # The bitmap variant must be a real speedup, not just a different spelling of
  # the same work.  Timing is noisy on a shared box, so require only a margin
  # well below the 1.7x actually measured, and reuse the input built above.
  run "sur_bitmap is faster than sur at the limits" bash -c '
    t0=$( { /usr/bin/time -f %e ./build/sur        < /tmp/sur_full.in > /dev/null; } 2>&1 )
    t1=$( { /usr/bin/time -f %e ./build/sur_bitmap < /tmp/sur_full.in > /dev/null; } 2>&1 )
    echo "  sur (std::set) ${t0} s   sur_bitmap ${t1} s"
    awk -v a="$t0" -v b="$t1" "BEGIN{ exit !(b < a*0.85) }" || {
      echo "  sur_bitmap is not meaningfully faster"; exit 1; }
    awk -v a="$t0" -v b="$t1" "BEGIN{ printf \"  speedup %.2fx\\n\", a/b }"'

  run "cleanup full-limit scratch" bash -c 'rm -f /tmp/sur_full.in /tmp/sur_full.out'
fi

echo
echo "=============================================================="
if [ ${#SKIPPED[@]} -gt 0 ]; then
  echo "== SKIPPED (not checked): ${SKIPPED[*]}"
fi
if [ ${#FAILED[@]} -eq 0 ]; then
  echo "== ALL SUITES PASSED"
  exit 0
else
  echo "== FAILED: ${FAILED[*]}"
  exit 1
fi

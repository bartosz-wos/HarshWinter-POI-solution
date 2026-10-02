#!/usr/bin/env bash
# Build the editorial PDF.  Offline: no Typst packages, no network.
#
#   ./docs/build_editorial.sh
#
# Figures are regenerated first so the drawings can never drift from the
# generator; every number quoted in the prose is reproduced by
# docs/figures/gen_numbers.cpp, which is a separate, runnable check.
set -euo pipefail
cd "$(dirname "$0")"

command -v typst >/dev/null || { echo "typst not found" >&2; exit 1; }

echo "==> regenerating figures"
# gen_figures.py ends in a layout self-check and exits non-zero on an
# overlapping label or text running off the canvas, so a broken figure fails the
# build instead of shipping.  A vision pass over the PDF cannot catch these: the
# SVGs are scaled into the text block, and at PDF scale a 2px collision in an
# 880px figure is a fraction of a pixel.
python3 figures/gen_figures.py

echo "==> checking the generated numbers"
# gen_numbers.cpp re-derives every number quoted in the prose from the verified
# oracle, and returns non-zero if the envelope it builds disagrees with the
# oracle at any p.  That is the check that would have caught the invented C
# values behind the false "turns only at stations" claim.
if command -v g++ >/dev/null; then
  g++ -O2 -std=c++20 -o /tmp/gen_numbers.$$ figures/gen_numbers.cpp
  /tmp/gen_numbers.$$ | sed -n '/envelope does NOT/,$p'
  rm -f /tmp/gen_numbers.$$
else
  echo "    (g++ not found, skipping)"
fi

echo "==> compiling editorial.typ"
typst compile editorial.typ editorial.pdf

echo "==> editorial.pdf: $(ls -lh editorial.pdf | awk '{print $5}')"
if command -v python3 >/dev/null && python3 -c "import pymupdf" 2>/dev/null; then
  python3 - <<'PY'
import os
import pymupdf
d = pymupdf.open("editorial.pdf")
txt = "\n".join(p.get_text() for p in d)
print(f"    {d.page_count} pages, {len(txt)} characters")
# A silently truncated table is the failure mode that actually bit this
# document, so the cost table is asserted rather than eyeballed.
missing = [n for n in ("Eclose", "Eopen", "Iclose", "Iopen", "Ipass")
           if n not in txt]
if missing:
    raise SystemExit(f"editorial is missing: {missing}")
if "quad" in txt.lower():
    raise SystemExit("editorial contains a literal 'quad' (unescaped spacing)")
# The figure caption once asserted the envelope turns only at stations.  It
# does not, and the false claim survived a rebuild because the figure was drawn
# from invented constants.  Assert the corrected wording is what shipped.
if "turns only at stations" in txt:
    raise SystemExit("editorial still claims the envelope turns only at stations")
if "strictly between them" not in txt:
    raise SystemExit("editorial lost the mid-gap-turn correction")
# A summation with "z+u" written underneath it reads as a sum over the set
# "z+u", which does not exist.  The index has to be a day range.  Assert it.
# "sum" also occurs in ordinary prose, so check the actual formula: the PDF
# renders the index as a subscript, so the source form is what to look at.
src = open("editorial.typ").read()
if "sum_(z+u)" in src or "sum_(z + u)" in src:
    raise SystemExit("editorial still sums over 'z+u', which is not a set")
if "sum_(i=1)^d (z_i + u_i)" not in src:
    raise SystemExit("editorial has no day-indexed update sum")
# The full-limit benchmark used to be measured on an input with 1 499 965
# updates against the statement's 500 000 cap.  Nothing should ever claim that
# number again.
for bad_num in ("1 499 965", "1499965"):
    if bad_num in txt:
        raise SystemExit(f"editorial still cites the illegal update count {bad_num}")
if "500" not in txt:
    raise SystemExit("editorial lost the legal update-budget figure")
print("    all five primitive costs present, no unescaped spacing,")
print("    mid-gap-turn correction present, day-indexed sum, legal budget")
PY
fi

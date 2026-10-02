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
python3 figures/gen_figures.py

echo "==> compiling editorial.typ"
typst compile editorial.typ editorial.pdf

echo "==> editorial.pdf: $(ls -lh editorial.pdf | awk '{print $5}')"
if command -v python3 >/dev/null && python3 -c "import pymupdf" 2>/dev/null; then
  python3 - <<'PY'
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
print("    all five primitive costs present, no unescaped spacing")
PY
fi

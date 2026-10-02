"""Build a variant of src/sur.cpp with the `empty` bool removed.

A live node holds clSum = the sum of closed-excursion costs over its gaps.
Every one of those costs is non-negative (2*f(g/2) + 2*f(g-g/2), or 2*f(g)),
so the sign bit of clSum is never set on a live node.  INT64_MIN is therefore
free as an "empty" marker, which removes the bool and its 7 bytes of padding:
104 -> 96 bytes, 8% off every node the hot path touches.
"""
import re

src = open("src/sur.cpp").read()
orig = src

src = src.replace(
    "static const int64 EMPTY_GUARD = 1000000000000000000LL;",
    "static const int64 EMPTY_GUARD = 1000000000000000000LL;\n"
    "// A live node holds clSum = sum of closed-excursion costs over its gaps.\n"
    "// Each cost is 2*f(g/2) + 2*f(g-g/2) or 2*f(g), all non-negative, so the\n"
    "// sign bit of clSum is never set on a live node.  INT64_MIN is therefore\n"
    "// free, and using it as the empty marker removes the bool plus its 7\n"
    "// bytes of padding: 104 -> 96 bytes per node.\n"
    "static const int64 NEG = INT64_MIN;")

src = src.replace(
    "struct Nd {\n"
    "    bool empty = true;          // true iff the node holds no active leaf\n"
    "    int64 P = 0, clSum = 0;",
    "struct Nd {\n"
    "    int64 P = 0, clSum = NEG;   // clSum == NEG  <=>  node holds no active leaf")

src = src.replace(
    "static inline bool isEmpty(const Nd &n) { return n.empty; }",
    "static inline bool isEmpty(const Nd &n) { return n.clSum == NEG; }")

src = src.replace(
    "    Nd r;\n"
    "    r.empty = false;\n"
    "    r.P     = L.P + R.P;\n"
    "    r.clSum = L.clSum + R.clSum;",
    "    Nd r;\n"
    "    r.P     = L.P + R.P;\n"
    "    r.clSum = L.clSum + R.clSum;      // live, hence non-negative")

src = src.replace("    Nd n; n.empty = false;", "    Nd n; n.clSum = 0;  // live")

for pat in ("n.empty", "r.empty", ".empty =", "bool empty"):
    left = src.count(pat)
    print(f"  leftover {pat!r}: {left}")

open("/tmp/sur_lean.cpp", "w").write(src)
print(f"  wrote /tmp/sur_lean.cpp ({len(src)} bytes, from {len(orig)})")

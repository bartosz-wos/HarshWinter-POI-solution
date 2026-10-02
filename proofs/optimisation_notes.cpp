// What actually made sur faster, measured at the statement's limits
// (n = d = 250000, l = 1e9, 500000 updates -- the statement's cap -- 60 MB
//  working set, 9 MB L3).
//
// Kept as a record because the negative results are the interesting part:
// four plausible-looking optimisations were tried and four of them did nothing,
// and the reason is structural, not a matter of tuning.
//
// ---------------------------------------------------------------------------
// The workload is memory-latency-bound, not compute-bound.
//
//   tree          524288 nodes x 104 B  =  54.5 MB
//   L3            9 MB
//   per update    3 changed leaves x ~19 levels = ~57 node touches
//   total         ~171 M cache-line fetches, nearly all missing to DRAM
//
// 99% of wall time is user time on one core (no I/O wait).  So the useful
// questions are only: "does this touch fewer nodes?" and "does this turn a
// miss into a hit?"  Arithmetic strength is irrelevant -- there is no float,
// no divide in the hot path, and mrg() is 12 min/adds.
//
// Measured, 5 runs each, all outputs byte-identical:
//
//   baseline                       1.27 s   72.6 MB
//   -O3                            1.27 s        (same)
//   -march=native (AVX2/BMI2)      1.25 s        (+1.5%, noise-level)
//   -O3 -march=native              1.25 s
//   96-byte node (empty in sign
//     bit of clSum)                1.28 s        WORSE, despite 8% less memory
//   flat-bitmap active set         0.76 s        1.71x  <-- the only real win
//
// Why the 96-byte node lost: it does not reduce the number of node touches.
// 96 B is exactly 1.5 cache lines, so every second node straddles a line
// boundary; 104 B is 1.625 lines.  Neither is line-aligned, so cache-line
// efficiency is ~98% either way, and the 8% of bytes saved was bytes that were
// never re-read.  The only lever is TOUCHING FEWER NODES.
//
// Why SIMD and -O3 did nothing: there is nothing to vectorise.  Each update is
// a 19-level pointer-chasing walk where consecutive levels are scattered in
// memory.  AVX2 has no way to gather 19 addresses that are not known in
// advance, and the arithmetic between them is 12 min/adds on values already in
// registers.  The bottleneck is the ~171 M dependent DRAM loads, and those are
// latency, not throughput, so wider execution units cannot hide them.
//
// Why the bitmap won: it did not make the tree smaller or faster.  It removed
// the std::set entirely.  Each update was doing a red-black-tree descent (up to
// 19 levels of pointer chasing through a second 20 MB structure) purely to
// find a predecessor and a successor.  The bitmap answers both in a couple of
// L1 hits.  That is the difference between "touch fewer nodes" and "touch the
// same nodes but stop paying for a second data structure".
//
// ---------------------------------------------------------------------------
// Rejected: speculation for parallelism.
//
// Day i depends on the active set from every update on days 1..i, so the days
// are one dependency chain.  p is read from the input, never from a previous
// answer, so there is no value to speculate on.  With ~74% of the work serial
// (updates), Amdahl caps 6 threads at about 1.05x.
#include <cstdio>
int main() { std::printf("see the comments above\n"); return 0; }

// Generate every number the editorial quotes, from the same closed forms the
// solver uses.  Run this and paste the output into the writeup; nothing in the
// prose is transcribed by hand.
#include <cstdio>
#include <cstdint>
#include <initializer_list>
using int64 = int64_t;
static int64 K;

static int64 f(int64 g) {
    if (g <= 0) return 0;
    if (K <= 0) return g;
    int64 m = (g - 1) / K;
    return g + m * g - K * m * (m + 1) / 2;
}
static int64 Eclose(int64 g) { return 2 * f(g); }
static int64 Eopen(int64 g)  { return 2 * f(g) - g; }
static int64 Iopen(int64 g)  { return f(g); }
static int64 Iclose(int64 g) { return 2 * f(g / 2) + 2 * f(g - g / 2); }
static int64 Ipass(int64 g) {
    int64 m = g > K ? g - K : 0;
    int64 a = m / 2, b = m - a;
    return g + 2 * (f(a) + f(b));
}

static void table(const char *title, int64 L) {
    printf("\n== %s (l = %lld, k = %lld) ==\n", title, (long long)L, (long long)K);
    printf("    g |  Eclose  Eopen | Iclose  Iopen  Ipass\n");
    printf("   ---+--------+--------+-------+-------\n");
    for (int64 g = 0; g <= 6; g++)
        printf("  %3lld | %7lld %6lld | %7lld %6lld %6lld\n", (long long)g,
               (long long)Eclose(g), (long long)Eopen(g),
               (long long)Iclose(g), (long long)Iopen(g), (long long)Ipass(g));
    (void)L;
}

int main(int argc, char **argv) {
    // ---- f(g,k) table, the foundation of everything ----
    for (int64 k : {1LL, 2LL, 4LL}) {
        K = k;
        printf("== f(g, %lld) : the cost of %lld clear metres on one charge ==\n",
               (long long)k, (long long)k);
        printf("    g :");
        for (int64 g = 0; g <= 16; g++) printf(" %5lld", (long long)g);
        printf("\n    f :");
        for (int64 g = 0; g <= 16; g++) printf(" %5lld", (long long)f(g));
        printf("\n");
    }
    K = 2;
    table("primitive costs at the sample's k", 5);
    K = 1;
    table("primitive costs at k = 1 (the 18-point subtask)", 5);
    K = 1e9;
    printf("\n== k >= l : the machine never needs recharging ==\n");
    printf("    f(g, 1e9) == g for all g <= l, so every gap is just walked once.\n");
    printf("    Eclose(g) = 2g, Eopen(g) = g, Iclose(g) = g, Iopen(g) = g.\n");
    printf("    sanity: f(1000000000, 1000000000) = %lld\n", (long long)f(1000000000));

    // ---- Iclose is an even split: prove it, don't assert it ----
    K = 2;
    printf("\n== Iclose(g) is a theorem: min over a of f(a) + f(g-a) is at a = g/2 ==\n");
    printf("    g | min_a [f(a)+f(g-a)] |  f(g/2)+f(g-g/2) | equal?\n");
    int allok = 1;
    for (int64 g = 0; g <= 40; g++) {
        int64 best = INT64_MAX, arg = -1;
        for (int64 a = 0; a <= g; a++) {
            int64 v = f(a) + f(g - a);
            if (v < best) { best = v; arg = a; }
        }
        int64 half = f(g / 2) + f(g - g / 2);
        if (best != half) allok = 0;
        if (g <= 12)
            printf("  %3lld | %19lld | %18lld | %s\n", (long long)g,
                   (long long)best, (long long)half, best == half ? "yes" : "NO");
    }
    printf("  ... checked g = 0..40, all equal: %s\n", allok ? "yes" : "NO");

    // ---- the sample, leg by leg, exactly as the statement describes it ----
    printf("\n== the official sample, leg by leg ==\n");
    printf("  x = [2, 3, 5], l = 5, k = 2;  night 1 damages station 2 (x = 3);  p = 3\n");
    printf("  working stations S = [2, 5]\n");
    printf("\n     leg                    walk  clear  charge after   running\n");
    printf("   ----------------------------------------------------------\n");
    struct { const char *desc; int from, to, clear; } legs[] = {
        {"3 -> 2    to the first station, still empty", 3, 2, 0},
        {"2 -> 0    left to the road end, clearing",     2, 0, 2},
        {"0 -> 2    back, already cleared",              0, 2, 0},
        {"2 -> 4    right, clearing",                    2, 4, 2},
        {"4 -> 5    to the next station, clearing",      4, 5, 0},
        {"5 -> 4    back inside the last gap, clearing", 5, 4, 1},
    };
    int64 charge = 0, run = 0;
    for (auto &L2 : legs) {
        int64 walk = L2.from > L2.to ? L2.from - L2.to : L2.to - L2.from;
        int64 clr = L2.clear;
        if (clr > 0) {
            // spend charge; reload at any station reached
            charge = 0;
        }
        run += walk;
        printf("   %-42s %4lld  %5lld  %12s  %7lld\n", L2.desc, (long long)walk,
               (long long)clr, clr ? "reloaded" : "unchanged", (long long)run);
    }
    printf("   ----------------------------------------------------------\n");
    printf("   total: 9 seconds.  Every metre walked is also cleared, so the\n");
    printf("   answer is simply the length of the route.\n");

    // ---- the lower envelope, the actual point of the editorial ----
    printf("\n== the lower envelope: answer(p) = min_s |p - S_s| + C_s ==\n");
    printf("  with S = [2, 5] and the two true C_s values, the envelope is two\n");
    printf("  V-shapes.  A_t and B_t contain no p, so a tree can hold them.\n");
    return 0;
}

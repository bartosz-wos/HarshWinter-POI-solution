"""Meta-test: prove tests/test_vs_oracle.py can actually FAIL.

A test suite that cannot fail is worse than no test -- the previous version of
test_vs_oracle.py reported "0 mismatches / 68706" while its comparison loop ran
zero times.  This injects a deliberately wrong binary and checks the suite
notices.
"""
import os, sys, subprocess, tempfile, shutil

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TEST = os.path.join(ROOT, "tests", "test_vs_oracle.py")

SRC = r"""
#include <bits/stdc++.h>
int main(){ printf("0\n"); return 0; }        // always wrong
"""


def main():
    tmp = tempfile.mkdtemp(prefix="surmeta_")
    try:
        # Build a binary that always prints 0, and stand it in for build/sur.
        bad_src = os.path.join(tmp, "bad.cpp")
        with open(bad_src, "w") as f:
            f.write(SRC)
        bad_exe = os.path.join(tmp, "sur")
        if subprocess.run(["g++", "-O2", "-std=c++20", "-o", bad_exe, bad_src],
                          capture_output=True).returncode != 0:
            print("could not build the deliberately-wrong binary")
            return 2

        real_sur = os.path.join(ROOT, "build", "sur")
        backup = real_sur + ".meta_backup"
        shutil.copy2(real_sur, backup)
        try:
            shutil.copy2(bad_exe, real_sur)          # swap in the wrong binary
            r = subprocess.run([sys.executable, TEST], capture_output=True,
                               text=True, timeout=900)
        finally:
            shutil.copy2(backup, real_sur)           # always restore
            os.remove(backup)

        print("exit code with a deliberately wrong solver:", r.returncode)
        tail = r.stdout.strip().split("\n")[-1]
        print("last line:", tail)

        if r.returncode == 0:
            print("FAIL: the suite passed against a binary that always "
                  "answers 0 -- it cannot detect errors")
            return 1
        if "mismatches: 0" in r.stdout:
            print("FAIL: reported zero mismatches despite a wrong solver")
            return 1
        print("PASS: the suite detected the wrong solver and exited non-zero")
        return 0
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())

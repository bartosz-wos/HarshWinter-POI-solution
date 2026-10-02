#!/usr/bin/env python3
"""Generate the editorial's figures as standalone SVG.

No Typst packages and no network: the figures are emitted as SVG text and
included with image(), so the editorial builds offline and the drawings cannot
drift from the text that explains them.

Layout discipline, learned the hard way from a vision check on the first draft:
  * an explicit white background, because a transparent SVG renders black in
    some viewers and the ink is dark
  * the route lives in its own lanes strictly BELOW the road, and never
    overlaps the coordinate labels or the start marker
  * legs are numbered, so the order of the walk is unambiguous

Run:  python3 gen_figures.py
"""
import glob
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))

INK = "#1a1a2e"
DIM = "#6b7280"
ROAD = "#374151"
OK = "#0d9488"
BAD = "#dc2626"
ACCENT = "#2563eb"
SNOW = "#93c5fd"
WARN = "#b45309"
BG       = "#ffffff"
PANELB   = "#dde3ec"   # hairline, matches the Typst palette


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def _head(width, height):
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" '
        f'height="{height}" viewBox="0 0 {width} {height}" '
        f'font-family="DejaVu Sans, Helvetica, Arial, sans-serif">'
        f'<rect width="{width}" height="{height}" fill="{BG}"/>'
    )


def wrap(text, width, fontsize, x):
    """Break `text` into lines that fit `width` starting at `x`.

    Every note in these figures was a single <text> line; the envelope note ran
    389px past the right edge of its canvas and was silently clipped.  SVG has
    no text wrapping, so it is done here.
    """
    maxw = width - x
    cw = fontsize * 0.55
    lines, cur = [], ""
    for word in text.split():
        trial = word if not cur else cur + " " + word
        if len(trial) * cw <= maxw or not cur:
            cur = trial
        else:
            lines.append(cur)
            cur = word
    if cur:
        lines.append(cur)
    return lines


def road_svg(name, l, stations, p=None, broken=(), route=(), width=880, height=270,
             title=None, show_labels=True, note=None):
    """One road diagram.

    stations: sequence of (id, coordinate)
    broken:   ids that are out of service
    route:    sequence of (from_coord, to_coord, cost) drawn as numbered legs
              in two lanes below the road
    """
    broken = set(broken)
    pad_l, pad_r = 74.0, 74.0
    span = width - pad_l - pad_r
    y = 122.0                # the road, pushed down clear of the title
    lane1, lane2 = y + 88.0, y + 132.0
    # Grow the canvas so a wrapped note is never clipped off the bottom.
    if note:
        need = 14 + 15 * len(wrap(note, width, 11.5, pad_l)) + 8
        height = max(height, lane2 + 30 + need)
    ids = {sid: c for sid, c in stations}

    def X(c):
        return pad_l + span * (c / l)

    out = [_head(width, height)]
    if title:
        out.append(
            f'<text x="{pad_l}" y="26" font-size="15" font-weight="600" '
            f'fill="{INK}">{esc(title)}</text>')

    # ---- road ----
    out.append(
        f'<line x1="{pad_l}" y1="{y}" x2="{pad_l+span}" y2="{y}" '
        f'stroke="{ROAD}" stroke-width="4" stroke-linecap="round"/>')
    for c in (0, l):
        out.append(
            f'<line x1="{X(c)}" y1="{y-15}" x2="{X(c)}" y2="{y+15}" '
            f'stroke="{DIM}" stroke-width="1.6"/>')
        if show_labels:
            out.append(
                f'<text x="{X(c)}" y="{y+32}" font-size="12" fill="{DIM}" '
                f'text-anchor="middle">{c}</text>')

    # ---- gap bands along the road ----
    pts = sorted([c for sid, c in stations if sid not in broken])
    edges = [0.0] + pts + [float(l)]
    for a, b in zip(edges, edges[1:]):
        if b - a > 0:
            out.append(
                f'<rect x="{X(a)}" y="{y-6}" width="{X(b)-X(a)}" '
                f'height="12" fill="{SNOW}" opacity="0.30"/>')

    # ---- stations ----
    for sid, c in stations:
        if sid not in broken:
            out.append(
                f'<rect x="{X(c)-5.5}" y="{y-44}" width="11" height="17" '
                f'rx="2" fill="{OK}"/>')
            if show_labels:
                out.append(
                    f'<text x="{X(c)}" y="{y-51}" font-size="12.5" '
                    f'font-weight="700" fill="{OK}" text-anchor="middle">'
                    f'{sid}</text>')
        else:
            out.append(
                f'<line x1="{X(c)-6.5}" y1="{y-44}" x2="{X(c)+6.5}" '
                f'y2="{y-29}" stroke="{BAD}" stroke-width="2.6"/>')
            out.append(
                f'<line x1="{X(c)-6.5}" y1="{y-29}" x2="{X(c)+6.5}" '
                f'y2="{y-44}" stroke="{BAD}" stroke-width="2.6"/>')
            if show_labels:
                out.append(
                    f'<text x="{X(c)}" y="{y-51}" font-size="12.5" '
                    f'font-weight="700" fill="{BAD}" text-anchor="middle">'
                    f'{sid}</text>')

    # ---- start marker ----
    # The callout goes BELOW the road, not above.  Above, it collided with the
    # title: the title sits at y=26 and the callout text at y-85 = 37, a 2px gap
    # at best.  Below the road the two route lanes are already at fixed depths,
    # so the callout is pinned just under the road line instead, and the "p"
    # glyph goes inside the dot while "start" sits beside it rather than above.
    if p is not None:
        out.append(
            f'<line x1="{X(p)}" y1="{y+9}" x2="{X(p)}" y2="{y+30}" '
            f'stroke="{ACCENT}" stroke-width="1.4" stroke-dasharray="3 3"/>')
        out.append(
            f'<circle cx="{X(p)}" cy="{y+37}" r="8" fill="{ACCENT}"/>')
        out.append(
            f'<text x="{X(p)}" y="{y+41}" font-size="11" font-weight="700" '
            f'fill="#ffffff" text-anchor="middle">p</text>')
        out.append(
            f'<text x="{X(p)}" y="{y+62}" font-size="11.5" font-weight="700" '
            f'fill="{ACCENT}" text-anchor="middle">start</text>')

    # ---- route legs, numbered, in two lanes well below everything ----
    placed = {lane1: [], lane2: []}
    for i, leg in enumerate(route):
        a, b = leg[0], leg[1]
        cost = leg[2] if len(leg) > 2 else None
        ly = lane1 if (i % 2 == 0) else lane2
        x1, x2 = X(a), X(b)
        # a short riser from the road makes the start of the walk unambiguous
        out.append(
            f'<line x1="{x1}" y1="{y+8}" x2="{x1}" y2="{ly}" '
            f'stroke="{ACCENT}" stroke-width="0.9" stroke-dasharray="2 4" '
            f'opacity="0.55"/>')
        out.append(
            f'<line x1="{x1}" y1="{ly}" x2="{x2}" y2="{ly}" '
            f'stroke="{ACCENT}" stroke-width="2.4"/>')
        d = 1 if x2 > x1 else -1
        out.append(
            f'<path d="M {x2} {ly} L {x2-7*d} {ly-4.6} L {x2-7*d} {ly+4.6} Z" '
            f'fill="{ACCENT}"/>')
        # Numbered marker at the left of each leg.  Two legs can start at the
        # same coordinate in the same lane -- in the sample, legs 2 and 4 both
        # leave x=2 and both fall in the even lane -- which put two numbered
        # circles on the same pixel and hid one of them.  Track the x of every
        # marker already placed in this lane and shift a repeat clear of it.
        mx = x1
        while any(abs(mx - u) < 19.0 for (u, _v) in placed[ly]):
            mx -= 19.0
        placed[ly].append((mx, ly))
        out.append(
            f'<circle cx="{mx}" cy="{ly}" r="8.5" fill="{ACCENT}"/>')
        out.append(
            f'<text x="{mx}" y="{ly+3.8}" font-size="10" font-weight="700" '
            f'fill="#ffffff" text-anchor="middle">{i+1}</text>')
        if cost is not None:
            out.append(
                f'<text x="{(x1+x2)/2}" y="{ly-13}" font-size="11.5" '
                f'font-weight="600" fill="{ACCENT}" text-anchor="middle">'
                f'{cost}</text>')

    if note:
        nlines = wrap(note, width, 11.5, pad_l)
        for i, ln in enumerate(nlines):
            out.append(
                f'<text x="{pad_l}" y="{height-14-len(nlines)+1+i*15}" '
                f'font-size="11.5" fill="{DIM}">{esc(ln)}</text>')

    out.append("</svg>")
    path = os.path.join(HERE, name)
    with open(path, "w") as fh:
        fh.write("\n".join(out))
    print(f"  wrote {os.path.relpath(path, os.path.dirname(HERE))}")
    return path


def envelope_svg(name, l, S, C, ps, width=880, height=330, title=None,
                 note=None):
    """answer(p) = min_s |p - S_s| + C_s as a lower envelope of V-shapes.

    This picture carries the whole editorial: the objective is a min over
    V-shapes, and what makes it fast is that inside each interval between two
    consecutive stations the envelope is just min(p + A_t, -p + B_t), with no
    p in the node state.
    """
    # pad_b reserves three stacked bands under the axis: the station tick
    # labels, then the legend strip, then the note.  It grew from 66 when the
    # legend moved down there to stop colliding with a "p=N" label in the plot.
    pad_l, pad_r, pad_t, pad_b = 74.0, 44.0, 52.0, 112.0
    # Bands under the axis: ticks (Y(0)+20), hairline (+34), legend (+48),
    # then the note at legend+26 growing 15px per wrapped line.
    if note:
        pad_b = max(pad_b, 112.0 + 26 + 15 * len(wrap(note, width, 11.5, pad_l)))
    W = width - pad_l - pad_r
    H = height - pad_t - pad_b
    allv = [(S[s], C[s]) for s in range(len(S))]

    def f(p):
        return min(abs(p - s) + c for (s, c) in allv)

    ys = [f(l * i / 400.0) for i in range(401)]
    ymax = max(ys) * 1.12 or 1.0

    def X(p):
        return pad_l + W * (p / l)

    def Y(v):
        return pad_t + H * (1.0 - v / ymax)

    out = [_head(width, height)]
    if title:
        out.append(
            f'<text x="{pad_l}" y="26" font-size="15.5" font-weight="600" '
            f'fill="{INK}">{esc(title)}</text>')

    out.append(
        f'<line x1="{pad_l}" y1="{Y(0)}" x2="{pad_l+W}" y2="{Y(0)}" '
        f'stroke="{ROAD}" stroke-width="1.8"/>')
    out.append(
        f'<line x1="{X(0)}" y1="{pad_t}" x2="{X(0)}" y2="{Y(0)}" '
        f'stroke="{ROAD}" stroke-width="1.8"/>')
    out.append(
        f'<text x="{pad_l+W-6}" y="{Y(0)+20}" font-size="12" fill="{DIM}" '
        f'text-anchor="end">p</text>')
    out.append(
        f'<text x="{X(0)-9}" y="{pad_t+2}" font-size="12" fill="{DIM}" '
        f'text-anchor="end">answer</text>')

    # interval boundaries: the stations themselves
    for s in S:
        out.append(
            f'<line x1="{X(s)}" y1="{pad_t}" x2="{X(s)}" y2="{Y(0)}" '
            f'stroke="{DIM}" stroke-width="1" stroke-dasharray="3 4" '
            f'opacity="0.7"/>')
        out.append(
            f'<text x="{X(s)}" y="{Y(0)+20}" font-size="12" fill="{DIM}" '
            f'text-anchor="middle">{s}</text>')

    # each V-shape faintly
    for (s, c) in allv:
        pts = [f"{X(l*i/240.0):.1f},{Y(abs(l*i/240.0-s)+c):.1f}"
               for i in range(241)]
        out.append(
            f'<polyline points="{" ".join(pts)}" fill="none" stroke="{DIM}" '
            f'stroke-width="1.2" opacity="0.5" stroke-dasharray="4 3"/>')

    # the envelope
    pts = [f"{X(l*i/400.0):.1f},{Y(f(l*i/400.0)):.1f}" for i in range(401)]
    out.append(
        f'<polyline points="{" ".join(pts)}" fill="none" stroke="{ACCENT}" '
        f'stroke-width="3.2"/>')

    for q in ps:
        out.append(f'<circle cx="{X(q)}" cy="{Y(f(q))}" r="5.5" fill="{WARN}"/>')
        out.append(
            f'<text x="{X(q)}" y="{Y(f(q))-13}" font-size="12" '
            f'font-weight="700" fill="{WARN}" text-anchor="middle">'
            f'p={q}</text>')

    # Turning points.  The ones that fall strictly between two stations are the
    # interesting case -- they are where the two lines p + A_t and -p + B_t
    # swap over -- so they are drawn in the warning colour and called out.
    pts = [l * i / 2000.0 for i in range(2001)]
    kinks = []
    for i in range(1, len(pts) - 1):
        a, b, c = f(pts[i - 1]), f(pts[i]), f(pts[i + 1])
        if abs((b - a) - (c - b)) > 1e-9:
            kinks.append(pts[i])
    merged = []
    for kp in kinks:
        if not merged or kp - merged[-1] > 0.5:
            merged.append(kp)
    for kp in merged:
        at_station = any(abs(kp - s) < 0.5 for s in S)
        col = DIM if at_station else WARN
        out.append(
            f'<circle cx="{X(kp)}" cy="{Y(f(kp))}" r="4" fill="{col}" '
            f'stroke="#ffffff" stroke-width="1.4"/>')
    # Legend, in its own strip BELOW the axis.  It used to sit in the top-left
    # of the plot area, where the "p=3" label landed and the two collided.  The
    # axis band is empty everywhere, so the legend is unambiguous there.
    # Bands under the axis, spaced so nothing can touch:
    #   ticks   at Y(0)+20
    #   legend  at Y(0)+48  (with a hairline at Y(0)+34)
    #   note    at legend + 24
    lx, ly = pad_l, Y(0) + 48
    out.append(
        f'<line x1="{pad_l}" y1="{ly-16}" x2="{width-pad_r}" y2="{ly-16}" '
        f'stroke="{PANELB}" stroke-width="0.8"/>')
    out.append(
        f'<circle cx="{lx+4}" cy="{ly-4}" r="4" fill="{DIM}" '
        f'stroke="#ffffff" stroke-width="1.4"/>')
    out.append(
        f'<text x="{lx+15}" y="{ly}" font-size="11" fill="{DIM}">'
        f'turn at a station</text>')
    out.append(
        f'<circle cx="{lx+175}" cy="{ly-4}" r="4" fill="{WARN}" '
        f'stroke="#ffffff" stroke-width="1.4"/>')
    out.append(
        f'<text x="{lx+186}" y="{ly}" font-size="11" fill="{WARN}">'
        f'turn inside a gap</text>')

    if note:
        # Anchored to the legend strip, not to the canvas bottom, so the two
        # can never collide if the plot is ever resized.
        nlines = wrap(note, width, 11.5, pad_l)
        for i, ln in enumerate(nlines):
            out.append(
                f'<text x="{pad_l}" y="{ly+26+i*15}" font-size="11.5" '
                f'fill="{DIM}">{esc(ln)}</text>')
    out.append("</svg>")
    path = os.path.join(HERE, name)
    with open(path, "w") as fh:
        fh.write("\n".join(out))
    print(f"  wrote {os.path.relpath(path, os.path.dirname(HERE))}")
    return path


def _selfcheck(paths):
    """Fail loudly on the two layout bugs that actually happened.

    1. Two <text> boxes overlapping.  A vision pass over the PDF reported the
       pages were clean while the figures were visibly broken, because the SVGs
       are scaled down to fit the text block -- at 100 dpi a 2px collision in
       an 880px-wide figure is a fraction of a pixel.  So the geometry is
       checked here instead of being looked at.
    2. A text line running past the right edge of the canvas, which SVG simply
       clips.  The envelope note was 389px too long and lost its ending.
    """
    bad = 0
    for fn in paths:
        s = open(fn).read()
        head = re.search(r'width="([\d.]+)"\s+height="([\d.]+)"', s)
        W = float(head.group(1))
        boxes = []
        for m in re.finditer(r'<text([^>]*)>([^<]*)</text>', s):
            attrs, body = m.group(1), m.group(2)
            if not body.strip():
                continue
            xm = re.search(r'\bx="([\d.]+)"', attrs)
            ym = re.search(r'\by="([\d.]+)"', attrs)
            fm = re.search(r'font-size="([\d.]+)"', attrs)
            am = re.search(r'text-anchor="(\w+)"', attrs)
            if not (xm and ym):
                continue
            x, y = float(xm.group(1)), float(ym.group(1))
            fs = float(fm.group(1)) if fm else 12.0
            w = len(body) * fs * 0.55
            anc = am.group(1) if am else "start"
            x0 = x - w / 2 if anc == "middle" else x
            boxes.append((x0, y - fs * 0.8, x0 + w, y + fs * 0.25, body))
            if x0 + w > W + 1:
                print(f"  FAIL {os.path.basename(fn)}: text runs off the "
                      f"canvas ({x0 + w:.0f} > {W:.0f}): {body[:50]!r}")
                bad += 1
        for i in range(len(boxes)):
            for j in range(i + 1, len(boxes)):
                a, b = boxes[i], boxes[j]
                ox = min(a[2], b[2]) - max(a[0], b[0])
                oy = min(a[3], b[3]) - max(a[1], b[1])
                if ox > 0 and oy > 0:
                    print(f"  FAIL {os.path.basename(fn)}: labels overlap "
                          f"({ox:.0f}x{oy:.0f}px): {a[4]!r} <-> {b[4]!r}")
                    bad += 1
    if bad:
        raise SystemExit(f"{bad} figure layout problem(s); see above")
    print("  layout self-check: no overlapping labels, nothing off-canvas")


def main():
    print("generating editorial figures")

    road_svg(
        "fig-sample.svg", l=5,
        stations=((1, 2), (2, 3), (3, 5)),
        broken=(2,), p=3,
        route=((3, 2, "1 s"), (2, 0, "2 s"), (0, 2, "2 s"),
               (2, 4, "2 s"), (4, 5, "1 s"), (5, 4, "1 s")),
        title="The official sample:  l = 5,  k = 2,  station 2 (at x = 3) is broken",
        note="Every leg walked is also cleared, so the total is just the length "
             "of the walk:  1+2+2+2+1+1 = 9.",
        height=300,
    )

    road_svg(
        "fig-gaps.svg", l=12,
        stations=((1, 1), (2, 4), (3, 7), (4, 10)),
        title="Gaps between working stations:  G0 and G4 are road ends, G1..G3 are interior",
        note="A road-end gap is serviced from one end only.  An interior gap "
             "may be entered from either end, and that is the difference "
             "between the E and I cost families.",
        height=230,
    )

    # C_s and the queried p are the REAL values recovered from the verified
    # oracle (reference/day_model.py), not hand-picked: C_s = max_p (answer(p) -
    # |p - S_s|), which reproduces answer(p) at every p.  An earlier version
    # used invented C values that made the envelope turn only at stations, and
    # the caption then said something the real curve contradicts -- the two
    # lines min(p + A_t, -p + B_t) cross at p* = (B_t - A_t)/2, which is in
    # general strictly inside a gap.  The sample alone shows it: p = 3 between
    # stations 2 and 5.
    envelope_svg(
        "fig-envelope.svg", l=12, S=[1, 4, 7, 10],
        C=[19, 20, 21, 20], ps=[3, 6],
        title="answer(p) = min over s of ( |p - S_s| + C_s )  --  a lower envelope of V-shapes",
        note="Inside a gap the envelope is exactly two lines, min(p + A_t, -p + B_t); "
             "they cross at (B_t - A_t)/2, which need not be a station.  A_t and "
             "B_t contain no p, so a segment tree can hold them.",
    )

    road_svg(
        "fig-updates.svg", l=12,
        stations=((1, 1), (2, 4), (3, 7), (4, 10)),
        broken=(3,),
        title="A breakage at station 3 changes at most three leaves",
        note="Leaf 3 (the station itself), leaf 2 (its predecessor's following "
             "gap changes length), and leaf 0 if the first working station moved.",
        height=230,
    )

    _selfcheck(sorted(glob.glob(os.path.join(HERE, "fig-*.svg"))))
    print("done")


if __name__ == "__main__":
    main()

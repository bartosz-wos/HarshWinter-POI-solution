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
import os

HERE = os.path.dirname(os.path.abspath(__file__))

INK = "#1a1a2e"
DIM = "#6b7280"
ROAD = "#374151"
OK = "#0d9488"
BAD = "#dc2626"
ACCENT = "#2563eb"
SNOW = "#93c5fd"
WARN = "#b45309"
BG = "#ffffff"


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def _head(width, height):
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" '
        f'height="{height}" viewBox="0 0 {width} {height}" '
        f'font-family="DejaVu Sans, Helvetica, Arial, sans-serif">'
        f'<rect width="{width}" height="{height}" fill="{BG}"/>'
    )


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

    # ---- start marker: a callout ABOVE, clear of everything else ----
    if p is not None:
        out.append(
            f'<line x1="{X(p)}" y1="{y-15}" x2="{X(p)}" y2="{y-60}" '
            f'stroke="{ACCENT}" stroke-width="1.4" stroke-dasharray="3 3"/>')
        out.append(
            f'<circle cx="{X(p)}" cy="{y-68}" r="7" fill="{ACCENT}"/>')
        out.append(
            f'<text x="{X(p)}" y="{y-72}" font-size="11" font-weight="700" '
            f'fill="#ffffff" text-anchor="middle">p</text>')
        out.append(
            f'<text x="{X(p)}" y="{y-85}" font-size="11.5" font-weight="700" '
            f'fill="{ACCENT}" text-anchor="middle">start</text>')

    # ---- route legs, numbered, in two lanes well below everything ----
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
        # numbered marker at the left of each leg
        out.append(
            f'<circle cx="{x1}" cy="{ly}" r="8.5" fill="{ACCENT}"/>')
        out.append(
            f'<text x="{x1}" y="{ly+3.8}" font-size="10" font-weight="700" '
            f'fill="#ffffff" text-anchor="middle">{i+1}</text>')
        if cost is not None:
            out.append(
                f'<text x="{(x1+x2)/2}" y="{ly-13}" font-size="11.5" '
                f'font-weight="600" fill="{ACCENT}" text-anchor="middle">'
                f'{cost}</text>')

    if note:
        out.append(
            f'<text x="{pad_l}" y="{height-14}" font-size="11.5" '
            f'fill="{DIM}">{esc(note)}</text>')

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
    pad_l, pad_r, pad_t, pad_b = 74.0, 44.0, 52.0, 66.0
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
    # legend
    lx, ly = pad_l + 6, pad_t + 14
    out.append(
        f'<circle cx="{lx}" cy="{ly-4}" r="4" fill="{DIM}" '
        f'stroke="#ffffff" stroke-width="1.4"/>')
    out.append(
        f'<text x="{lx+11}" y="{ly}" font-size="11" fill="{DIM}">'
        f'turn at a station</text>')
    out.append(
        f'<circle cx="{lx+140}" cy="{ly-4}" r="4" fill="{WARN}" '
        f'stroke="#ffffff" stroke-width="1.4"/>')
    out.append(
        f'<text x="{lx+151}" y="{ly}" font-size="11" fill="{WARN}">'
        f'turn inside a gap</text>')

    if note:
        out.append(
            f'<text x="{pad_l}" y="{height-13}" font-size="11.5" '
            f'fill="{DIM}">{esc(note)}</text>')
    out.append("</svg>")
    path = os.path.join(HERE, name)
    with open(path, "w") as fh:
        fh.write("\n".join(out))
    print(f"  wrote {os.path.relpath(path, os.path.dirname(HERE))}")
    return path


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

    print("done")


if __name__ == "__main__":
    main()

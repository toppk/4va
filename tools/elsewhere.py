#!/usr/bin/env python3
"""Generate ELSEWHERE: sparse botanical, architectural, and animal wireframes."""

import argparse
from math import cos, sin, pi, tau
from pathlib import Path

from cosmic import Shape
from minimal import affine_rank, turn

ROOT = Path(__file__).resolve().parents[1]


def bloom():
    s = Shape('ShimmerBloom')
    # Separate petal outlines surround an empty throat: no central spoke hub.
    for j in range(7):
        angle = tau*j/7
        points = []
        for i in range(12):
            t = tau*i/12
            radial = .56 + .36*cos(t)
            lateral = .16*sin(t)
            points.append((radial*cos(angle)-lateral*sin(angle),
                           .30+radial*sin(angle)+lateral*cos(angle),
                           .16*sin(t)+.12*cos(angle), .32*sin(angle+t)))
        s.strand(points, True)
    s.strand([(.07*sin(i*.5), -.02-i*.16, .12*sin(i*.4), .18*cos(i*.4))
              for i in range(8)])
    s.strand([(0, -.65, 0, .1), (.38, -.48, .08, .32),
              (.25, -.83, .12, .24), (.02, -.81, .04, .1)], True)
    return s


def reeds():
    s = Shape('GlassReeds')
    for j in range(5):
        start = len(s.points)
        height = [1.15, 1.65, 1.4, 1.8, 1.2][j]
        for side in (-1, 1):
            s.strand([((j-2)*.34 + side*.08*(1-.7*u)+.17*sin(3*u+j),
                       -.85+height*u, .12*cos(j+u*3)+side*.06,
                       .30*sin(j*.9+u*2)+side*.09)
                      for u in (i/5 for i in range(6))])
        s.lines.extend((start+i, start+6+i) for i in (0, 2, 4, 5))
    return s


def relic():
    s = Shape('SuspendedRelic')
    for j, (radius, height) in enumerate([(.86, .45), (1, .27), (.72, .04), (.16, -.87)]):
        s.strand([(radius*cos(t), height, radius*.48*sin(t),
                   .30*sin(t+j*.65)) for t in (tau*i/6 for i in range(6))], True)
    s.lines.extend((j*6+i, (j+1)*6+i) for j in range(3) for i in range(6))
    return s


def sail():
    s = Shape('HorizonSail')
    for j in range(3):
        s.strand([(x, .16*x*x+(j-1)*.09, (j-1)*(.32-.15*abs(x)),
                   .32*sin(x*2+(j-1)*.8)) for x in (-1, -.75, -.5, -.25, 0, .25, .5, .75, 1)])
    s.lines.extend((j*9+i, (j+1)*9+i) for j in range(2) for i in range(9))
    s.strand([(-.8, -.26, -.1, -.25), (.65, -.26, -.1, .25),
              (.9, -.37, .1, .32), (-.5, -.37, .1, -.3)], True)
    s.lines.extend([(0, 27), (8, 28), (18, 30), (26, 29)])
    return s


def bird():
    s = Shape('EstuaryBird')
    for side in (-1, 1):
        outline = [(.10, .18), (.35, .40), (.68, .57), (1.18, .65),
                   (1.04, .41), (.89, .19), (.70, -.02), (.49, -.16),
                   (.25, -.12), (.11, -.02)]
        s.strand([(side*x, y, .18*sin(x*2), side*.35*x)
                  for x, y in outline], True)
        for i in range(5):
            x = .34+i*.15
            s.strand([(side*x, .17+x*.37, .18*sin(x*2), side*.35*x),
                      (side*(x+.13), -.19+x*.49, .12, side*(.35*x+.14))])
    s.strand([(.12*cos(t), .03+.35*sin(t), .08*cos(t), .11*sin(2*t))
              for t in (tau*i/12 for i in range(12))], True)
    s.strand([(.065*cos(t), .45+.12*sin(t), .03, .12+.06*cos(t))
              for t in (tau*i/8 for i in range(8))], True)
    s.strand([(-.045, .52, .03, .13), (0, .68, .04, .22), (.045, .52, .03, .13)])
    s.strand([(-.08, -.23, .02, -.08), (-.22, -.58, .05, -.3),
              (0, -.49, -.03, -.05), (.22, -.58, .05, .3), (.08, -.23, .02, .08)])
    return s


def turtle():
    s = Shape('TidalTurtle')
    for j, radius in enumerate((1, .73, .37)):
        s.strand([(.58*radius*cos(t), .72*radius*sin(t), .08+j*.16,
                   .25*sin(2*t)*(1-j*.18)) for t in (tau*i/12 for i in range(12))], True)
    s.lines.extend((j*12+i, (j+1)*12+i) for j in range(2) for i in range(12))
    for side in (-1, 1):
        for front in (-1, 1):
            s.strand([(side*.41, front*.38, .06, side*.12),
                      (side*.76, front*.56, -.03, side*.33),
                      (side*.96, front*.42, -.12, side*.4),
                      (side*.71, front*.18, -.08, side*.27),
                      (side*.48, front*.17, .02, side*.08)], True)
    s.strand([(.16*cos(t), .83+.23*sin(t), .04+.03*cos(t), .16+.09*sin(t))
              for t in (tau*i/8 for i in range(8))], True)
    s.strand([(-.08, -.67, .05, 0), (0, -.97, -.06, -.26),
              (.08, -.67, .05, 0)])
    return s


LABELS = [('shimmerbloom', 'Shimmer bloom', bloom), ('glassreeds', 'Glass reeds', reeds),
          ('skyrelic', 'Suspended relic', relic), ('horizonsail', 'Horizon sail', sail),
          ('estuarybird', 'Estuary bird', bird), ('tidalturtle', 'Tidal turtle', turtle)]


def models():
    result = {}
    for name, label, build in LABELS:
        s = build()
        center = [sum(p[k] for p in s.points)/len(s.points) for k in range(4)]
        s.points = [tuple(v-c for v, c in zip(p, center)) for p in s.points]
        s.normalize()
        assert affine_rank(s.points) == 4
        assert 40 <= len(s.lines) <= 150
        assert len(set(tuple(sorted(e)) for e in s.lines)) == len(s.lines)
        result[name] = s
    return result


def preview(shapes):
    rows = ['<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="1630" viewBox="0 0 1000 1630" role="img" aria-labelledby="title desc">',
            '<title id="title">Elsewhere: six forms in four dimensions</title>',
            '<desc id="desc">Botanical, architectural, and animal outlines shown in a silhouette view and a rotated four-dimensional view.</desc>',
            '<rect width="1000" height="1630" fill="#08101c"/>',
            '<g fill="#e3ecfa" font-family="sans-serif"><text x="40" y="45" font-size="26">ELSEWHERE / familiar forms, unfamiliar space</text>',
            '<text x="40" y="74" font-size="14">Six sparse studies · silhouette at left / XW and YZ rotation at right</text></g>']
    colors = ['#e5aacd', '#b7deb0', '#c3e3f4', '#e8d8ba', '#b3c7ff', '#8cd7bf']
    groups = ['SHIMMER / botanical', 'HORIZON / architectural', 'ESTUARY / animal']
    for index, (name, label, _) in enumerate(LABELS):
        cy = 200+index*240
        s = shapes[name]
        color = colors[index]
        if index % 2 == 0:
            rows.append(f'<text x="40" y="{cy-94}" fill="#a7b9ce" font-family="sans-serif" font-size="13">{groups[index//2]}</text>')
        for view in range(2):
            pts = [turn(p, [(0, 3, view*.95), (1, 2, .16+view*.6)]) for p in s.points]
            pts = [(470+view*350+110*p[0], cy-100*p[1]) for p in pts]
            path = ' '.join(f'M{pts[a][0]:.2f},{pts[a][1]:.2f}L{pts[b][0]:.2f},{pts[b][1]:.2f}' for a, b in s.lines)
            rows.append(f'<path d="{path}" fill="none" stroke="{color}" stroke-width="1.3" stroke-linejoin="round" opacity=".9"/>')
        rows.append(f'<g fill="#e3ecfa" font-family="monospace"><text x="40" y="{cy-20}" font-size="18">{name}.4vd</text><text x="40" y="{cy+5}" font-size="13">{label}</text><text x="40" y="{cy+29}" font-size="12">{len(s.points)} points / {len(s.lines)} lines</text></g>')
    return '\n'.join(rows+['</svg>'])+'\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='verify generated files without writing')
    args = parser.parse_args()
    shapes = models()
    artifacts = {ROOT/'data'/f'{name}.4vd': s.serialize() for name, s in shapes.items()}
    artifacts[ROOT/'doc'/'elsewhere-preview.svg'] = preview(shapes)
    for path, contents in artifacts.items():
        if args.check:
            if not path.exists() or path.read_text() != contents:
                raise SystemExit(f'Out of date: {path}')
        else:
            path.write_text(contents)
    for name, s in shapes.items():
        print(f'{name}: {len(s.points)} points, {len(s.lines)} lines, 4D span')


if __name__ == '__main__':
    main()

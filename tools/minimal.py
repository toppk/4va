#!/usr/bin/env python3
"""Generate four sparse structures whose vertices span all four dimensions."""

import argparse
from math import cos, sin, tau, sqrt
from pathlib import Path

from cosmic import Shape

ROOT = Path(__file__).resolve().parents[1]


def turn(point, angles):
    point = list(point)
    for a, b, angle in angles:
        point[a], point[b] = (point[a]*cos(angle)-point[b]*sin(angle),
                              point[a]*sin(angle)+point[b]*cos(angle))
    return tuple(point)


def affine_rank(points):
    rows = [[v-o for v, o in zip(p, points[0])] for p in points[1:]]
    rank = 0
    for col in range(4):
        pivot = next((i for i in range(rank, len(rows)) if abs(rows[i][col]) > 1e-7), None)
        if pivot is None:
            continue
        rows[rank], rows[pivot] = rows[pivot], rows[rank]
        scale = rows[rank][col]
        rows[rank] = [v/scale for v in rows[rank]]
        for i in range(rank+1, len(rows)):
            scale = rows[i][col]
            rows[i] = [v-scale*u for v, u in zip(rows[i], rows[rank])]
        rank += 1
    return rank


def models():
    walk = Shape('FourAxisWalk')
    point = [-.5]*4
    for sign in (1, -1):
        for axis in range(4):
            walk.points.append(tuple(point))
            point[axis] += sign
    walk.lines = [(i, (i+1) % 8) for i in range(8)]

    gates = Shape('OrthogonalGates')
    corners = [(-.65, -.65), (.65, -.65), (.65, .65), (-.65, .65)]
    gates.strand([(a, b, 0, 0) for a, b in corners], closed=True)
    gates.strand([(0, 0, a, b) for a, b in corners], closed=True)
    gates.lines.extend((i, i+4) for i in range(4))

    book = Shape('FourPageBook')
    book.points = [(-.6, 0, 0, 0), (.6, 0, 0, 0)]
    book.lines = [(0, 1)]
    for direction in [(1, 0, 0), (0, 1, 0), (0, 0, 1), (-1/sqrt(3),)*3]:
        start = len(book.points)
        book.points.extend([(-.6, *direction), (.6, *direction)])
        book.lines.extend([(0, start), (start, start+1), (start+1, 1)])

    star = Shape('PentagramLift')
    star.points = [(cos(tau*i/5), sin(tau*i/5), 0, 0) for i in range(5)]
    star.points.extend((0, 0, cos(tau*i/5), sin(tau*i/5)) for i in range(5))
    star.lines = ([(i, (i+1) % 5) for i in range(5)] +
                  [(5+i, 5+(i+2) % 5) for i in range(5)] +
                  [(i, 5+i) for i in range(5)])

    shapes = dict(zip(('axiswalk', 'orthogates', 'fourbook', 'pentalift'),
                      (walk, gates, book, star)))
    for shape in shapes.values():
        # Center and give the saved object an oblique initial pose, so axes
        # and pages do not disappear into the initial XY projection.
        center = [sum(p[k] for p in shape.points)/len(shape.points) for k in range(4)]
        shape.points = [turn(tuple(v-c for v, c in zip(p, center)),
                             [(0, 2, .53), (1, 3, .71), (1, 2, .38)])
                        for p in shape.points]
        shape.normalize()
        assert affine_rank(shape.points) == 4
        assert len(set(tuple(sorted(edge)) for edge in shape.lines)) == len(shape.lines)
    return shapes


def preview(shapes):
    rows = ['<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="1040" viewBox="0 0 1000 1040">',
            '<rect width="1000" height="1040" fill="#08101c"/>',
            '<g fill="#e3ecfa" font-family="sans-serif"><text x="40" y="45" font-size="26">SMALL / four-dimensional structures</text>',
            '<text x="40" y="74" font-size="14">Two orthographic views of each object; the second rotates XW and YZ.</text></g>']
    for index, (name, shape) in enumerate(shapes.items()):
        cy = 178 + index*240
        color = ['#8cdfff', '#ffc28c', '#a9e9b2', '#c8b0ff'][index]
        for view in range(2):
            pts = [turn(p, [(0, 3, view*1.1), (1, 2, view*.7)]) for p in shape.points]
            pts = [(450+view*350+105*p[0], cy-100*p[1]) for p in pts]
            path = ' '.join(f'M{pts[a][0]:.2f},{pts[a][1]:.2f}L{pts[b][0]:.2f},{pts[b][1]:.2f}' for a, b in shape.lines)
            rows.append(f'<path d="{path}" fill="none" stroke="{color}" stroke-width="1.7"/>')
            for x, y in pts:
                rows.append(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="3" fill="{color}"/>')
        rows.append(f'<g fill="#e3ecfa" font-family="monospace"><text x="40" y="{cy}" font-size="18">{name}.4vd</text><text x="40" y="{cy+26}" font-size="13">{len(shape.points)} vertices / {len(shape.lines)} edges</text></g>')
    return '\n'.join(rows + ['</svg>']) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='verify generated files without writing')
    args = parser.parse_args()
    shapes = models()
    artifacts = {ROOT/'data'/f'{name}.4vd': shape.serialize() for name, shape in shapes.items()}
    artifacts[ROOT/'doc'/'minimal-preview.svg'] = preview(shapes)
    for path, contents in artifacts.items():
        if args.check:
            if not path.exists() or path.read_text() != contents:
                raise SystemExit(f'Out of date: {path}')
        else:
            path.write_text(contents)
    for name, shape in shapes.items():
        print(f'{name}: {len(shape.points)} vertices, {len(shape.lines)} edges, affine dimension 4')


if __name__ == '__main__':
    main()

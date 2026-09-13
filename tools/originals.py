#!/usr/bin/env python3
"""Preview the originals or reconstruct their geometry from formulas.

The historical .4vd files were NOT created by this tool. This is a modern
reconstruction, not recovered authoring code. See data/ORIGINALS.md.
"""

import argparse
from html import escape
from itertools import combinations, permutations, product
from math import cos, dist, isfinite, sin, sqrt, tau
from pathlib import Path
import re

from minimal import affine_rank, turn
from cosmic import Shape

ROOT = Path(__file__).resolve().parents[1]
GROUPS = [
    ('POLYTOPES / finite frameworks', [
        ('5cell', '4-simplex'), ('hcube', 'Tesseract'),
        ('16cell', 'Cross-polytope'), ('24cell', '24-cell'),
        ('600cell', '600-cell'), ('tripris', 'Triangle × triangle')]),
    ('TORI AND CIRCLES / sampled round structures', [
        ('ctor', 'Clifford torus'), ('ctor2', 'One family of torus circles'),
        ('hsph', 'Six coordinate great circles')]),
    ('SURFACE / a three-dimensional height field', [('sin', 'Sine-wave grid')]),
]


def models():
    """Construct all ten objects without reading any existing .4vd file."""
    result = {}

    def polytope(name, title, points):
        shape = Shape(title)
        shape.points = list(points)
        pairs = [(dist(shape.points[a], shape.points[b]), a, b)
                 for a, b in combinations(range(len(shape.points)), 2)]
        length = min(d for d, _, _ in pairs)
        shape.lines = [(a, b) for d, a, b in pairs if abs(d-length) < 1e-9]
        result[name] = shape

    a = .9
    polytope('5cell', '5-Cell', [
        (-a, -a/sqrt(3), -a/sqrt(6), -a/sqrt(10)),
        (a, -a/sqrt(3), -a/sqrt(6), -a/sqrt(10)),
        (0, 2*a/sqrt(3), -a/sqrt(6), -a/sqrt(10)),
        (0, 0, 3*a/sqrt(6), -a/sqrt(10)),
        (0, 0, 0, 4*a/sqrt(10))])
    polytope('hcube', 'Hypercube', product((-.5, .5), repeat=4))
    axes = [tuple(sign if k == axis else 0 for k in range(4))
            for axis in range(4) for sign in (-1, 1)]
    polytope('16cell', '16-Cell', axes)
    points = []
    for i, j in combinations(range(4), 2):
        for u, v in product((-1/sqrt(2), 1/sqrt(2)), repeat=2):
            point = [0]*4
            point[i], point[j] = u, v
            points.append(tuple(point))
    polytope('24cell', '24-Cell', points)
    phi = (1+sqrt(5))/2
    seed = (phi/2, .5, 1/(2*phi), 0)
    points = axes + list(product((-.5, .5), repeat=4))
    for perm in permutations(range(4)):
        inversions = sum(perm[i] > perm[j] for i, j in combinations(range(4), 2))
        if inversions % 2:
            continue
        for signs in product((-1, 1), repeat=3):
            signs = iter(signs)
            points.append(tuple(seed[i]*next(signs) if seed[i] else 0 for i in perm))
    polytope('600cell', '600-Cell', points)
    triangle = [(-.5, -1/(2*sqrt(3))), (.5, -1/(2*sqrt(3))), (0, 1/sqrt(3))]
    polytope('tripris', 'triprism', [a+b for a in triangle for b in triangle])

    # Same circle-product formulas as Matt Welsh's ctorus/cutctorus generators.
    for name, title in [('ctor', 'CliffordTorus-20x20'), ('ctor2', 'CutCliffordTorus-20x20')]:
        shape = Shape(title)
        shape.points = [(cos(tau*i/20), sin(tau*i/20), cos(tau*j/20), sin(tau*j/20))
                        for j in range(20) for i in range(20)]
        shape.lines = [(j*20+i, j*20+(i+1) % 20) for j in range(20) for i in range(20)]
        if name == 'ctor':
            shape.lines.extend((j*20+i, ((j+1) % 20)*20+i) for j in range(20) for i in range(20))
        result[name] = shape

    shape = Shape('Hypersphere')
    for a, b in combinations(range(4), 2):
        points = []
        for i in range(16):
            point = [0]*4
            point[a], point[b] = sin(tau*i/16), cos(tau*i/16)
            points.append(tuple(point))
        shape.strand(points, closed=True)
    result['hsph'] = shape

    # Height field from Matt Welsh's 4vdmake, with its original axis placement.
    shape = Shape('4VDMMAKEV2.1.2CREATED')
    shape.points = [(i/10-1, (sin((i/10-1)*15)+sin((j/10-1)*15))/5, j/10-1, 0)
                    for j in range(20) for i in range(20)]
    shape.lines = ([(j*20+i, j*20+i+1) for j in range(20) for i in range(19)] +
                   [(j*20+i, (j+1)*20+i) for j in range(19) for i in range(20)])
    result['sin'] = shape
    return result


def verify_originals(shapes):
    """Compare geometry, allowing historical decimal precision and ordering."""
    for name, shape in shapes.items():
        points, edges = read_object(ROOT/'data'/f'{name}.4vd')
        if len(points) != len(shape.points) or len(edges) != len(shape.lines):
            raise ValueError(f'{name}: point/line counts differ from reconstruction')
        # hsph repeats shared axis points in its six circles; preserve that
        # multiplicity by using its known circle order, not coordinate welding.
        if name == 'hsph':
            mapping = list(range(len(points)))
        else:
            mapping = [min(range(len(shape.points)), key=lambda i: dist(p, shape.points[i]))
                       for p in points]
        if len(set(mapping)) != len(points) or any(
                dist(p, shape.points[i]) > 1e-5 for p, i in zip(points, mapping)):
            raise ValueError(f'{name}: coordinates differ from reconstruction')
        mapped_edges = {tuple(sorted((mapping[a], mapping[b]))) for a, b in edges}
        expected = {tuple(sorted(edge)) for edge in shape.lines}
        if mapped_edges != expected:
            raise ValueError(f'{name}: connectivity differs from reconstruction')
        print(f'{name}: formula matches original coordinates and connectivity')


def read_object(path):
    rows = path.read_text().splitlines()
    position = 0

    def section(letter, width, convert):
        nonlocal position
        match = re.fullmatch(rf'{letter}=\{{\s*(\d+)\s*\}}', rows[position].strip())
        if not match:
            raise ValueError(f'{path}: expected {letter} header at line {position+1}')
        count = int(match[1])
        position += 1
        result = [tuple(map(convert, row.split())) for row in rows[position:position+count]]
        position += count
        if len(result) != count or any(len(row) != width for row in result):
            raise ValueError(f'{path}: incorrect {letter} section size')
        return result

    points = section('p', 4, float)
    edges = section('l', 2, int)
    # tripris uses the historical spelling "name=" rather than "n=".
    if not re.fullmatch(r'(?:n|name)=\S+', rows[position].strip()):
        raise ValueError(f'{path}: missing object name')
    if any(row.strip() for row in rows[position+1:]):
        raise ValueError(f'{path}: unexpected trailing data')
    if not all(isfinite(v) for point in points for v in point):
        raise ValueError(f'{path}: non-finite coordinates')
    if not all(0 <= a < len(points) and 0 <= b < len(points) and a != b for a, b in edges):
        raise ValueError(f'{path}: invalid edge index')
    return points, edges


def preview():
    height = 150 + len(GROUPS)*65 + 10*235
    rows = [f'<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="{height}" viewBox="0 0 1000 {height}" role="img" aria-labelledby="title desc">',
            '<title id="title">Original 4va collection</title>',
            '<desc id="desc">Ten original wireframes grouped by structure, each shown in two rotated orthographic views.</desc>',
            f'<rect width="1000" height="{height}" fill="#08101c"/>',
            '<g fill="#e3ecfa" font-family="sans-serif"><text x="40" y="45" font-size="26">ORIGINALS / the 4va collection</text>',
            '<text x="40" y="74" font-size="14">Two orthographic views · centered and scaled for comparison · source geometry preserved</text>',
            '<text x="440" y="108" font-size="12">VIEW A</text><text x="790" y="108" font-size="12">VIEW B / XW + YZ rotation</text></g>']
    top = 130
    palette = ['#8cdfff', '#ffc28c', '#a9e9b2', '#c8b0ff']
    item = 0
    for title, entries in GROUPS:
        rows.append(f'<path d="M40,{top}H960" stroke="#283a50"/><text x="40" y="{top+32}" fill="#afc1da" font-family="sans-serif" font-size="16">{escape(title)}</text>')
        top += 65
        for name, label in entries:
            points, edges = read_object(ROOT/'data'/f'{name}.4vd')
            dimension = affine_rank(points)
            print(f'{name}: {len(points)} points, {len(edges)} lines, dimension {dimension}')
            center = [sum(p[k] for p in points)/len(points) for k in range(4)]
            points = [tuple(v-c for v, c in zip(p, center)) for p in points]
            radius = max(sqrt(sum(v*v for v in p)) for p in points)
            cy = top + 100
            color = palette[item % len(palette)]
            item += 1
            for view in range(2):
                angles = [(0, 2, .53), (1, 3, .71), (1, 2, .38),
                          (0, 3, view*1.1), (1, 2, view*.7)]
                projected = [turn(p, angles) for p in points]
                projected = [(475+view*350+110*p[0]/radius, cy-110*p[1]/radius) for p in projected]
                path = ' '.join(f'M{projected[a][0]:.2f},{projected[a][1]:.2f}L{projected[b][0]:.2f},{projected[b][1]:.2f}' for a, b in edges)
                dense = len(edges) > 200
                rows.append(f'<path d="{path}" fill="none" stroke="{color}" stroke-width="{.65 if dense else 1.3}" opacity="{.65 if dense else .9}"/>')
                if len(points) <= 24:
                    for x, y in projected:
                        rows.append(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="2.5" fill="{color}"/>')
            rows.append(f'<g fill="#e3ecfa" font-family="monospace"><text x="40" y="{cy-24}" font-size="18">{name}.4vd</text><text x="40" y="{cy}" font-size="13">{escape(label)}</text><text x="40" y="{cy+24}" font-size="12">{len(points)} points / {len(edges)} lines / {dimension}D span</text></g>')
            top += 235
    return '\n'.join(rows + ['</svg>']) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true',
                        help='verify original geometry and SVG, or --output-dir artifacts, without writing')
    parser.add_argument('--output-dir', type=Path,
                        help='generate reconstructed .4vd files in a separate directory; no source files needed')
    args = parser.parse_args()
    shapes = models()
    if args.output_dir is not None:
        destination = args.output_dir.resolve()
        if destination == (ROOT/'data').resolve():
            parser.error('--output-dir must differ from data/ to preserve the historical files')
        if not args.check:
            destination.mkdir(parents=True, exist_ok=True)
        for name, shape in shapes.items():
            path = destination/f'{name}.4vd'
            contents = shape.serialize()
            if args.check:
                if not path.exists() or path.read_text() != contents:
                    raise SystemExit(f'Out of date: {path}')
            else:
                path.write_text(contents)
            print(f'{name}: {len(shape.points)} points, {len(shape.lines)} lines')
        return
    if args.check:
        verify_originals(shapes)
    contents = preview()
    path = ROOT/'doc'/'originals-preview.svg'
    if args.check:
        if not path.exists() or path.read_text() != contents:
            raise SystemExit(f'Out of date: {path}')
    else:
        path.write_text(contents)


if __name__ == '__main__':
    main()

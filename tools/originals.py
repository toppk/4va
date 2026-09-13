#!/usr/bin/env python3
"""Make a grouped contact sheet from the original 4vd files, without editing them."""

import argparse
from html import escape
from math import isfinite, sqrt
from pathlib import Path
import re

from minimal import affine_rank, turn

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
    parser.add_argument('--check', action='store_true', help='verify the SVG without writing')
    args = parser.parse_args()
    contents = preview()
    path = ROOT/'doc'/'originals-preview.svg'
    if args.check:
        if not path.exists() or path.read_text() != contents:
            raise SystemExit(f'Out of date: {path}')
    else:
        path.write_text(contents)


if __name__ == '__main__':
    main()

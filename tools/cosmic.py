#!/usr/bin/env python3
"""Reproduce the stitched-inspired 4vd collection and its vector contact sheet."""

import argparse
import math
from pathlib import Path
import random

TAU = math.tau
sin, cos = math.sin, math.cos
ROOT = Path(__file__).resolve().parents[1]


class Shape:
    def __init__(self, name):
        self.name = name
        self.points = []
        self.lines = []

    def strand(self, points, closed=False):
        start = len(self.points)
        self.points.extend(points)
        end = len(self.points)
        self.lines.extend((i, i + 1) for i in range(start, end - 1))
        if closed:
            self.lines.append((end - 1, start))

    def normalize(self):
        # A bounding 4-ball keeps every orientation manageable in the viewer.
        radius = max(math.sqrt(sum(v * v for v in p)) for p in self.points)
        self.points = [tuple(v * 1.12 / radius for v in p) for p in self.points]

    def serialize(self):
        assert 0 < len(self.name) < 32 and not any(c.isspace() for c in self.name)
        assert all(len(p) == 4 and all(math.isfinite(v) for v in p) for p in self.points)
        assert all(0 <= a < len(self.points) and 0 <= b < len(self.points)
                   and a != b and self.points[a] != self.points[b] for a, b in self.lines)
        rows = [f"p={{{len(self.points)}}}"]
        rows.extend(" ".join(f"{v:.6f}" for v in p) for p in self.points)
        rows.append(f"l={{{len(self.lines)}}}")
        rows.extend(f"{a} {b}" for a, b in self.lines)
        rows.append(f"n={self.name}")
        return "\n".join(rows) + "\n"


def accretion():
    shape = Shape("AccretionSpindle")
    for j in range(27):
        r = 0.24 + 1.55 * (j / 26) ** 1.5
        shape.strand([(r * cos(t), 0.09 * sin(3*t + j*.19), r * sin(t),
                       .16 * cos(2*t + j*.23))
                      for t in (TAU*i/128 for i in range(128))], closed=True)
    for j in range(18):
        phase = TAU*j/18
        for side in (-1, 1):
            points = []
            for i in range(90):
                u = i/89
                r = .13 + .78 * u**3
                t = phase + 5*u
                points.append((r*cos(t), side*(.10+1.65*u), r*sin(t),
                               .38*sin(phase+2*u)*u))
            shape.strand(points)
    return shape


def vortices():
    shape = Shape("BinaryVortices")
    for side in (-1, 1):
        for j in range(15):
            points = []
            for i in range(180):
                u = i/179
                t = side*(TAU*2.7*u + j*.115)
                r = .035 + .73*u*u
                points.append((side*.78 + r*cos(t), r*sin(t),
                               .24*sin(2*t + j*.09)*u, side*.4 + .23*cos(t)*u))
            shape.strand(points)
    for j in range(12):
        shape.strand([(1.75*(u-.5), .16*sin(TAU*u+j*.07)+(j-5.5)*.022,
                       .32*sin(math.pi*u)*cos(j*.15), .4*cos(math.pi*u))
                      for u in (i/99 for i in range(100))])
    return shape


def ribbon():
    shape = Shape("FoldedLightRibbon")
    for j in range(25):
        v = (j/24-.5)*.40
        shape.strand([((.85+v*cos(3*t))*cos(2*t),
                       (.85+v*cos(3*t))*sin(2*t),
                       .48*sin(3*t)+v*sin(3*t), .43*cos(3*t)+v*sin(5*t))
                      for t in (TAU*i/240 for i in range(240))], closed=True)
    return shape


def web():
    shape = Shape("CosmicMycelium")
    rng = random.Random(451)

    def branch(origin, direction, length, depth):
        bend = [rng.uniform(-.5, .5) for _ in range(4)]
        points = [tuple(origin[k] + length*(direction[k]*u + bend[k]*sin(math.pi*u)*.35)
                        for k in range(4)) for u in (i/12 for i in range(13))]
        shape.strand(points)
        if depth:
            for _ in range(3):
                child = [direction[k]*.45 + rng.uniform(-1, 1) for k in range(4)]
                norm = math.sqrt(sum(v*v for v in child))
                branch(points[-1], [v/norm for v in child], length*.58, depth-1)
        else:
            center = points[-1]
            shape.strand([tuple(center[k] + .024*(cos(t) if k == 0 else sin(t) if k == 1 else 0)
                                for k in range(4)) for t in (TAU*i/8 for i in range(8))], True)

    for j in range(7):
        t = TAU*j/7
        branch((0, 0, 0, 0), (cos(t)*.85, sin(t)*.85, .4*sin(2*t), .4*cos(2*t)), .7, 3)
    return shape


def contact_sheet(shapes):
    rows = ['<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="1000" viewBox="0 0 1200 1000">',
            '<rect width="1200" height="1000" fill="#050a14"/>',
            '<g fill="#e1eeff" font-family="sans-serif"><text x="36" y="44" font-size="26">STITCHED / four-dimensional studies</text>',
            '<text x="36" y="72" font-size="14">Static orthographic sketches · open the .4vd files to tumble through W</text></g>']
    colors = ['#9edcff', '#ff986a', '#aab6ff', '#72e7d0']
    for index, (filename, shape) in enumerate(shapes.items()):
        cx, cy = 300 + (index % 2)*600, 300 + (index//2)*450
        projected = []
        for x, y, z, w in shape.points:
            # A little W rotation, then an elevated view of the XZ disk.
            x, w = x*cos(.28)-w*sin(.28), x*sin(.28)+w*cos(.28)
            y = y*cos(.55)-z*sin(.55)
            projected.append((cx+205*x, cy-180*y))
        path = ' '.join(f'M{projected[a][0]:.2f},{projected[a][1]:.2f}L{projected[b][0]:.2f},{projected[b][1]:.2f}'
                        for a, b in shape.lines)
        rows.append(f'<path d="{path}" fill="none" stroke="{colors[index]}" stroke-width=".65" opacity=".65"/>')
        rows.append(f'<text x="{cx-260}" y="{cy+195}" fill="#e1eeff" font-family="monospace" font-size="17">{filename}.4vd</text>')
    rows.append('</svg>')
    return '\n'.join(rows) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='check committed/generated artifacts without writing')
    args = parser.parse_args()
    shapes = dict(zip(('accretion', 'vortices', 'ribbon', 'mycelium'),
                      (accretion(), vortices(), ribbon(), web())))
    artifacts = {}
    for name, shape in shapes.items():
        shape.normalize()
        artifacts[ROOT / 'data' / f'{name}.4vd'] = shape.serialize()
        print(f'{name}: {len(shape.points)} points, {len(shape.lines)} lines')
    artifacts[ROOT / 'doc' / 'cosmic-preview.svg'] = contact_sheet(shapes)
    for path, contents in artifacts.items():
        if args.check:
            if not path.exists() or path.read_text() != contents:
                raise SystemExit(f'Out of date: {path}')
        else:
            path.write_text(contents)


if __name__ == '__main__':
    main()

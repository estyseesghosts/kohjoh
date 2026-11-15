"""Execute the actual lighting shader source with float24 arithmetic.

Checks all specular inputs and LUT bytes, plus random lighting contexts.
This is an arithmetic oracle; emulator image comparison tests the assembled GPU path.
"""
import math
from pathlib import Path
import random
import re
import struct

ROOT = Path(__file__).resolve().parents[2]


def f24(value):
    if value == 0:
        return 0.0
    bits, = struct.unpack('<I', struct.pack('<f', value))
    return struct.unpack('<f', struct.pack('<I', bits & ~127))[0]


class Shader:
    def __init__(self):
        self.constants, self.aliases, self.labels, self.code = {}, {}, {}, []
        source = ROOT / 'src/graphics/pica/shaders/g3d_lit.v.pica'
        for raw in source.read_text().splitlines():
            line = raw.split(';')[0].strip()
            if not line:
                continue
            if line.startswith('.constf '):
                name, values = line[8:].split('(', 1)
                self.constants[name] = [f24(float(x)) for x in values.rstrip(')').split(',')]
            elif line.startswith('.alias '):
                _, alias, register = line.split()
                self.aliases[alias] = register
            elif line.startswith('.'):
                continue
            elif line.endswith(':'):
                self.labels[line[:-1]] = len(self.code)
            else:
                op, *arguments = line.split(None, 1)
                self.code.append((op, [x.strip() for x in arguments[0].split(',')] if arguments else []))
        assert len(self.code) <= 512

    def run(self, uniforms, inputs):
        registers = {**self.constants, **inputs}
        address = {'x': 0, 'y': 0}
        compare = [False, False]
        specular = {}

        def get(token):
            negative = token.startswith('-')
            if negative:
                token = token[1:]
            match = re.fullmatch(r'(\w+(?:\[[^]]+\])?)(?:\.([xyzw]+))?', token)
            assert match, token
            name, swizzle = match.groups()
            if '[' in name:
                name, index = name[:-1].split('[')
                if index.isdigit():
                    index = int(index)
                else:
                    indexed = re.fullmatch(r'a0\.([xy])(?:\s*\+\s*(\d+))?', index)
                    assert indexed, index
                    index = address[indexed[1]] + int(indexed[2] or 0)
                vector = uniforms[name][index]
            else:
                name = self.aliases.get(name, name)
                vector = uniforms.get(name, registers.get(name, [0.0] * 4))
            swizzle = swizzle or 'xyzw'
            if len(swizzle) < 4:
                swizzle += swizzle[-1] * (4 - len(swizzle))
            result = [vector['xyzw'.index(c)] for c in swizzle]
            return [-x for x in result] if negative else result

        def put(token, vector):
            name, _, mask = token.partition('.')
            mask = mask or 'xyzw'
            output = list(registers.get(name, [0.0] * 4))
            for component in mask:
                i = 'xyzw'.index(component)
                output[i] = f24(vector[i])
            registers[name] = output

        comparisons = {'eq': lambda a, b: a == b, 'ne': lambda a, b: a != b,
                       'lt': lambda a, b: a < b, 'le': lambda a, b: a <= b,
                       'gt': lambda a, b: a > b, 'ge': lambda a, b: a >= b}
        watches = {self.labels[f'no_shininess_{i}']: i for i in range(4)}
        pc, steps = 0, 0
        while pc < len(self.code):
            if pc in watches:
                specular[watches[pc]] = get('r8.yyyy')[0]
            op, args = self.code[pc]
            pc += 1
            steps += 1
            assert steps <= 600
            if op == 'end':
                break
            if op == 'jmpc':
                condition = compare[0 if args[0].endswith('x') else 1]
                if args[0].startswith('!'):
                    condition = not condition
                if condition:
                    pc = self.labels[args[1]]
                continue
            if op == 'cmp':
                a, b = get(args[0]), get(args[3])
                compare = [comparisons[args[1]](a[0], b[0]), comparisons[args[2]](a[1], b[1])]
                continue
            if op == 'mova':
                component = args[0][-1]
                address[component] = int(get(args[1])['xyzw'.index(component)])
                continue
            a = get(args[1])
            if op == 'mov':
                result = a
            elif op == 'flr':
                result = [math.floor(x) for x in a]
            elif op in ('dp3', 'dp4'):
                b = get(args[2])
                total = 0.0
                for i in range(3 if op == 'dp3' else 4):
                    total = f24(total + f24(a[i] * b[i]))
                result = [total] * 4
            else:
                b = get(args[2])
                operation = {'add': lambda a, b: a + b, 'mul': lambda a, b: a * b,
                             'min': min, 'max': max, 'sge': lambda a, b: float(a >= b)}[op]
                result = [operation(x, y) for x, y in zip(a, b)]
            put(args[0], result)
        return registers['outclr'], registers.get('r6', [0.0] * 4)[:3], specular


def scenario(shader, normal, directions, halves, ambient, diffuse, specular, emission,
             table=None, alpha=255):
    def split(vectors, low):
        return [[(-x - ((-x >> 7) << 7)) if low else (-x >> 7) for x in v] + [0]
                for v in vectors]
    matrices = [[1.0, 0, 0, 0], [0, 1.0, 0, 0], [0, 0, 1.0, 0], [0, 0, 0, 1.0]] * 6
    uniforms = dict(palette=matrices, texscale=[1, 1, 0, 1],
                    light_dir_hi=split(directions, False), light_dir_lo=split(directions, True),
                    light_half_hi=split(halves, False), light_half_lo=split(halves, True),
                    ambient=[v + [0] for v in ambient], diffuse=[v + [0] for v in diffuse],
                    specular=[v + [0] for v in specular], emission=emission + [0],
                    params=[alpha, int(table is not None), 0, 0],
                    shininess=[(table or [0] * 128)[i:i + 4] for i in range(0, 128, 4)])
    bits = sum((x & 16383) << (14 * i) for i, x in enumerate(normal))
    inputs = {'v0': [0, 0, 0, 0], 'v1': [bits >> (8 * i) & 255 for i in range(4)],
              'v2': [0, 0, 0, 0], 'v3': [(bits >> 32 & 31) << 3, 128 | (bits >> 37 & 31), 0, 0]}
    rgba, decoded, specs = shader.run(uniforms, inputs)
    assert decoded == list(normal), (normal, decoded)
    rgb = list(emission)
    for i in range(4):
        d = max(0, min(4096, -sum(a * b for a, b in zip(normal, directions[i])) >> 12))
        h = max(0, min(4096, -sum(a * b for a, b in zip(normal, halves[i])) >> 12))
        spec = h * h >> 12
        if table is not None:
            spec = table[min(127, spec >> 5)] * 4096 // 255
        assert specs[i] == spec, (i, normal, halves[i], specs[i], spec)
        for c in range(3):
            rgb[c] += ambient[i][c] + ((diffuse[i][c] * d + 2048) >> 12) + ((specular[i][c] * spec + 2048) >> 12)
    expanded = [min(31, x) * 8 + (min(31, x) >> 2) for x in rgb] + [alpha]
    expected = [f24(x * shader.constants['colscale'][0]) for x in expanded]
    assert rgba == expected, (normal, rgba, expected, rgb)


def main():
    shader = Shader()
    # Literal COLOR and strip-boundary colours share the lit program.
    for alpha in range(256):
        colour = [alpha, 255 - alpha, alpha ^ 85, alpha]
        inputs = {'v0': [0, 0, 0, 0], 'v1': colour,
                  'v2': [0, 0, 0, 0], 'v3': [0, 0, 0, 0]}
        uniforms = {'palette': [[1, 0, 0, 0], [0, 1, 0, 0],
                                [0, 0, 1, 0], [0, 0, 0, 1]],
                    'texscale': [1, 1, 0, 1]}
        rgba, _, _ = shader.run(uniforms, inputs)
        assert rgba == [f24(x * shader.constants['colscale'][0]) for x in colour]
    print('PASS PICA source: literal colour and alpha bytes', flush=True)
    zero = [[0, 0, 0] for _ in range(4)]
    half = [[-4096, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0]]
    spec = [[31, 17, 9], [0, 0, 0], [0, 0, 0], [0, 0, 0]]
    for h in range(4097):
        scenario(shader, [h, 0, 0], zero, half, zero, zero, spec, [0, 0, 0])
    print('PASS PICA source: all 4097 fixed-point specular inputs', flush=True)
    for entry in range(256):
        scenario(shader, [4096, 0, 0], zero, half, zero, zero, spec, [0, 0, 0], [entry] * 128)
    print('PASS PICA source: all 256 shininess values', flush=True)
    rng = random.Random(19271)
    for trial in range(1200):
        maximum = 16384 if trial & 1 else 4096
        n = [rng.randrange(-8192, 8192) for _ in range(3)]
        directions = [[rng.randrange(-maximum, maximum) for _ in range(3)] for _ in range(4)]
        halves = [[rng.randrange(-maximum, maximum) for _ in range(3)] for _ in range(4)]
        products = [[[rng.randrange(0, 10) for _ in range(3)] for _ in range(4)] for _ in range(3)]
        table = [rng.randrange(256) for _ in range(128)] if trial & 2 else None
        scenario(shader, n, directions, halves, *products, [rng.randrange(8) for _ in range(3)],
                 table, alpha=rng.randrange(256))
    print(f'PASS PICA source: 1200 random NORMAL contexts, {len(shader.code)} instructions, float24 intermediates', flush=True)


if __name__ == '__main__':
    main()

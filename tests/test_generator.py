"""Exercise the CLI and independently validate small generated designs."""
import itertools
import math
import re
import subprocess
import sys

PROGRAM = sys.argv[1]


def run(*args, success=True):
    result = subprocess.run([PROGRAM, *map(str, args)], capture_output=True,
                            text=True, timeout=30)
    assert (result.returncode == 0) == success, (args, result)
    assert 'Sanitizer' not in result.stderr, result.stderr
    if not success:
        assert result.returncode > 0 and result.stderr, result
    elif 'Usage:' not in result.stdout:
        factorial = math.factorial(int(args[-1]))
        counts = []
        for line in result.stdout.splitlines():
            if line.startswith('Design:'):
                automsize = int(re.search(r'automsize: (\d+)', line).group(1))
                count = int(re.search(r'#DistDesigns: (\d+)\)', line).group(1))
                assert automsize > 0 and factorial % automsize == 0, line
                assert count == factorial // automsize, line
                counts.append(count)
        summary = result.stdout.splitlines()[-1]
        assert summary == f'Total #DistDesigns: {sum(counts)}', result.stdout
    return result.stdout


def check_designs(points, expected_blocks, expected_automorphisms):
    output = run(points)
    lines = [line for line in output.splitlines() if line.startswith('Design:')]
    assert len(lines) == 1, output
    blocks = [tuple(map(int, block.split(', ')))
              for block in re.findall(r'\{([^}]+)\}', lines[0])]
    assert len(blocks) == expected_blocks, output
    pairs = set()
    for block in blocks:
        assert len(set(block)) == 3 and all(0 <= v < points for v in block)
        for pair in itertools.combinations(sorted(block), 2):
            assert pair not in pairs, output
            pairs.add(pair)
    # Four distinct triples on six points with every degree two form a Pasch.
    for four in itertools.combinations(blocks, 4):
        vertices = [v for block in four for v in block]
        assert not (len(set(vertices)) == 6 and
                    all(vertices.count(v) == 2 for v in set(vertices))), output
    automorphisms = int(re.search(r'automsize: (\d+)', output).group(1))
    assert automorphisms == expected_automorphisms, output
    # An independent five-block subset count: a linear triple system is a
    # mitre exactly when its seven points have degrees 3,2,2,2,2,2,2.
    mitres = 0
    for five in itertools.combinations(blocks, 5):
        vertices = [v for block in five for v in block]
        degrees = sorted(vertices.count(v) for v in set(vertices))
        mitres += degrees == [2, 2, 2, 2, 2, 2, 3]
    reported = int(re.search(r', mitres: (\d+),', output).group(1))
    assert reported == mitres, (reported, mitres, output)
    if points == 9:
        assert reported == 36, output
    if points <= 7:
        # Independently count vertex permutations preserving the block set.
        block_set = {tuple(sorted(block)) for block in blocks}
        count = sum({tuple(sorted(perm[v] for v in block)) for block in blocks}
                    == block_set for perm in itertools.permutations(range(points)))
        assert count == automorphisms, (count, output)
    return output


help_text = run('--help')
assert 'Usage:' in help_text
if '-s ' in help_text:
    stats = run('-iste', 3)
    assert '1 complete designs' in stats and 'CPU time' in stats
    assert 'canonicity checks' in stats
if '-p ' in help_text:
    matrices = run('-pcq', 3)
    assert 'Partial Design:' in matrices and 'Design:' in matrices
check_designs(3, 1, 6)
check_designs(5, 2, 8)
check_designs(6, 3, 6)
check_designs(7, 5, 12)
check_designs(9, 12, 432)
assert run(4) == 'Total #DistDesigns: 0\n'
# This search reports two classes. Independently enumerate all labeled
# four-block PSTS covering seven points; none can be a six-point Pasch.
multiple = run('-b1', '-B4', 7)
assert len([line for line in multiple.splitlines() if line.startswith('Design:')]) == 2
triples = list(itertools.combinations(range(7), 3))
labeled_total = 0
for blocks in itertools.combinations(triples, 4):
    vertices = {v for block in blocks for v in block}
    pairs = [pair for block in blocks for pair in itertools.combinations(block, 2)]
    labeled_total += len(vertices) == 7 and len(set(pairs)) == 12
assert labeled_total == 2100
assert multiple.splitlines()[-1] == f'Total #DistDesigns: {labeled_total}'
assert run('-C', '-', 3) == run(3)
assert run('-C', '-0,1-2', 3) == run(3)
assert run('-C', '2-', 3) == run(3)
assert run('-r1', '-R1', '-b1', '-B1', '--', 3) == run(3)
for args in [(), ('abc',), (2,), (65,), ('-1',), ('3x',), ('3', '4'),
             ('9999999999999999999999',), ('-r',), ('-r', 'garbage', 3),
             ('-r', 0, 4), ('-R', 0, 3), ('-B', 0, 3), ('-R', 2, 3), ('-b', 2, 3),
             ('-a', 0, 3), ('-A', 1, 3), ('-P', 3), ('-L', '[]', 3),
             ('-S', '[]', 3), ('-M', '[]', 3)]:
    run(*args, success=False)
for value in ['', '3', '0-3', '2-1', 'a', '1,,2', ',1', '1,', '--', '1-2-3',
              '99999999999999999999']:
    run('-C', value, 3, success=False)
print('CLI, design validity, automorphisms, mitres, labeled totals and invalid-input checks passed')

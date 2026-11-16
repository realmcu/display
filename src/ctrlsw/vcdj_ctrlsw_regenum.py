#!/usr/bin/env python3
#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0
#
"""Census of the register enum that the prune_reg_enum rule cuts down.

    python vcdj_ctrlsw_regenum.py [--cdb FILE] [--out DIR]

Not part of --check; run it after a vendor drop, or when the rule's "keep"
numbers stop matching. It regenerates vcdj_ctrlsw.c with every rule except
prune_reg_enum, preprocesses it with the real compile command from the SCons
compilation database (`scons --cdb`), and classifies each member:

  live     named in the preprocessed code: token pastes and #ifdef resolved
  dormant  named only in text the build drops (a disabled branch, an unused macro)
  alias    named by nobody, but a kept alias points at it
  unused   none of these

The rule matches on text alone, so it keeps live + dormant + alias. This script
gets there through the preprocessor instead, and exits 1 if the two counts
disagree. It also lists every register id that is not a single enum name.
Output goes to DIR (default build/vcdj_regenum/): the .c, the .i and a .tsv.
"""
import argparse
import json
import os
import re
import shlex
import subprocess
import sys
from collections import Counter, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import vcdj_ctrlsw_gen as gen  # noqa: E402
import vcdj_ctrlsw_prune as prune  # noqa: E402

ACCESSORS = ('SetDecRegister', 'GetDecRegister', 'SetPpuRegister', 'GetPpuRegister')


def find_cdb():
    d = HERE
    while os.path.dirname(d) != d:
        p = os.path.join(d, 'build', 'compile_commands.json')
        if os.path.isfile(p):
            return p
        d = os.path.dirname(d)
    sys.exit('vcdj_ctrlsw_regenum: no build/compile_commands.json above %s; '
             'run `scons --cdb` or pass --cdb' % HERE)


def preprocess(cdb, src, out):
    """gcc -E of `src` with the flags the build uses for vcdj_ctrlsw.c."""
    with open(cdb) as f:
        ent = next((e for e in json.load(f) if os.path.basename(e['file']) == 'vcdj_ctrlsw.c'),
                   None)
    if ent is None:
        sys.exit('vcdj_ctrlsw_regenum: %s has no entry for vcdj_ctrlsw.c' % cdb)
    argv = ent['arguments'] if 'arguments' in ent else shlex.split(ent['command'], posix=False)
    i = argv.index('-o')
    argv = argv[:i] + argv[i + 2:]
    argv = [a for a in argv if a != '-c' and os.path.basename(a) != 'vcdj_ctrlsw.c']
    # The copy sits elsewhere, so its "vcdj_ctrlsw.h" has to be found by path.
    argv += ['-I', HERE, '-E', src, '-o', out]
    subprocess.run(argv, cwd=ent['directory'], check=True)


def own_lines(path, name):
    """{source line: text} of the preprocessed lines that come from `name`."""
    out, cur, n = {}, None, 0
    with open(path, encoding='latin-1') as f:
        for l in f:
            m = re.match(r'^# (\d+) "([^"]*)"', l)
            if m:
                n, cur = int(m.group(1)), m.group(2)
                continue
            if cur and os.path.basename(cur.replace('\\\\', '\\')) == name:
                out[n] = out.get(n, '') + re.sub(r'"(?:\\.|[^"\\])*"', '""', l.rstrip('\n'))
            n += 1
    return out


def call_args(text, pos):
    depth, cur, out = 0, '', []
    for ch in text[pos:]:
        if ch == '(':
            depth += 1
        elif ch == ')':
            if depth == 0:
                return out + [cur]
            depth -= 1
        elif ch == ',' and depth == 0:
            out.append(cur)
            cur = ''
            continue
        cur += ch
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--cdb', help='compile_commands.json (default: build/ above this script)')
    ap.add_argument('--out', help='output directory (default: build/vcdj_regenum/ next to the cdb)')
    a = ap.parse_args()
    cdb = a.cdb or find_cdb()
    out = a.out or os.path.join(os.path.dirname(cdb), 'vcdj_regenum')
    os.makedirs(out, exist_ok=True)

    rules = json.loads(prune.read(prune.RULES))['rules']
    rule = next((r for r in rules if r['action'] == 'prune_reg_enum'), None)
    if rule is None:
        sys.exit('vcdj_ctrlsw_regenum: no prune_reg_enum rule in %s' % prune.RULES)
    h, c, _ = gen.generate()
    h, c = prune.apply_rules(h, c, [r for r in rules if r is not rule])
    lines = [l for l, _ in c]
    src, pp = os.path.join(out, 'vcdj_ctrlsw.c'), os.path.join(out, 'vcdj_ctrlsw.i')
    with open(src, 'wb') as f:
        f.write(''.join(l + '\n' for l in lines).encode('latin-1'))
    preprocess(cdb, src, pp)

    e_lo, e_hi, base, alias, t_lo, t_hi, _ = prune._reg_enum(rule, lines, 'vcdj_ctrlsw.c')
    names = [n for n, _ in base[:-1]]
    skip = set(range(e_lo, e_hi + 1)) | set(range(t_lo, t_hi + 1))
    tok = re.compile(r'\b%s\w+' % re.escape(rule['prefix']))
    members = set(names) | set(alias)

    live = defaultdict(list)
    pp_lines = own_lines(pp, 'vcdj_ctrlsw.c')
    for n, l in pp_lines.items():
        if n - 1 not in skip:
            for t in tok.findall(l):
                live[t].append(n)
    raw = defaultdict(list)
    code = prune.code_lines(lines)
    hcode = prune.code_lines([l for l, _ in h])
    for i, l in [(i, l) for i, l in enumerate(code) if i not in skip] + list(enumerate(hcode)):
        for t in tok.findall(l):
            for m in (t, t + '_LSB', t + '_MSB'):
                if m in members:
                    raw[m].append(i + 1)
    cls = {}
    for n in names + list(alias):
        cls[n] = 'live' if live.get(n) else 'dormant' if raw.get(n) else None
    for x, (t, _) in alias.items():
        if cls[x] and not cls[t]:
            cls[t] = 'alias'
    cls = {n: v or 'unused' for n, v in cls.items()}

    tsv = os.path.join(out, 'regenum.tsv')
    with open(tsv, 'w', encoding='utf-8') as f:
        f.write('index\tname\tclass\tlive_refs\tfirst_lines\n')
        for k, n in enumerate(names):
            ls = sorted(set(live.get(n, []) + raw.get(n, [])))
            f.write('%d\t%s\t%s\t%d\t%s\n' % (k, n, cls[n], len(live.get(n, [])),
                                              ','.join(map(str, ls[:6]))))
        for x, (t, _) in alias.items():
            f.write('alias\t%s=%s\t%s\t%d\t\n' % (x, t, cls[x], len(live.get(x, []))))

    body = '\n'.join(l for n, l in sorted(pp_lines.items()) if n - 1 not in skip)
    ids, odd = Counter(), []
    for m in re.finditer(r'\b(%s)\s*\(' % '|'.join(ACCESSORS), body):
        args = call_args(body, m.end())
        idarg = ' '.join(args[1].split()) if len(args) > 1 else '?'
        if re.fullmatch(r'\(?\s*%s\w+\s*\)?' % rule['prefix'], idarg):
            ids['name'] += 1
        elif idarg != 'u32 id':          # the accessor's own definition
            odd.append('%s(.., %s, ..)' % (m.group(1), idarg))
    odd += ['%s[%s]' % (rule['table'], ' '.join(m.group(1).split()))
            for m in re.finditer(r'\b%s\s*\[([^\]]*)\]' % rule['table'], body)]

    count = Counter(cls[n] for n in names)
    acount = Counter(cls[x] for x in alias)
    # Like "keep" in the rule, the count leaves out `last`, which always stays.
    kept = len(names) - count['unused']
    kept_alias = len(alias) - acount['unused']
    print('members %d (without %s): %s' % (len(names), rule['last'], dict(count)))
    print('aliases %d: %s' % (len(alias), dict(acount)))
    print('accessor calls with one name as id: %d' % ids['name'])
    print('ids that are not one name (each needs a look):')
    for o in odd:
        print('  ' + o)
    print('wrote %s' % tsv)
    want = rule['keep']
    if (kept, kept_alias) != (want['members'], want['aliases']):
        sys.exit('vcdj_ctrlsw_regenum: keeps %d members and %d aliases, the rule says '
                 '%d and %d' % (kept, kept_alias, want['members'], want['aliases']))
    print('agrees with the rule: %d members, %d aliases' % (kept, kept_alias))


if __name__ == '__main__':
    main()

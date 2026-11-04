#!/usr/bin/env python3
#
# Copyright (c) 2026, Realtek Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0
#
"""Apply the prune list to the generated vcdj_ctrlsw.c / vcdj_ctrlsw.h.

    python vcdj_ctrlsw_prune.py              generate, prune, write both files
    python vcdj_ctrlsw_prune.py --check      exit 1 if the files on disk differ
    python vcdj_ctrlsw_prune.py --map FILE   also write an output line -> vendor
                                             file:line map of the pruned files

This runs after vcdj_ctrlsw_gen.py: the generator reproduces the vendor build
and stays checkable, and everything deleted from it lives in
vcdj_ctrlsw_prune.json as data. Rules locate code by name and by brace
structure, never by line number, so adding a source to a segment or regrouping
the output does not invalidate them.
"""
import argparse
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_H = os.path.join(HERE, 'vcdj_ctrlsw.h')
OUT_C = os.path.join(HERE, 'vcdj_ctrlsw.c')
RULES = os.path.join(HERE, 'vcdj_ctrlsw_prune.json')

sys.path.insert(0, HERE)
import vcdj_ctrlsw_gen as gen  # noqa: E402

GUARD_OPEN = re.compile(r'\s*#\s*(?:if|ifdef|ifndef)\b')
GUARD_ANY = re.compile(r'\s*#\s*(if|ifdef|ifndef|endif)\b')


class PruneError(Exception):
    pass


def read(path):
    with open(path, 'rb') as f:
        return f.read().decode('latin-1')


def code_lines(lines):
    """The lines with comments and literals blanked, so brace counting is safe."""
    text = '\n'.join(lines)
    blank = [(kind, a, b) for kind, a, b in gen.spans(text) if kind != 'code']
    out, pos = [], 0
    for kind, a, b in blank:
        out.append(text[pos:a])
        out.append(re.sub(r'[^\n]', ' ', text[a:b]))
        pos = b
    out.append(text[pos:])
    return ''.join(out).split('\n')


def find_definition(code, name, where):
    """Index of the line holding the definition's opening brace."""
    pat = re.compile(r'^[A-Za-z_][\w \t\*]*\b%s\s*\(' % re.escape(name))
    hits = [i for i, l in enumerate(code) if pat.match(l)]
    if len(hits) != 1:
        raise PruneError('%s: %s: expected 1 definition, found %d at lines %s'
                         % (where, name, len(hits), [h + 1 for h in hits]))
    start = hits[0]
    for i in range(start, len(code)):
        if '{' in code[i]:
            return start, i
        if ';' in code[i]:
            break
    raise PruneError('%s: %s: definition at line %d has no body'
                     % (where, name, start + 1))


def match_brace(code, open_line):
    """Line of the brace that closes the one on `open_line`."""
    depth = 0
    for i in range(open_line, len(code)):
        depth += code[i].count('{') - code[i].count('}')
        if depth < 0:
            raise PruneError('unbalanced } at line %d' % (i + 1))
        if depth == 0:
            return i
    raise PruneError('unterminated body from line %d' % (open_line + 1))


def guard_span(lines, lo, hi):
    """Extend [lo, hi] over a #if / #ifdef / #ifndef that wraps it exactly."""
    if lo == 0 or not GUARD_OPEN.match(lines[lo - 1]):
        return lo, hi
    start = lo - 1
    depth = 0
    for i in range(start, len(lines)):
        m = GUARD_ANY.match(lines[i])
        if not m:
            continue
        if m.group(1) == 'endif':
            depth -= 1
            if depth == 0:
                if i < hi:
                    raise PruneError('guard starting at line %d closes before the '
                                     'function body ends' % (start + 1))
                return start, max(hi, i)
        else:
            depth += 1
    raise PruneError('guard starting at line %d has no #endif' % (start + 1))


def delete_function(rule, doc, which):
    name = rule['name']
    lines = doc['lines']
    code = code_lines(lines)
    start, open_line = find_definition(code, name, which)
    hi = match_brace(code, open_line)
    lo, hi = guard_span(lines, start, hi)
    if hi + 1 < len(lines) and not lines[hi + 1].strip():
        hi += 1          # the blank line after the body goes too
    del doc['lines'][lo:hi + 1]


def delete_declaration(rule, doc):
    name = rule['name']
    lines = doc['lines']
    code = code_lines(lines)
    pat = re.compile(r'\b%s\s*\(' % re.escape(name))
    hits = [i for i, l in enumerate(code) if pat.search(l)]
    if not hits:
        if rule.get('header', True):
            raise PruneError('vcdj_ctrlsw.h: %s: declaration not found' % name)
        return
    if len(hits) != 1:
        raise PruneError('vcdj_ctrlsw.h: %s: expected 1 declaration, found %d at '
                         'lines %s' % (name, len(hits), [h + 1 for h in hits]))
    lo = hits[0]
    hi = lo
    while hi < len(lines) and not code[hi].rstrip().endswith(';'):
        hi += 1
    if hi == len(lines):
        raise PruneError('vcdj_ctrlsw.h: %s: declaration at line %d is not terminated'
                         % (name, lo + 1))
    # A #ifdef / #endif pair that wraps only this declaration goes with it.
    if (lo > 0 and GUARD_OPEN.match(lines[lo - 1]) and hi + 1 < len(lines)
            and re.match(r'\s*#\s*endif\b', lines[hi + 1])):
        lo, hi = lo - 1, hi + 1
    del lines[lo:hi + 1]


def dead_regions(code):
    """Line indices inside an `#if 0` block that has no #else / #elif.

    A call there is not compiled, so deleting its target leaves nothing
    dangling. Any #else or #elif makes the block live again and it is not
    marked, which keeps this check conservative.
    """
    dead, stack = set(), []
    for i, l in enumerate(code):
        m = GUARD_ANY.match(l)
        if not m:
            continue
        kw = m.group(1)
        if kw in ('if', 'ifdef', 'ifndef'):
            stack.append([i, False, kw == 'if' and l.split(None, 1)[1].strip() == '0'])
        elif kw == 'endif' and stack:
            start, live, was_zero = stack.pop()
            if was_zero and not live:
                dead.update(range(start, i + 1))
        elif stack:
            stack[-1][1] = True
    return dead


def check_no_code_references(rule, doc, which):
    """The deleted name must not survive in live code; comments may mention it."""
    name = rule['name']
    pat = re.compile(r'\b%s\b' % re.escape(name))
    code = code_lines(doc['lines'])
    dead = dead_regions(code)
    left = [i + 1 for i, l in enumerate(code) if i not in dead and pat.search(l)]
    if left:
        raise PruneError('%s: %s still referenced in live code at lines %s; the rule '
                         'would leave a dangling use' % (which, name, left[:5]))


ACTIONS = {
    'delete_function': (delete_function, check_no_code_references),
}


def apply_rules(h, c, rules):
    docs = {'vcdj_ctrlsw.h': {'lines': [l for l, _ in h], 'origin': [o for _, o in h]},
            'vcdj_ctrlsw.c': {'lines': [l for l, _ in c], 'origin': [o for _, o in c]}}
    for n, rule in enumerate(rules, 1):
        action = rule.get('action')
        if action not in ACTIONS:
            raise PruneError('rule %d: unknown action %r' % (n, action))
        fn, verify = ACTIONS[action]
        fn(rule, docs['vcdj_ctrlsw.c'], 'vcdj_ctrlsw.c')
        delete_declaration(rule, docs['vcdj_ctrlsw.h'])
        for name, doc in docs.items():
            verify(rule, doc, name)
    return (list(zip(docs['vcdj_ctrlsw.h']['lines'], docs['vcdj_ctrlsw.h']['origin'])),
            list(zip(docs['vcdj_ctrlsw.c']['lines'], docs['vcdj_ctrlsw.c']['origin'])))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--check', action='store_true', help='fail if the files on disk differ')
    ap.add_argument('--map', metavar='FILE', help='write the pruned output line -> vendor line map')
    a = ap.parse_args()
    try:
        h, c, _ = gen.generate()
        rules = json.loads(read(RULES))['rules']
        h, c = apply_rules(h, c, rules)
    except gen.GenError as e:
        sys.exit('vcdj_ctrlsw_prune: generator error: %s' % e)
    except (PruneError, ValueError, KeyError) as e:
        sys.exit('vcdj_ctrlsw_prune: error: %s' % e)

    files = ((OUT_H, h), (OUT_C, c))
    if a.map:
        with open(a.map, 'w') as f:
            for p, lines in files:
                for n, (_, o) in enumerate(lines, 1):
                    if o:
                        f.write('%s\t%d\t%s\t%d\n' % (os.path.basename(p), n,
                                                      gen.rel(o[0]), o[1]))
    if a.check:
        stale = [p for p, t in files
                 if not os.path.isfile(p) or read(p) != ''.join(l + '\n' for l, _ in t)]
        for p in stale:
            print('vcdj_ctrlsw_prune: %s is stale; rerun the generator and this script'
                  % os.path.basename(p))
        sys.exit(1 if stale else 0)
    for p, t in files:
        with open(p, 'wb') as f:
            f.write(''.join(l + '\n' for l, _ in t).encode('latin-1'))
        print('wrote %s (%d lines)' % (os.path.basename(p), len(t)))


if __name__ == '__main__':
    main()

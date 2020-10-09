#!/usr/bin/env  python3

#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

import sys
import json
import os
import subprocess
import argparse
import re
import time
from pathlib import Path
import concurrent.futures

def get_sdkroot():
    """Get sdk root folder by asking git"""
    return subprocess.check_output(['git', 'rev-parse', '--show-toplevel'], shell=False, cwd=os.path.dirname(os.path.realpath(__file__))).decode('utf-8').strip()


def run_clang_tidy(build_dir, entry):
    """Run clang-tidy as subprocess and print its output on error"""
    cmd = ['clang-tidy', '-extra-arg=-fcolor-diagnostics', '-p={}'.format(build_dir), '-quiet', entry['file']]
    p = subprocess.Popen(cmd, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = [o.decode('utf-8') for o in p.communicate()]
    if p.returncode != 0:
        print("\n{}\nError while processing {}\n{}{}".format(' '.join(cmd), entry['file'], out, err))
        return 1
    return 0

def process_entry(build_dir, entry):
    start_time = time.time()
    num_errors = run_clang_tidy(build_dir, entry)
    return {**entry, **{'dt': time.time() - start_time, 'errors': num_errors}}


def load_comdb(path):
    with open(path, 'r') as f:
        entries = json.load(f)
        print(f'Loaded {len(entries)} entries from {path}')
        return entries

def add_relpath(entries, base_path):
     return [{**e, **{'rel_file': str(Path(e['file']).relative_to(base_path))}} for e in entries]

def filter_entries(entries, excludes, includes):
    # remove duplicates
    entries = {e['file'] : e for e in entries}.values()

    # user filter
    exclude_re = re.compile('|'.join([f'(?:{e})' for e in excludes]))
    include_re = re.compile('|'.join([f'(?:{i})' for i in includes]))
    result = []
    for e in entries:
        if include_re.search(e['rel_file']) and not exclude_re.search(e['rel_file']):
            result.append(e)

    return result

def sort_entries(entries, sort_patterns):
    re_patterns = [(re.compile(e[0]), e[1]) for e in sort_patterns]
    def key_fun(e):
        prio = 0
        for p in re_patterns:
            reg, reg_prio = p
            if reg.search(e['rel_file']):
                prio = reg_prio
                break
        return (-prio,
                -Path(e['file']).stat().st_size,
                e['rel_file'])
    return sorted(entries, key=key_fun)


def format_dt(dt):
    seconds = int(dt) % 60
    minutes = int(dt) // 60
    millis  = int(dt * 1000.) % 1000
    if minutes != 0:
        return f'{minutes}m{seconds:02}.{millis:03}'
    return f'{seconds}.{millis:03}'

def print_slowest_entries(entries, timing_selection):
    entries = sorted(entries, key=lambda e: e['dt'], reverse=True)
    print(f'Timing statistics for {timing_selection} slowest entries')
    num_entries = len(entries) if timing_selection == 'all' else int(timing_selection)
    for e in entries[:num_entries]:
        print(f'{format_dt(e["dt"]):>10}  {e["rel_file"]}')

def print_overall_runtime(entries, num_threads, start_time):
    wallclock_runtime = time.time() - start_time
    total_cpu_runtime = sum([e['dt'] for e in entries])
    cpu_usage = ((total_cpu_runtime / num_threads) / wallclock_runtime) * 100
    print(f'Wallclock runtime {format_dt(wallclock_runtime)}, total thread runtime {format_dt(total_cpu_runtime)}, CPU usage {cpu_usage:.1f}%')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('compdb', help='Full path of compile_commands.json')
    parser.add_argument('-f', '--filter', default=None, help='filter processed files that match the provided regex')
    parser.add_argument('--timing', default=None, required=False, help='show timing for X longest entries (or "all" for all)')
    parser.add_argument('--threads', default=os.cpu_count(), type=int, required=False, help='Number of CPU cores to use (default: all)')
    args = parser.parse_args()

    start_time = time.time()
    sdkroot = Path(get_sdkroot())

    entries = load_comdb(args.compdb)
    entries = add_relpath(entries, sdkroot)
    entries = filter_entries(entries,
                             ['^external/'],
                             [args.filter] if args.filter else [])
    entries = sort_entries(entries, [('/test/', 10)])

    print('Check {} remaining entries with {} threads'.format(len(entries), args.threads))
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.threads) as executor:
        build_dir = os.path.dirname(args.compdb)
        result_entries = list(executor.map(lambda e: process_entry(build_dir, e), entries))


    if args.timing:
        print_slowest_entries(result_entries, args.timing)
        print()

    print_overall_runtime(result_entries, args.threads, start_time)

    errors = sum([e['errors'] for e in result_entries])
    if errors == 0:
        print("Done without error")
        return 0
    else:
        print('Done with errors in {}/{} files'.format(errors, len(entries)))
        return 1


sys.exit(main())

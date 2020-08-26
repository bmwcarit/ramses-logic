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
import queue
import threading
import subprocess
import argparse
import re
import time
from pathlib import Path


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


def worker(workq, resultq, build_dir):
    """Worker thread function: Process items from workq and put final number of errors in resultq"""
    errors = 0
    while True:
        e = workq.get()
        if e is None:
            return
        start_time = time.time()
        num_errors = run_clang_tidy(build_dir, e)
        resultq.put((e['file'], time.time() - start_time, num_errors))


def format_dt(dt):
    seconds = int(dt) % 60
    minutes = int(dt) // 60
    millis  = int(dt * 1000.) % 1000
    if minutes != 0:
        return f'{minutes}m{seconds:02}.{millis:03}'
    return f'{seconds}.{millis:03}'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('compdb', help='Full path of compile_commands.json')
    parser.add_argument('-f', '--filter', default=None, help='filename filter')
    parser.add_argument('--timing', default=None, required=False, help='show timing for X entries (or "all" for all)')
    parser.add_argument('--threads', default=os.cpu_count(), type=int, required=False, help='Number of CPU cores to use (default: all)')
    args = parser.parse_args()

    start_time = time.time()

    # Read in compile_commands.json
    with open(args.compdb, 'r') as f:
        entries = json.load(f)
    print('Loaded {} entries from {}'.format(len(entries), args.compdb))

    # filter out 3rd party files
    sdkroot = Path(get_sdkroot())
    rel_includes = []
    rel_excludes = ['external/']

    filtered_entries = []
    for e in entries:
        if (any(e['file'].startswith(str(sdkroot / p)) for p in rel_includes) or not any(e['file'].startswith(str(sdkroot / p)) for p in rel_excludes)) and not e['file'].endswith('.pb.cc'):
            filtered_entries.append(e)

    if args.filter:
        filtered_entries = [e for e in filtered_entries if re.search(args.filter.lower(), e['file'].lower())]

    # remove duplicate filenames (when same file built for multiple targets)
    filtered_entries = {e['file'] : e for e in filtered_entries}.values()

    # Sort list for predictable order with everything in test folders first because on average take longer to process.
    # Use file size as 2nd sort criteria and name as 3rd to disambiguate if everything else is the same.
    filtered_entries = sorted(filtered_entries, key=lambda e: ('/test/' not in e['file'], -Path(e['file']).stat().st_size, e['file']))

    # put all remaining files into thread-safe queue
    print('Check {} remaining entries with {} threads'.format(len(filtered_entries), args.threads))

    workq = queue.Queue()
    for e in filtered_entries:
        workq.put(e)
    resultq = queue.Queue()

    # spawn worker threads to run clang-tidy in parallel
    thread_args = {'workq': workq, 'resultq': resultq, 'build_dir': os.path.dirname(args.compdb)}
    threads = [threading.Thread(target=worker, kwargs=thread_args) for _ in range(args.threads)]
    for t in threads:
        t.start()
        workq.put(None)

    # collect per file results from threads
    thread_results = [resultq.get() for _ in filtered_entries]

    # join threads
    [t.join() for t in threads]

    # collect results
    errors = sum([num_errors for _, _, num_errors in thread_results])
    total_runtime = sum([dt for _, dt, _ in thread_results])
    wallclock_runtime = time.time() - start_time

    if args.timing:
        file_runtime = sorted(thread_results, key=lambda e: e[1], reverse=True)
        print(f'Timing statistics for {args.timing} slowest entries')
        num_entries = len(file_runtime) if args.timing == 'all' else int(args.timing)
        for fname, dt, _ in file_runtime[:num_entries]:
            print(f'{format_dt(dt):>10}  {Path(fname).relative_to(sdkroot)}')
        print()

    cpu_usage = ((total_runtime / args.threads) / wallclock_runtime) * 100
    print(f'Wallclock runtime {format_dt(wallclock_runtime)}, total thread runtime {format_dt(total_runtime)}, CPU usage {cpu_usage:.1f}%')

    if errors == 0:
        print("Done without error")
        return 0
    else:
        print('Done with errors in {}/{} files'.format(errors, len(filtered_entries)))
        return 1


sys.exit(main())

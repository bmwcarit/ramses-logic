#!/usr/bin/env python3

#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

import os
import sys
import argparse
from pathlib import Path
import re
import subprocess
import json

def get_sdkroot():
    """Get sdk root folder by asking git"""
    return subprocess.check_output(['git', 'rev-parse', '--show-toplevel'], shell=False, cwd=os.path.dirname(os.path.realpath(__file__))).decode('utf-8').strip()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--prof-dir', required=True, help='Directory with profraw files')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--report-dir', help='Output directory for report')
    group.add_argument('--export-file', help='Output file for export data')
    parser.add_argument('--export-summary', required=False, default=None, help='Export coverage summary as json')
    args = parser.parse_args()

    profdir = Path(args.prof_dir)
    sdkroot = get_sdkroot()

    if not profdir.is_dir():
        raise Exception(f'profdir does not exist {profdir}')

    prof_files = [e.name for e in profdir.glob(f'*.profraw')]
    if not prof_files:
        raise Exception('No profraw files match filter')

    executables = []
    for e in prof_files:
        m = re.match(r'^(.*?)_\d+\.profraw$', e)
        if m:
            executables.append(m.group(1))
    for e in executables:
        if not (profdir / e).exists():
            raise Exception(f'Could not find exectuable {e}')

    merged_data = 'coverage.profdata'
    print(f'Generate {merged_data}')
    subprocess.check_call(['llvm-profdata', 'merge', '-o', merged_data] + prof_files, shell=False, cwd=profdir)

    merged_executable = f'merged-executable'
    print(f'Generate {merged_executable}')
    subprocess.check_call(['ld', '-r', '-z', 'muldefs', '-o', merged_executable] + executables, shell=False, cwd=profdir)

    exclude_regex = f'{sdkroot}/(external|src/flatbuffers/generated)/'

    if args.report_dir:
        reportdir = Path(args.report_dir).resolve()
        print(f'Generate report in {reportdir}')
        subprocess.check_call(['llvm-cov', 'show',
                               merged_executable,
                               '-show-line-counts-or-regions',
                               f'-instr-profile={merged_data}',
                               '-format=html',
                               f'-output-dir={reportdir}',
                               f"-ignore-filename-regex={exclude_regex}",
                               '-Xdemangler=c++filt',
                               sdkroot],
                              shell=False, cwd=profdir)
    elif args.export_file:
        export_file = Path(args.export_file)
        print(f'Generate json export at {export_file}')
        export_file.write_bytes(
            subprocess.check_output(['llvm-cov', 'export',
                                     '-format=text',
                                     '-skip-expansions',
                                     f"-ignore-filename-regex={exclude_regex}",
                                     f'-instr-profile={merged_data}',
                                     merged_executable,
                                     sdkroot],
                                    shell=False, cwd=profdir))

    if args.export_summary:
        summary_file = Path(args.export_summary)
        print(f'Generate summary json at {summary_file}')
        summary = json.loads(
            subprocess.check_output(['llvm-cov', 'export',
                                     '-format=text',
                                     '-summary-only',
                                     f"-ignore-filename-regex={exclude_regex}",
                                     f'-instr-profile={merged_data}',
                                     merged_executable,
                                     sdkroot],
                                    shell=False, cwd=profdir))
        summary_file.write_text(json.dumps(summary['data'][0]['totals']))

    print('Done')

sys.exit(main())

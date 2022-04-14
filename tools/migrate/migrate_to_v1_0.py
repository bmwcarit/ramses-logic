#!/usr/bin/env python3

#  -------------------------------------------------------------------------
#  Copyright (C) 2022 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

import click

import os
from pathlib import Path

class Common:
    def __init__(self):
        pass


@click.group()
@click.pass_context
def cli(ctx):
    ctx.obj = Common()

@cli.command()
@click.pass_obj
@click.argument('path')
def types(conf, path):
    patterns = [
        ['INT32', 'Type:Int32()'],
        ['INT64', 'Type:Int64()'],
        ['INT'  , 'Type:Int32()'],
        ['FLOAT', 'Type:Float()'],
        ['STRING','Type:String()'],
        ['BOOL',  'Type:Bool()'],
        ['VEC2F', 'Type:Vec2f()'],
        ['VEC3F', 'Type:Vec3f()'],
        ['VEC4F', 'Type:Vec4f()'],
        ['VEC2I', 'Type:Vec2i()'],
        ['VEC3I', 'Type:Vec3i()'],
        ['VEC4I', 'Type:Vec4i()'],
        ['ARRAY', 'Type:Array'],
    ]
    replace_patterns_in_files(conf, path, patterns)

@cli.command()
@click.pass_obj
@click.argument('path')
def inout(conf, path):
    patterns = [
        ['function interface()', 'function interface(IN,OUT)'],
        ['function run()', 'function run(IN,OUT)'],
    ]
    replace_patterns_in_files(conf, path, patterns)

def replace_patterns_in_files(conf, path, patterns):
    rootpath = Path(path)
    if not rootpath.exists():
        raise Exception(f"No folder '{rootpath}' found!")

    for root, _dirs, files in os.walk(rootpath):
        for filename in files:
            file = Path(root) / filename

            isExternal = ((rootpath / "external") in file.parents)
            if isExternal:
                print(f"Skipping file {file} in {root}...")
                continue

            if filename.endswith(".lua") or filename.endswith(".cpp") or filename.endswith(".h"):
                print(f"Processing file {file}...")

                with open(file, 'r') as f :
                    filedata = f.read()

                # Replace the target string
                for p in patterns:
                    filedata = filedata.replace(p[0], p[1])

                # Write the file out again
                with open(file, 'w') as f:
                    f.write(filedata)


if __name__ == '__main__':

    cli()

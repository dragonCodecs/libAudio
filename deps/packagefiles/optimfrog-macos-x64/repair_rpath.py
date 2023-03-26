#!/usr/bin/env python3
# This file is part of libAudio
#
# SPDX-FileCopyrightText: 2022-2023 amyspark <amy@amyspark.me>
# SPDX-License-Identifier: BSD-3-Clause
#

import argparse
import errno
import pathlib
import os
import shutil
import subprocess

if __name__ == '__main__':
    argParser = argparse.ArgumentParser(
        description = 'Repair the rpath of a dylib')

    argParser.add_argument('libname', metavar = 'FILE', type = pathlib.Path,
                            help = 'Library to parse')
    argParser.add_argument('destination', metavar = 'FILE',
                            type = pathlib.Path,
                            help = 'Copy libname to destination')

    args = argParser.parse_args()

    libname: pathlib.Path = args.libname
    destination: pathlib.Path = args.destination.resolve(strict=False)

    if not libname.exists():
        raise FileNotFoundError(
            errno.ENOENT, os.strerror(errno.ENOENT), libname)
    
    shutil.copy(libname.resolve(strict=True), destination)

    subprocess.run(['/usr/bin/install_name_tool', '-id', f"@rpath/{destination.name}", destination], check=True)

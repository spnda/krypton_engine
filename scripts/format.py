#!/usr/bin/env python3

import argparse
import glob
import os
import shutil
import subprocess
import sys

# Just C/C++/Obj-C/Obj-C++ extensions
extensions = [".hpp", ".cpp", ".mm", ".m" ".c", ".h", ".cc", ".hh"]


def find_clang_format():
    # We first search if there's a clang-format with no version number
    if shutil.which("clang-format") is not None:
        return "clang-format"

    # Now we search for a versioned clang-format with the highest version number
    for version in range(14, 1, -1):  # We support up to clang-format-14 for now
        if shutil.which(f"clang-format-{version}") is not None:
            return f"clang-format-{version}"


clangfmt = find_clang_format()
cmakefmt = shutil.which("cmake-format")


def call_and_check(args):
    return_code = subprocess.call(args)
    if return_code != 0:
        sys.exit(return_code)


# Formats a single C++, CMake or Python file
def format_file(write, file):
    for ext in extensions:
        if file.endswith(ext):
            print(f"Formatting {file}")

            # If we don't want to write to files directory we set -Werror so that
            # clang-format will exit with an error code and we know something is ill-formed.
            write_files = ["-i"] if write else ["--dry-run", "-Werror"]
            call_and_check([clangfmt, *write_files, "--style=file", file])

    if cmakefmt is not None and file.endswith("CMakeLists.txt"):
        print(f"Formatting {file}")
        call_and_check([cmakefmt, "-o" if write else "--check", file, file])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--write",
        help="Write the formatted files to disk",
        default=False,
        action=argparse.BooleanOptionalAction,
    )
    parser.add_argument("--directory", help="The directory to format")
    parser.add_argument("filenames", nargs="*", help="A list of files to format")
    args = parser.parse_args()

    if clangfmt is None:
        print("Could not find clang-format")
        return

    if not os.path.isfile(".clang-format"):
        print("No .clang-format file found in the current directory")
        return

    # Run clang-format on all files
    if args.directory is not None:
        for file in glob.iglob(f"{args.directory}/**/*", recursive=True):
            format_file(args.write, file)

    for file in args.filenames:
        format_file(args.write, file)


if __name__ == "__main__":
    main()

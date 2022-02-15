#!/usr/bin/python3

import glob
import os
import shutil
import subprocess
import sys

# Just C/C++/Obj-C/Obj-C++ extensions
extensions = [".hpp", ".cpp", ".mm", ".m" ".c", ".h", ".cc", ".hh"]

def findClangFormat():
    # We first search if there's a clang-format with no version number
    if shutil.which("clang-format") is not None:
        return "clang-format"

    # Now we search for a versioned clang-format with the highest version number
    for version in range(13, 1, -1): # We support up to clang-format-13 for now
        if shutil.which(f"clang-format-{version}") is not None:
            return f"clang-format-{version}"

def findCmakeFormat():
    return shutil.which("cmake-format")

def main():
    # Check if the first argument passed is a valid directory
    if len(sys.argv) < 2 or not os.path.isdir(sys.argv[1]):
        print("Missing or invalid directory argument")
        return

    clangfmt = findClangFormat()
    cmakefmt = findCmakeFormat()
    if clangfmt is None:
        print("Could not find clang-format")
        return
    
    if not os.path.isfile(".clang-format"):
        print("No .clang-format file found in the current directory")
        return

    # Run clang-format on all files
    for file in glob.iglob(f"{sys.argv[1]}/**/*", recursive=True):
        for ext in extensions:
            if file.endswith(ext):
                print(f"Formatting {file}")
                subprocess.call([clangfmt, "-i", "--style=file", file])
        
        if cmakefmt is not None and file.endswith("CMakeLists.txt"):
                print(f"Formatting {file}")
                subprocess.call([cmakefmt, "-o", file, file])

    print("Finished formatting all files")

if __name__ == "__main__":
    main()

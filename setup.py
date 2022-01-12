import os
from pathlib import Path
import platform
import subprocess
import sys
import urllib.request
import zipfile

class colors:
    green = "\033[92m"
    yellow = "\033[93m"
    red = "\033[91m"
    end = "\033[0m"

def call(args, cwd=".", shell=False):
    try:
        # It's probably not the best idea to also surpress stderr, but it
        # keeps our logs clean and this doesn't do much anyway
        subprocess.check_call(args, cwd=cwd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, shell=shell)
        return True   
    except subprocess.CalledProcessError:
        return False

def main():
    print(f"{colors.green}Setting up project...{colors.end}")
    if sys.version_info < (3, 10):
        print(f"{colors.red}Python 3.10 or later is required to run this script.{colors.end}")
        return

    if not os.path.isdir("build"):
        Path("build/debug").mkdir(parents=True, exist_ok=True)
        Path("build/release").mkdir(parents=True, exist_ok=True)

    # Clone submodules if the user hasn't already
    print(f"{colors.green}Cloning submodules...{colors.end}")
    call(["git", "submodule", "update", "--init", "--recursive"], ".")

    print(f"{colors.green}Downloading slang...{colors.end}")
    if not os.path.isdir("external/slang"):
        Path("external/slang").mkdir(parents=True, exist_ok=True)

    # Determine slang build URL
    slang_zip_url = ""
    match platform.system():
        case "Darwin": # MacOS
            print(f"{colors.orange}slang does not provide MacOS builds.")
        case "Windows": # Windows
            slang_zip_url = "https://github.com/shader-slang/slang/releases/download/v0.19.25/slang-0.19.24-win64.zip"
        case "Linux": # Linux
            slang_zip_url = "https://github.com/shader-slang/slang/releases/download/v0.19.25/slang-0.19.24-linux-x86_64.zip"

    # Download slang build from GitHub
    if len(slang_zip_url) > 0 and not os.path.exists("external/slang/slang.zip"):
        urllib.request.urlretrieve(slang_zip_url, "external/slang/slang.zip")

        # Extract slang build zip file
        with zipfile.ZipFile("external/slang/slang.zip", "r") as zip_ref:
            zip_ref.extractall("external/slang")

    # The VCPKG_ROOT environment variable should point to the vpckg installation.
    # Without this, our CMake script might not be able to identify where to find dependencies.
    if "VCPKG_ROOT" not in os.environ:
        print(f"{colors.yellow}VCPKG_ROOT is not defined as an environment variable. This might cause issues with dependencies.{colors.end}")

    match platform.system():
        case "Darwin": # MacOS
            # We will try running vcpkg install, even on Mac, but won't print
            # any messages/errors to the console as the user might've installed
            # the packages through e.g. brew
            call(["vcpkg", "install"])

            # glslang uses behaviour which was deprecated with CMake 3.13,
            # therefore we'll disable that warning using -Wno-dev
            print(f"{colors.green}Configuring build files...{colors.end}")
            call(["cmake", "-G", "Xcode", "-Wno-dev", "../.."], "build/debug")
            call(["cmake", "-G", "Xcode", "-Wno-dev", "../.."], "build/release")
        case _: # Windows / Linux
            print(f"{colors.green}Installing dependencies through vcpkg...{colors.end}")
            if not call(["vcpkg", "install"], "."):
                print(f"{colors.red}The command 'vcpkg install' failed. Perhaps vcpkg is not installed?{colors.end}")

            print(f"{colors.green}Configuring build files...{colors.end}")
            call(["cmake", "-Wno-dev", "../.."], "build/debug", True)
            call(["cmake", "-Wno-dev", "../.."], "build/release", True)

    print(f"{colors.green}Finished configuring project files in /build/debug/ and /build/release.{colors.end}")

if __name__ == "__main__":
    main()

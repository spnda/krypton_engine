#!/usr/bin/env python3

import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import urllib.request
import zipfile


class Colors:
    green = "\033[92m"
    yellow = "\033[93m"
    red = "\033[91m"
    end = "\033[0m"


def call(args, cwd=".", shell=False):
    try:
        # It's probably not the best idea to also supress stderr, but it
        # keeps our logs clean and this doesn't do much anyway
        subprocess.check_call(
            args,
            cwd=cwd,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            shell=shell,
        )
        return True
    except subprocess.CalledProcessError:
        return False


def main():
    print(f"{Colors.green}Setting up project...{Colors.end}")
    if sys.version_info < (3, 10):
        print(
            f"{Colors.red}Python 3.10 or later is required to run this script.{Colors.end}"
        )
        return

    if not os.path.isdir("build"):
        Path("build/debug").mkdir(parents=True, exist_ok=True)
        Path("build/release").mkdir(parents=True, exist_ok=True)

    # Run pre-commit install to install all necessary git hooks
    if shutil.which("pre-commit"):
        call(["pre-commit", "install"])
    else:
        print(
            f"{Colors.yellow}pre-commit not found. Git hooks will not be installed.{Colors.end}"
        )

    # Clone submodules if the user hasn't already
    print(f"{Colors.green}Cloning submodules...{Colors.end}")
    call(["git", "submodule", "update", "--init", "--recursive"], ".")

    print(f"{Colors.green}Downloading slang...{Colors.end}")
    if not os.path.isdir("external/slang"):
        Path("external/slang").mkdir(parents=True, exist_ok=True)

    # Determine slang build URL. We update the version here
    slang_version = "0.23.2"
    slang_zip_url = ""
    match platform.system():
        case "Darwin":  # MacOS
            print(f"{Colors.yellow}slang does not provide MacOS builds.")
        case "Windows":  # Windows
            slang_zip_url = f"https://github.com/shader-slang/slang/releases/download/v{slang_version}/slang-{slang_version}-win64.zip"
        case "Linux":  # Linux
            slang_zip_url = f"https://github.com/shader-slang/slang/releases/download/v{slang_version}/slang-{slang_version}-linux-x86_64.zip"

    # Download slang build from GitHub
    if len(slang_zip_url) > 0:
        urllib.request.urlretrieve(slang_zip_url, "external/slang/slang.zip")

        # Extract slang build zip file
        with zipfile.ZipFile("external/slang/slang.zip", "r") as zip_ref:
            zip_ref.extractall("external/slang")

    # The VCPKG_ROOT environment variable should point to the vpckg installation.
    # Without this, our CMake script might not be able to identify where to find dependencies.
    if "VCPKG_ROOT" not in os.environ:
        print(
            f"{Colors.yellow}VCPKG_ROOT is not defined as an environment variable. This may lead to missing dependencies.{Colors.end}"
        )

    match platform.system():
        case "Darwin":  # MacOS
            # We will try running vcpkg install, even on Mac, but won't print
            # any messages/errors to the console as the user might've installed
            # the packages through e.g. brew
            call(["vcpkg", "install"])

            if shutil.which("cmake") is None:
                print(
                    f"{Colors.red}cmake is not installed. The project cannot be built.{Colors.end}"
                )
            else:
                # glslang uses behaviour which was deprecated with CMake 3.13,
                # therefore we'll disable that warning using -Wno-dev
                print(f"{Colors.green}Configuring build files...{Colors.end}")
                call(["cmake", "-G", "Xcode", "-Wno-dev", "../.."], "build/debug")
                call(["cmake", "-G", "Xcode", "-Wno-dev", "../.."], "build/release")
        case _:  # Windows / Linux
            print(f"{Colors.green}Installing dependencies through vcpkg...{Colors.end}")
            if shutil.which("vcpkg") is not None:
                if not call(["vcpkg", "install"], "."):
                    print(
                        f"{Colors.red}The command 'vcpkg install' failed.{Colors.end}"
                    )
            else:
                print(
                    f"{Colors.yellow}vcpkg is not installed. This may lead to missing dependencies.{Colors.end}"
                )

            if shutil.which("cmake") is None:
                print(
                    f"{Colors.red}cmake is not installed. The project cannot be built.{Colors.end}"
                )
            else:
                print(f"{Colors.green}Configuring build files...{Colors.end}")
                call(
                    ["cmake", "-Wno-dev", "../.."],
                    "build/debug",
                    platform.system() == "Windows",
                )
                call(
                    ["cmake", "-Wno-dev", "../.."],
                    "build/release",
                    platform.system() == "Windows",
                )

    print(
        f"{Colors.green}Finished configuring project files in /build/debug/ and /build/release.{Colors.end}"
    )


if __name__ == "__main__":
    main()

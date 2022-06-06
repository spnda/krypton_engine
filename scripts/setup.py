#!/usr/bin/env python3

import os
import platform
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path
import urllib.request
import urllib.error
from typing import Callable


class Colors:
    green = "\033[92m"
    yellow = "\033[93m"
    red = "\033[91m"
    end = "\033[0m"
    bold = "\033[1m"


def call(args, cwd=".", shell=False) -> bool:
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


def create_dir(path):
    if not os.path.isdir(path):
        Path(path).mkdir(parents=True, exist_ok=True)


# Downloads a zip file from given url and then extracts that zip file
def download_and_extract(url: str, path: str):
    if len(url) == 0:
        return

    try:
        f_path, _ = urllib.request.urlretrieve(url, f"{path}/temp.zip")
        with zipfile.ZipFile(f_path, "r") as zip_ref:
            zip_ref.extractall(path)
        os.remove(f_path)
    except urllib.error.HTTPError:
        print(f"{Colors.red}Could not download {url}.{Colors.end}")


def download_external(name: str, url_callback: Callable[[], str]):
    print(f"{Colors.green}Downloading {name}...{Colors.end}")
    dir_path = f"external/{name}"
    create_dir(dir_path)
    url = url_callback()
    if len(url) > 0:
        download_and_extract(url, dir_path)
    elif os.path.isdir(dir_path):
        shutil.rmtree(dir_path)


def find_vcpkg_toolchain_file() -> str | None:
    vcpkg_dir = shutil.which("vcpkg")
    if vcpkg_dir is None or vcpkg_dir == "":
        return None
    else:
        return vcpkg_dir[:-5] + "scripts/buildsystems/vcpkg.cmake"


def configure_cmake(generator: str | None = None):
    if shutil.which("cmake") is None:
        print(
            f"{Colors.red}cmake is not installed. The project cannot be built.{Colors.end}"
        )
        return

    create_dir("build/debug")
    create_dir("build/release")

    gen = ["-G", generator] if generator is not None else []
    toolchain_file = find_vcpkg_toolchain_file()
    tcf_param = [f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}"] if toolchain_file is not None else []
    call(
        ["cmake", *gen, *tcf_param, "../.."],
        "build/debug",
        platform.system() == "Windows",
    )
    call(
        ["cmake", *gen, *tcf_param, "../.."],
        "build/release",
        platform.system() == "Windows",
    )


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
    call(
        ["git", "submodule", "update", "--init", "--recursive"],
        ".",
    )

    # Download slang
    slang_version = "0.23.4"
    slang_zip_url = (
        f"https://github.com/shader-slang/slang/releases/download/v{slang_version}/"
    )
    match platform.system():
        case "Darwin":  # MacOS
            print(f"{Colors.yellow}slang does not provide MacOS builds.{Colors.end}")
            slang_zip_url = ""  # so that it doesn't try to download anything
        case "Windows":  # Windows
            slang_zip_url += f"slang-{slang_version}-win64.zip"
        case "Linux":  # Linux
            slang_zip_url += f"slang-{slang_version}-linux-x86_64.zip"

    download_external("slang", lambda: slang_zip_url)

    # Download glslang. glslang builds for MacOS and Linux are broken. On these platforms, we use
    # brew, apt, or any other package manager to obtain builds.
    glslang_url = "https://github.com/KhronosGroup/glslang/releases/download/master-tot/glslang-master-"
    match platform.system():
        case "Darwin":  # MacOS
            glslang_url = ""
        case "Windows":  # Windows
            glslang_url += "windows-x64"
        case "Linux":  # Linux
            glslang_url = ""

    if len(glslang_url) > 0:
        download_external("glslang-debug", lambda: glslang_url + "-Debug.zip")
        download_external("glslang-release", lambda: glslang_url + "-Release.zip")

    metalcpp_url = "https://developer.apple.com/metal/cpp/files/metal-cpp_macOS13_iOS16-beta.zip"
    if platform.system() == "Darwin":
        download_external("metal-cpp", lambda: metalcpp_url)

    # The VCPKG_ROOT environment variable should point to the vpckg installation.
    # Without this, our CMake script might not be able to identify where to find dependencies.
    if "VCPKG_ROOT" not in os.environ:
        print(
            f"{Colors.yellow}VCPKG_ROOT is not defined as an environment variable. This may lead to missing "
            f"dependencies.{Colors.end} "
        )

    match platform.system():
        case "Darwin":  # MacOS
            # We will try running vcpkg install, even on Mac, but won't print
            # any messages/errors to the console as the user might've installed
            # the packages through e.g. brew
            call(["vcpkg", "install"])

            # glslang uses behaviour which was deprecated with CMake 3.13,
            # therefore we'll disable that warning using -Wno-dev
            print(f"{Colors.green}Configuring build files...{Colors.end}")
            configure_cmake("Xcode")
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

            print(f"{Colors.green}Configuring build files...{Colors.end}")
            configure_cmake()

    print(
        f"{Colors.green}{Colors.bold}Finished configuring project files in /build/debug/ and /build/release.{Colors.end}"
    )


if __name__ == "__main__":
    main()

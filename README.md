# krypton

A modular rendering engine written in C++20 with a Vulkan 1.3 and Metal backend.

## Building

A fully conforming C++20 compiler is requried, e.g. GCC 10, Clang 13 or Visual Studio 16.

On Windows, one can simply install vcpkg, git and CMake to get this project running.

On Debian-based Linux distributions, however, it is recommended to additionally install Vulkan packages
from LunarG's [apt repository](https://vulkan.lunarg.com/doc/view/latest/linux/getting_started_ubuntu.html).
LunarG also provides a shaderc package.

After everything is installed, it is recommended to run `python setup.py` with Python 3.10 or newer. This installes
dependencies through vcpkg, if found, clones submodules if you have not done so already and configures the CMake project.

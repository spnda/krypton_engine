# krypton

A modular rendering engine written in C++20 with a Vulkan and Metal 3 backend. It includes a RHI
to quickly and easily interface with the GPU. We support Vulkan 1.1, 1.2, and 1.3 and only require
a minimal amount of extensions. The Metal backend only runs on devices running macOS 13 or iOS 16.

## Building

A fully conforming C++20 compiler is required, e.g. GCC 10, Clang 13 or Visual Studio 16. All
dependencies are installed via Git submodules and the setup script. Note that the python script
required at least Python 3.10.

```shell
git clone https://github.com/spnda/krypton_engine --recursive
cd krypton_engine
python3 scripts/setup.py
```

Then, building, is as simple as navigating to `build/debug` and running cmake. The python script
already configures CMake properly for you.

```shell
cd build/debug
cmake --build .
```

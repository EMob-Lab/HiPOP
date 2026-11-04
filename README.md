# HiPOP

High Performance Optimal Path module


## Description

HiPOP is the C++ graph library used by [MnMS](https://github.com/EMob-Lab/MnMS.git).
It is composed of:
- a C++ core library implementing a graph data-structure and several shortest path search algorithms,
- a Python wrapper exposing the C++ classes and functions to the Python world.


## Install HiPOP

### Install prebuilt distribution

This is the recommended (and most simple!) method to install HiPOP:
a prebuilt distribution adapted to your Python version and OS / system architecture
is downloaded from [PyPI](https://pypi.org/project/HiPOP/).

```shell
pip install hipop
```

This installation method requires your OS / system architecture to be one of these:
- Windows on x86_64 architecture (i.e. Intel/AMD CPU)
- MacOS (≥ 14 Sonoma) on ARM64 architecture (i.e. Apple M* CPU)
- Linux on x86_64 and ARM64 architectures

The exact list of prebuilt distributions is available on https://pypi.org/project/HiPOP/#files.
If none of them matches your environment, please contact the package maintainers,
or try the local build installation method.


### Install from local build

This method requires the following components to be available on your system:

- C++ compiler that supports C++17 and [OpenMP](https://www.openmp.org/)
- [CMake](https://cmake.org/) ≥ 3.19
- Python ≥ 3.10

Then, here is the local build install procedure:

1. Download the latest HiPOP sources from https://github.com/EMob-Lab/HiPOP/archive/refs/heads/main.zip
   or by cloning the git repository:
   ```shell
   git clone https://github.com/EMob-Lab/HiPOP.git
   ```

2. From the root directory of the HiPOP sources:
   ```shell
   pip install .

   # Optionally, to run the tests:
   pip install --group dev
   pytest
   ```

Remarks:

- The `--group <group-name>` option requires pip ≥ 25.1. If you have an older version,
  the install command above will fail with an error message such as `no such option: --group`.
  In this case, try to upgrade pip beforehand with:
  ```shell
  pip install --upgrade pip
  ```
  Alternatively, install the test runner `pytest` manually (or skip the optional test running step).

- Editable install mode can be used. The install command then becomes:
  ```shell
  pip install --group dev --editable .
  ```
  Please note that using the `--group dev` option (or equivalent, to install the dev dependencies)
  is mandatory for HiPOP editable installs.

- If you don't have the prerequisite C++/OpenMP/CMake components available on your system,
  but you have [Conda](https://docs.conda.io/) available, a Conda environment with these components
  is provided with the HiPOP sources. Install and activate it with:
  ```shell
  conda create -f conda/env.yaml
  conda activate hipop-dev
  ```
  Please note that the use of this Conda environment is optional. HiPOP can be used with regular
  (non-Conda) setups.

- If you are using MacOS and the Apple Clang C++ compiler, please note that OpenMP
  may not be available by default. You may install it with:
  ```shell
  brew install libomp
  OpenMP_ROOT=$(brew --prefix libomp)
  ```
  Alternatively, you may also use the Conda environment provided with the HiPOP sources
  (cf. previous remark).


### Install the C++ core library only (from local build)

This installation procedure allows you to build locally and install only the HiPOP C++ core library.
The Python wrapper is neither built nor installed here.
This procedure is intended for advanced users.

For this procedure, same requirements as mentioned in the previous section
in terms of C++/OpenMP/CMake components.
However, you don't need to have Python installed on your system.

Then, from the root directory of the HiPOP sources:
```shell
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=<install-directory>
make
make test    # Optionally, to run the tests
make install
```
... where `<install-directory>` is the path to the location where you want to install HiPOP.

# HiPOP - Developer setup

Quick reference for setting up a development environment and running common
development tasks. For end-user installation, see [README.md](README.md).


## Prerequisites

- C++ compiler that supports C++17 and [OpenMP](https://www.openmp.org/)
- [CMake](https://cmake.org/) ≥ 3.19
- Python ≥ 3.10

Remarks:

- The commands below use `pip` with the `--group <group-name>` option, which requires pip ≥ 25.1.
  Upgrade an older pip with:
  ```shell
  pip install --upgrade pip
  ```
  However, using pip is not mandatory: any package manager that supports
  [dependency-groups](https://packaging.python.org/en/latest/specifications/dependency-groups/)
  ([uv](https://docs.astral.sh/uv/), [PDM](https://pdm-project.org/), etc.) can be used instead.

- If these components are not available on your system but you have [Conda](https://docs.conda.io/),
  you may use the environment shipped with the sources:
  ```shell
  conda create -f conda/env.yaml
  conda activate hipop-dev
  ```

- On MacOS with Apple Clang, OpenMP is not bundled by default. Use:
  ```shell
  brew install libomp
  export OpenMP_ROOT=$(brew --prefix libomp)
  ```


## Set up the environment

```shell
git clone https://github.com/EMob-Lab/HiPOP.git
cd HiPOP
pip install --group dev --editable .
```

The build is driven by [scikit-build-core](https://scikit-build-core.readthedocs.io/)
(build tree in `build-scikit/`). The editable mode is configured so that the C++ extension
is recompiled on-the-fly when any C++ source file is modified.

By default, the C++ sources are built with `CMAKE_BUILD_TYPE=Release`
(i.e. highest C++ optimization level, no debug symbols).
To change that, set the [build type](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html)
before installing:
```shell
export CMAKE_BUILD_TYPE=Debug    # Debug | RelWithDebInfo | Release (default) | ...
```

Configuration of the build is in `pyproject.toml`, and in the CMake files for the C++ components.


## Common tasks

### Set up pre-commit hook

A homogeneous style across contributors and editors keeps diffs minimal and free of unrelated noise.
To that end, [pre-commit](https://pre-commit.com/) checks that files use consistent
line endings, have no trailing whitespace, and end with a single newline
(detailed configuration in `.pre-commit-config.yaml`).

Run this once in each cloned repository to install the check as a git hook:
```shell
pre-commit install
```
The hook then runs automatically on `git commit` against the staged files.
When a staged file breaks one of the rules, the hook rewrites it in place
and aborts the commit; review the fix, re-stage the file, and commit again.


### Check the Python type annotations

```shell
pyrefly check
```

Configuration of the Python type checker [pyrefly](https://pyrefly.org/) is in `pyrefly.toml`.

The build generates type stubs (`*.pyi` files) for the compiled C++ extension, so that
the type checker can analyze code using it; these stubs are also bundled into the wheel files.
They can be found in directory `build-scikit/python/stubs`.


### Run the Python test suite

```shell
pytest
```

Python tests are defined in `python/tests`.
Configuration of the Python test runner [pytest](https://docs.pytest.org/) is in `pytest.toml`.


### Build and run the C++ test suite

```shell
cmake -S . -B build
cd build
make
make test
```

Again, by default, everything is built with `CMAKE_BUILD_TYPE=Release`,
and can be overridden by setting the `CMAKE_BUILD_TYPE` environment variable
before invoking `cmake`.

C++ tests are defined in `cpp/tests`.


### Run static analysis of the C++ code

HiPOP uses [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) to catch bugs and performance
issues beyond what compiler warnings cover. To enable it, follow the "Build and run the C++ test suite"
workflow above, additionally passing `-DHIPOP_USE_CLANG_TIDY=ON` to `cmake`.
The static analysis checks will then be run as part of the build.

Configuration is in the `.clang-tidy` files (repository root, plus incremental overrides
in subdirectories where needed).

Remarks:

- On Linux with GCC, clang-tidy additionally needs the LLVM OpenMP development headers.
  On Ubuntu/Debian, can be installed with:
  ```shell
  sudo apt install libomp-dev
  ```

- On MacOS, clang-tidy is not installed by default. Use:
  ```shell
  brew install llvm
  export CLANG_TIDY_ROOT=$(brew --prefix llvm)
  ```


### Generate distribution files

```shell
pip install --group distrib
python -m build            # Generate a source distribution archive + a local (non-portable) wheel
python -m build --sdist    # ... or just the source distribution archive
python -m build --wheel    # ... or just the local (non-portable) wheel
```

The corresponding artifacts are generated in directory `dist`.

The wheel from `python -m build` is bound to the Python interpreter and OS/architecture
used to generate it. Generating portable wheels needs additional operations, such as setting the
[compatibility tags](https://packaging.python.org/en/latest/specifications/platform-compatibility-tags/),
bundling external shared libraries into the wheel
(via [`auditwheel`](https://github.com/pypa/auditwheel) on Linux,
or its [MacOS](https://github.com/matthew-brett/delocate) /
[Windows](https://github.com/adang1345/delvewheel) equivalents),
and repeating the build for every targeted Python version.
This is achieved with:

```shell
pip install --group distrib
cibuildwheel
```

Generation of the portable wheels is ensured by [cibuildwheel](https://cibuildwheel.pypa.io/),
whose configuration is in `pyproject.toml`.
The portable wheels are generated in directory `wheelhouse`.

Run locally, `cibuildwheel` only produces the wheels that can be built on the local
OS and architecture (e.g. `manylinux` wheels on a Linux host).
The complete set of wheels published to PyPI is built by the deployment workflow `.github/workflows/deploy.yaml`.
This procedure is documented for reference only: developers do not need to generate
portable wheels themselves, as this is handled by the deployment workflow.

import hipop
import importlib.metadata


def test_version_consistency() -> None:

    # "Package version" is the version defined in `cpp/hipop/__init__.py`.
    package_version = hipop.__version__ # type: ignore[missing-attribute]

    # "Distribution version" is the version defined in `pyproject.toml`.
    distribution_version = importlib.metadata.version("HiPOP") # Name of the project in `pyproject.toml`

    assert package_version == distribution_version, "Package version and distribution version must be identical"

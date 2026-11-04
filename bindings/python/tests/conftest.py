import ctypes
import os
import sys
from collections.abc import Iterator
from pathlib import Path

import maplibre_native_ffi as mln
import pytest
from support import Harness


def pytest_configure(config: pytest.Config) -> None:
    # Registered here rather than in pyproject.toml, because the Android device
    # run copies only this directory.
    config.addinivalue_line(
        "markers",
        "global_state: changes process-wide native state and restores its default",
    )
    _load_host_graphics()


def _load_host_graphics() -> None:
    # macOS has no EGL, so the host loads ANGLE before it renders, and the
    # library uses the copy the process holds. The OpenGL build installs ANGLE
    # beside the library for tests, and this suite stands in for that host.
    install = os.environ.get("MAPLIBRE_NATIVE_C_INSTALL_DIR")
    if sys.platform != "darwin" or not install:
        return
    for name in ("libGLESv2.dylib", "libEGL.dylib"):
        library = Path(install) / "lib" / name
        if library.is_file():
            ctypes.CDLL(str(library))


@pytest.fixture
def harness() -> Iterator[Harness]:
    fixture = Harness()
    try:
        yield fixture
    finally:
        fixture.release()


@pytest.fixture
def map_handle(harness: Harness) -> mln.MapHandle:
    return harness.create_map()

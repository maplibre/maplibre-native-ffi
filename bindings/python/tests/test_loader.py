import os
from pathlib import Path

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import _loader, _native
from support import result


def test_the_package_loads_its_extension_and_finds_native_directories_once(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    package = Path(_loader.__file__).resolve().parent
    # The wheel carries the extension, which links the native library in.
    assert Path(_native.__file__).resolve().parent == package
    with mln.runtime_create() as runtime:
        assert result(runtime.barrier()) is None

    # On Windows the loader also registers these directories for the native
    # library's dependencies: each existing one once, in this order.
    install = tmp_path / "install"
    (install / "bin").mkdir(parents=True)
    extra = tmp_path / "extra"
    extra.mkdir()
    monkeypatch.setenv("MAPLIBRE_NATIVE_C_INSTALL_DIR", str(install))
    monkeypatch.setenv(
        "MAPLIBRE_NATIVE_C_RUNTIME_LIBRARY_DIRS",
        os.pathsep.join([str(extra), str(tmp_path / "missing"), str(install / "bin")]),
    )

    assert _loader._native_loader_dirs() == [package, install / "bin", extra]

import importlib.util
import sys
from pathlib import Path

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import _native
from support import result

RUNTIMES = (
    "maplibre_native_ffi_runtime_opengl",
    "maplibre_native_ffi_runtime_metal",
    "maplibre_native_ffi_runtime_vulkan",
)


def test_the_package_loads_the_extension_of_its_runtime() -> None:
    # The runtime carries the extension, which links the native library in.
    (runtime,) = (name for name in RUNTIMES if name in sys.modules)
    assert _native is sys.modules[f"{runtime}._native"]
    with mln.RuntimeHandle.create() as handle:
        result(handle.clear_resource_transform())


def _run_the_runtime_lookup() -> None:
    # Runs the module that the package imports as `_native`, under another name.
    path = Path(mln.__file__).parent / "_native.py"
    spec = importlib.util.spec_from_file_location("runtime_lookup", path)
    assert spec is not None and spec.loader is not None
    spec.loader.exec_module(importlib.util.module_from_spec(spec))


def test_a_second_runtime_refuses_the_import(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    for name in RUNTIMES:
        (tmp_path / name).mkdir()
        (tmp_path / name / "__init__.py").touch()
    monkeypatch.syspath_prepend(tmp_path)

    with pytest.raises(ImportError, match="install exactly one runtime"):
        _run_the_runtime_lookup()


def test_a_missing_runtime_refuses_the_import(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(sys, "path", [str(tmp_path)])
    for name in RUNTIMES:
        monkeypatch.delitem(sys.modules, name, raising=False)

    with pytest.raises(
        ImportError, match="no MapLibre Native runtime is installed; install one of"
    ):
        _run_the_runtime_lookup()

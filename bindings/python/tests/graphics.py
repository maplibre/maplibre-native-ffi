"""GPU contexts from tests/graphics, loaded with ctypes."""

from __future__ import annotations

import ctypes
import os
import sys
from functools import cache
from pathlib import Path

METAL = 1
VULKAN = 2
EGL = 3
WGL = 4


class Context(ctypes.Structure):
    """mln_test_graphics_context. Only the backend's own fields are set."""

    _fields_ = (
        ("backend", ctypes.c_uint32),
        ("vulkan_queue_family_index", ctypes.c_uint32),
        ("metal_device", ctypes.c_void_p),
        ("vulkan_instance", ctypes.c_void_p),
        ("vulkan_physical_device", ctypes.c_void_p),
        ("vulkan_device", ctypes.c_void_p),
        ("vulkan_queue", ctypes.c_void_p),
        ("vulkan_get_instance_proc_addr", ctypes.c_void_p),
        ("vulkan_get_device_proc_addr", ctypes.c_void_p),
        ("egl_display", ctypes.c_void_p),
        ("egl_config", ctypes.c_void_p),
        ("egl_context", ctypes.c_void_p),
        ("wgl_device_context", ctypes.c_void_p),
        ("wgl_context", ctypes.c_void_p),
        ("get_proc_address", ctypes.c_void_p),
    )


def _library_path() -> Path:
    name = {
        "win32": "mln_test_graphics.dll",
        "darwin": "libmln_test_graphics.dylib",
    }.get(sys.platform, "libmln_test_graphics.so")
    candidates = []
    install = os.environ.get("MAPLIBRE_NATIVE_C_INSTALL_DIR")
    if install:
        candidates += [Path(install) / "bin" / name, Path(install) / "lib" / name]
    # The Android device run copies the library in beside the tests.
    candidates.append(Path(__file__).resolve().parents[1] / "build" / "graphics" / name)
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise FileNotFoundError(f"{name} is in none of {[str(c) for c in candidates]}")


@cache
def _library() -> ctypes.CDLL:
    library = ctypes.CDLL(str(_library_path()))
    library.mln_test_graphics_last_error.restype = ctypes.c_char_p
    library.mln_test_graphics_create.argtypes = (ctypes.c_uint32,)
    library.mln_test_graphics_create.restype = ctypes.c_void_p
    library.mln_test_graphics_destroy.argtypes = (ctypes.c_void_p,)
    library.mln_test_graphics_destroy.restype = None
    library.mln_test_graphics_get_context.argtypes = (
        ctypes.c_void_p,
        ctypes.POINTER(Context),
    )
    library.mln_test_graphics_get_context.restype = ctypes.c_bool
    library.mln_test_graphics_make_current.argtypes = (ctypes.c_void_p,)
    library.mln_test_graphics_make_current.restype = ctypes.c_bool
    return library


def _failure(call: str) -> RuntimeError:
    reason = _library().mln_test_graphics_last_error().decode(errors="replace")
    return RuntimeError(f"{call} failed: {reason}")


class Graphics:
    """A device or context that stands in for the host's.

    It belongs to one thread at a time. Close it after every session that
    borrows it has detached.
    """

    def __init__(self, backend: int) -> None:
        library = _library()
        handle = library.mln_test_graphics_create(backend)
        if not handle:
            raise _failure("mln_test_graphics_create")
        context = Context()
        if not library.mln_test_graphics_get_context(handle, ctypes.byref(context)):
            error = _failure("mln_test_graphics_get_context")
            library.mln_test_graphics_destroy(handle)
            raise error
        self._handle: int | None = handle
        self.context = context

    def make_current(self) -> None:
        """Make an EGL or WGL context current on the calling thread."""
        if not _library().mln_test_graphics_make_current(self._handle):
            raise _failure("mln_test_graphics_make_current")

    def close(self) -> None:
        if self._handle is not None:
            _library().mln_test_graphics_destroy(self._handle)
            self._handle = None

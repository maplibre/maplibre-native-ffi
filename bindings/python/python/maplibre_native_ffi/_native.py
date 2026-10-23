"""Stands in for the native extension of the one installed runtime.

Each runtime distribution installs the extension for its render backend as
``<module>._native``, under a top-level module of its own. Importing this module
imports that extension and replaces this module with it in ``sys.modules``, so
the binding reaches the extension as ``maplibre_native_ffi._native``. The import
raises ``ImportError`` unless exactly one runtime is installed.
"""

import sys
from importlib import import_module
from importlib.util import find_spec

# Each runtime's top-level module, and the distribution that installs it.
RUNTIMES = {
    "maplibre_native_ffi_runtime_opengl": "maplibre-native-ffi-runtime-opengl",
    "maplibre_native_ffi_runtime_metal": "maplibre-native-ffi-runtime-metal",
    "maplibre_native_ffi_runtime_vulkan": "maplibre-native-ffi-runtime-vulkan",
}

_installed = [module for module in RUNTIMES if find_spec(module) is not None]
if not _installed:
    raise ImportError(
        "no MapLibre Native runtime is installed; install one of "
        + ", ".join(RUNTIMES.values())
    )
if len(_installed) > 1:
    raise ImportError(
        " and ".join(RUNTIMES[module] for module in _installed)
        + " are installed; install exactly one runtime"
    )
sys.modules[__name__] = import_module(f"{_installed[0]}._native")

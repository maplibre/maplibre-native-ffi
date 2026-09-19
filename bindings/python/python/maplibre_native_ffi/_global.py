"""Process-global entry points for the Python binding."""

from . import _native
from .render import OpenGLContextProvider, RenderBackend
from .runtime import NetworkStatus

EXPECTED_C_ABI_VERSION: int = int(_native.expected_c_abi_version())


def c_version() -> int:
    """Return the native C ABI contract version."""
    return int(_native.c_version())


def supported_render_backends() -> RenderBackend:
    """Return render backends compiled into the linked native library."""
    return RenderBackend(_native.supported_render_backends_raw())


def supported_opengl_context_providers() -> OpenGLContextProvider:
    """Return OpenGL context providers compiled into the linked native library."""
    return OpenGLContextProvider(_native.supported_opengl_context_providers_raw())


def network_status() -> NetworkStatus:
    """Return MapLibre Native's process-global network status."""
    return NetworkStatus(_native.network_status_raw())


def load_plugin(path: str, entry_point: str) -> None:
    """Load a layer plugin shared library and register its style layer types.

    ``path`` is the UTF-8 path of the plugin library, and ``entry_point``
    names the exported entry point that the library calls with the
    process-wide register function. The library stays loaded for the process
    lifetime, because registration retains the plugin's callbacks.

    This function is callable from any thread. Call it before any style that
    uses the plugin's layer types loads. Loading an identical plugin again
    succeeds.

    Raises :class:`InvalidArgumentError` when ``path`` or ``entry_point`` is
    empty, and :class:`NativeError` with the OS or plugin diagnostic when the
    library fails to load, the entry point fails to resolve, or registration
    fails.
    """
    _native.load_plugin(path, entry_point)


def set_network_status(status: NetworkStatus) -> None:
    """Set MapLibre Native's process-global network status."""
    network_status_value = (
        status if isinstance(status, NetworkStatus) else NetworkStatus(status)
    )
    _native.set_network_status_raw(network_status_value.native_code)

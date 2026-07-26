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


def set_network_status(status: NetworkStatus) -> None:
    """Set MapLibre Native's process-global network status."""
    network_status_value = (
        status if isinstance(status, NetworkStatus) else NetworkStatus(status)
    )
    _native.set_network_status_raw(network_status_value.native_code)

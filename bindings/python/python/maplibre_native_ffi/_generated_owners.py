"""Generated from the C headers by tools/bindgen. Do not edit."""

from concurrent.futures import Future

from ._future import completed_future
from ._generated_operations import (
    _AcquiredFrameHandleOperations,
    _FramePacerHandleOperations,
    _GpuDeviceHandleOperations,
    _MapHandleOperations,
    _MapProjectionHandleOperations,
    _RenderSessionHandleOperations,
    _ResourceRequestHandleOperations,
    _ResourceTransformRequestHandleOperations,
    _RuntimeHandleOperations,
    _StyleExpressionHandleOperations,
)
from ._lifecycle import NativeHandleMixin, ScopedHandleMixin


class AcquiredFrameHandle(_AcquiredFrameHandleOperations, NativeHandleMixin):
    """A rendered frame that the caller holds to read or show.

    See `mln_acquired_frame` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    __module__ = "maplibre_native_ffi"
    _handle_name = "AcquiredFrameHandle"

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner

    @property
    def id(self) -> int:
        """The native handle value. It stays readable after release."""
        return int(self._native.id)


class FramePacerHandle(_FramePacerHandleOperations, ScopedHandleMixin):
    """Demands render sessions' frames at a display's refreshes.

    See `mln_frame_pacer` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    __module__ = "maplibre_native_ffi"
    _handle_name = "FramePacerHandle"

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner

    @property
    def id(self) -> int:
        """The native handle value. It stays readable after release."""
        return int(self._native.id)


class GpuDeviceHandle(_GpuDeviceHandleOperations, ScopedHandleMixin):
    """A GPU, and the queue that render sessions on it share.

    See `mln_gpu_device` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    __module__ = "maplibre_native_ffi"
    _handle_name = "GpuDeviceHandle"

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner

    @property
    def id(self) -> int:
        """The native handle value. It stays readable after release."""
        return int(self._native.id)


class MapHandle(_MapHandleOperations, ScopedHandleMixin):
    """A map, with its style and camera.

    See `mln_map` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    __module__ = "maplibre_native_ffi"
    _handle_name = "MapHandle"
    _ready: Future[None]

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None, ready=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        owner._ready = completed_future() if ready is None else ready
        return owner

    @property
    def ready(self) -> Future[None]:
        """Completes once the map is ready, and fails with `MaplibreError` when it
        fails to become ready. Asynchronous calls on the map before then wait
        for it."""
        return self._ready

    @property
    def id(self) -> int:
        """The native handle value, which runtime events report as their source.

        It stays readable after release, so events drained later still match
        the handle that produced them.
        """
        return int(self._native.id)

    def dispose(self) -> None:
        """Releases the map in any state, and finishes in the background.

        Use it from destructors and finalizers. Otherwise, use
        `MapHandle.release()`.

        See `mln_map_dispose` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        self._native._dispose()


class MapProjectionHandle(_MapProjectionHandleOperations, ScopedHandleMixin):
    """A camera view for converting between screen points and coordinates.

    See `mln_map_projection` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    __module__ = "maplibre_native_ffi"
    _handle_name = "MapProjectionHandle"

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner

    @property
    def id(self) -> int:
        """The native handle value. It stays readable after release."""
        return int(self._native.id)


class RenderSessionHandle(_RenderSessionHandleOperations, ScopedHandleMixin):
    """A render session draws a map into a surface or a texture.

    See `mln_render_session` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    __module__ = "maplibre_native_ffi"
    _handle_name = "RenderSessionHandle"
    _ready: Future[None]

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None, ready=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        owner._ready = completed_future() if ready is None else ready
        return owner

    @property
    def ready(self) -> Future[None]:
        """Completes once the session is ready, and fails with `MaplibreError` when it
        fails to become ready. Asynchronous calls on the session before then wait
        for it."""
        return self._ready

    @property
    def id(self) -> int:
        """The native handle value. It stays readable after release."""
        return int(self._native.id)

    def dispose(self) -> None:
        """Releases the session in any state, and finishes in the background.

        Use it from destructors and finalizers, and
        `RenderSessionHandle.release()` elsewhere. It ends the session as a
        release does, except that it abandons a session whose frames you
        hold, which ends those frames. Keep servicing a session driven by
        `RenderDriverKind.CALLER_GRAPHICS_THREAD` until
        `RenderSessionHandle.service_driver_work()` no longer accepts it.

        See `mln_render_session_dispose` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        self._native._dispose()


class ResourceRequestHandle(_ResourceRequestHandleOperations, ScopedHandleMixin):
    """A resource load that a resource provider answers.

    See `mln_resource_request` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    __module__ = "maplibre_native_ffi"
    _handle_name = "ResourceRequestHandle"

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner

    @property
    def id(self) -> int:
        """The native handle value. It stays readable after release."""
        return int(self._native.id)


class ResourceTransformRequestHandle(
    _ResourceTransformRequestHandleOperations, ScopedHandleMixin
):
    """A URL rewrite that a resource transform answers with the URL to load.

    See `mln_resource_transform_request` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    __module__ = "maplibre_native_ffi"
    _handle_name = "ResourceTransformRequestHandle"

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner

    @property
    def id(self) -> int:
        """The native handle value. It stays readable after release."""
        return int(self._native.id)


class RuntimeHandle(_RuntimeHandleOperations, ScopedHandleMixin):
    """A runtime owns maps and the network and cache settings that they share.

    See `mln_runtime` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    __module__ = "maplibre_native_ffi"
    _handle_name = "RuntimeHandle"

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner

    @property
    def id(self) -> int:
        """The native handle value, which runtime events report as their source.

        It stays readable after release, so events drained later still match
        the handle that produced them.
        """
        return int(self._native.id)

    def dispose(self) -> None:
        """Releases a runtime in any state, without waiting.

        The runtime finishes its teardown after its maps are released.

        See `mln_runtime_dispose` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        self._native._dispose()


class StyleExpressionHandle(_StyleExpressionHandleOperations, ScopedHandleMixin):
    """A parsed style expression.

    See `mln_style_expression` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    __module__ = "maplibre_native_ffi"
    _handle_name = "StyleExpressionHandle"

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner

    @property
    def id(self) -> int:
        """The native handle value. It stays readable after release."""
        return int(self._native.id)

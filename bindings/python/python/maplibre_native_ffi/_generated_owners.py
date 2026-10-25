"""Generated from the C headers by tools/bindgen. Do not edit."""

from ._generated_operations import (
    _AcquiredFrameHandleOperations,
    _GeojsonSourceDataHandleOperations,
    _HttpHeaderTransformResponseScopeOperations,
    _MapHandleOperations,
    _MapProjectionHandleOperations,
    _RenderSessionHandleOperations,
    _ResourceRequestHandleOperations,
    _ResourceTransformRequestHandleOperations,
    _RuntimeHandleOperations,
    _StyleExpressionHandleOperations,
)
from ._lifecycle import NativeHandleMixin, ScopedHandleMixin


class HttpHeaderTransformResponseScope(_HttpHeaderTransformResponseScopeOperations):
    __module__ = "maplibre_native_ffi"

    def __init__(self):
        raise TypeError("response scopes are supplied during callbacks")

    @classmethod
    def _from_native(cls, native):
        value = cls.__new__(cls)
        value._native = native
        return value


class AcquiredFrameHandle(_AcquiredFrameHandleOperations, NativeHandleMixin):
    """A rendered frame that a render session lends until its release.

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
        """The native handle value. It stays readable after close."""
        return int(self._native.id)


class GeojsonSourceDataHandle(_GeojsonSourceDataHandleOperations, ScopedHandleMixin):
    """Prepared GeoJSON data that sources retain independently of its handle.

    See `mln_geojson_source_data` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    __module__ = "maplibre_native_ffi"
    _handle_name = "GeojsonSourceDataHandle"

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
        """The native handle value. It stays readable after close."""
        return int(self._native.id)


class MapHandle(_MapHandleOperations, ScopedHandleMixin):
    """A map, which holds map state independent of any render target.

    See `mln_map` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    __module__ = "maplibre_native_ffi"
    _handle_name = "MapHandle"

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

        It stays readable after close, so events drained later still match
        the handle that produced them.
        """
        return int(self._native.id)

    def dispose(self) -> None:
        """Consumes a map handle without observing its asynchronous retirement.

        See `mln_map_dispose` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
        """
        self._native._exit()


class MapProjectionHandle(_MapProjectionHandleOperations, ScopedHandleMixin):
    """A standalone projection of one map view.

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
        """The native handle value. It stays readable after close."""
        return int(self._native.id)


class RenderSessionHandle(_RenderSessionHandleOperations, ScopedHandleMixin):
    """A render session, which renders one map to one render target.

    See `mln_render_session` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    __module__ = "maplibre_native_ffi"
    _handle_name = "RenderSessionHandle"

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
        """The native handle value. It stays readable after close."""
        return int(self._native.id)

    def dispose(self) -> None:
        """Consumes a session in any state and retires it, then runs completion,
        if given, once the host may destroy the target's graphics objects.

        See `mln_render_session_dispose` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
        """
        self._native._exit()


class ResourceRequestHandle(_ResourceRequestHandleOperations, ScopedHandleMixin):
    """A resource request that a resource provider handles.

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
        """The native handle value. It stays readable after close."""
        return int(self._native.id)


class ResourceTransformRequestHandle(
    _ResourceTransformRequestHandleOperations, ScopedHandleMixin
):
    """A resource URL transform that waits for its host's answer.

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
        """The native handle value. It stays readable after close."""
        return int(self._native.id)


class RuntimeHandle(_RuntimeHandleOperations, ScopedHandleMixin):
    """A runtime: the native scheduler thread and event store for its maps.

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

        It stays readable after close, so events drained later still match
        the handle that produced them.
        """
        return int(self._native.id)

    def dispose(self) -> None:
        """Consumes a runtime handle without observing its asynchronous
        retirement.

        See `mln_runtime_dispose` in the
        [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
        """
        self._native._exit()


class StyleExpressionHandle(_StyleExpressionHandleOperations, ScopedHandleMixin):
    """A parsed style expression, immutable and independent of any map.

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
        """The native handle value. It stays readable after close."""
        return int(self._native.id)

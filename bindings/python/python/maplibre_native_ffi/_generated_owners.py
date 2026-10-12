"""Generated from the C headers by tools/bindgen. Do not edit."""

from ._generated_operations import (
    _AcquiredFrameHandleOperations,
    _EventBatchHandleOperations,
    _GeojsonSourceDataHandleOperations,
    _HttpHeaderTransformResponseScopeOperations,
    _MapHandleOperations,
    _MapProjectionHandleOperations,
    _RenderFrameBatchHandleOperations,
    _RenderSessionHandleOperations,
    _ResourceRequestHandleOperations,
    _ResourceTransformResponseScopeOperations,
    _RuntimeHandleOperations,
)
from ._lifecycle import NativeHandleMixin


class HttpHeaderTransformResponseScope(_HttpHeaderTransformResponseScopeOperations):
    def __init__(self):
        raise TypeError("response scopes are supplied during callbacks")

    @classmethod
    def _from_native(cls, native):
        value = cls.__new__(cls)
        value._native = native
        return value


class ResourceTransformResponseScope(_ResourceTransformResponseScopeOperations):
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

    _handle_name = "AcquiredFrameHandle"
    _parent: NativeHandleMixin | None

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner


class EventBatchHandle(_EventBatchHandleOperations, NativeHandleMixin):
    """An owned batch of runtime events from one drain.

    See `mln_event_batch` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    _handle_name = "EventBatchHandle"
    _parent: NativeHandleMixin | None

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner


class GeojsonSourceDataHandle(_GeojsonSourceDataHandleOperations, NativeHandleMixin):
    _handle_name = "GeojsonSourceDataHandle"
    _parent: NativeHandleMixin | None

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner


class MapHandle(_MapHandleOperations, NativeHandleMixin):
    """A map, which holds map state independent of any render target.

    See `mln_map` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    _handle_name = "MapHandle"
    _parent: NativeHandleMixin | None

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner


class MapProjectionHandle(_MapProjectionHandleOperations, NativeHandleMixin):
    """A standalone projection of a map's transform state at its creation.

    See `mln_map_projection` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    _handle_name = "MapProjectionHandle"
    _parent: NativeHandleMixin | None

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner


class RenderFrameBatchHandle(_RenderFrameBatchHandleOperations, NativeHandleMixin):
    """An owned batch of frame results from one drain.

    See `mln_render_frame_batch` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    _handle_name = "RenderFrameBatchHandle"
    _parent: NativeHandleMixin | None

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner


class RenderSessionHandle(_RenderSessionHandleOperations, NativeHandleMixin):
    """A render session, which renders one map to one render target.

    See `mln_render_session` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    _handle_name = "RenderSessionHandle"
    _parent: NativeHandleMixin | None

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner


class ResourceRequestHandle(_ResourceRequestHandleOperations, NativeHandleMixin):
    """A resource request that a resource provider handles.

    See `mln_resource_request_handle` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    _handle_name = "ResourceRequestHandle"
    _parent: NativeHandleMixin | None

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner


class RuntimeHandle(_RuntimeHandleOperations, NativeHandleMixin):
    """A runtime: the native scheduler thread and event store for its maps.

    See `mln_runtime` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    _handle_name = "RuntimeHandle"
    _parent: NativeHandleMixin | None

    def __init__(self):
        raise TypeError("native handles are returned by their owning operations")

    @classmethod
    def _from_native(cls, native, parent=None):
        owner = cls.__new__(cls)
        owner._native = native
        owner._parent = parent
        return owner

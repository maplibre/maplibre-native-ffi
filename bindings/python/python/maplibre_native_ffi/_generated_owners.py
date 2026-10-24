"""Generated from the C headers by tools/bindgen. Do not edit."""

from ._generated_operations import (
    _AcquiredFrameHandleOperations,
    _GeojsonSourceDataHandleOperations,
    _HttpHeaderTransformResponseScopeOperations,
    _MapHandleOperations,
    _MapProjectionHandleOperations,
    _RenderFrameBatchHandleOperations,
    _RenderSessionHandleOperations,
    _ResourceRequestHandleOperations,
    _ResourceTransformRequestHandleOperations,
    _RuntimeHandleOperations,
    _StyleExpressionHandleOperations,
)
from ._lifecycle import NativeHandleMixin


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


class GeojsonSourceDataHandle(_GeojsonSourceDataHandleOperations, NativeHandleMixin):
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


class MapHandle(_MapHandleOperations, NativeHandleMixin):
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


class MapProjectionHandle(_MapProjectionHandleOperations, NativeHandleMixin):
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


class RenderFrameBatchHandle(_RenderFrameBatchHandleOperations, NativeHandleMixin):
    """An owned batch of frame results from one drain.

    See `mln_render_frame_batch` in the
    [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    """

    __module__ = "maplibre_native_ffi"
    _handle_name = "RenderFrameBatchHandle"

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


class ResourceRequestHandle(_ResourceRequestHandleOperations, NativeHandleMixin):
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


class ResourceTransformRequestHandle(
    _ResourceTransformRequestHandleOperations, NativeHandleMixin
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


class RuntimeHandle(_RuntimeHandleOperations, NativeHandleMixin):
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


class StyleExpressionHandle(_StyleExpressionHandleOperations, NativeHandleMixin):
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

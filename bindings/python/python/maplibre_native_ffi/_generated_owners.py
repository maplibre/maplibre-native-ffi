"""Generated from the C headers by tools/bindgen. Do not edit."""

from ._generated_operations import (
    _AcquiredFrameHandleOperations,
    _BufferHandleOperations,
    _EventBatchHandleOperations,
    _GeoJsonSourceDataHandleOperations,
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


class BufferHandle(_BufferHandleOperations, NativeHandleMixin):
    _handle_name = "BufferHandle"
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


class GeoJsonSourceDataHandle(_GeoJsonSourceDataHandleOperations, NativeHandleMixin):
    _handle_name = "GeoJsonSourceDataHandle"
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

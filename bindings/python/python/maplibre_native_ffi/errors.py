"""Generated from C headers by tools/bindgen. Do not edit."""

from ._error import MaplibreError
from ._generated_values import Status


class InvalidArgumentError(MaplibreError):
    """Error for the invalid argument status."""

    status = Status.INVALID_ARGUMENT


class InvalidStateError(MaplibreError):
    """Error for the invalid state status."""

    status = Status.INVALID_STATE


class WrongThreadError(MaplibreError):
    """Error for the wrong thread status."""

    status = Status.WRONG_THREAD


class UnsupportedFeatureError(MaplibreError):
    """Error for the unsupported status."""

    status = Status.UNSUPPORTED


class NativeError(MaplibreError):
    """Error for the native error status."""

    status = Status.NATIVE_ERROR


class CancelledError(MaplibreError):
    """Error for the cancelled status."""

    status = Status.CANCELLED


class BusyError(MaplibreError):
    """Error for the busy status."""

    status = Status.BUSY


class TargetLostError(MaplibreError):
    """Error for the target lost status."""

    status = Status.TARGET_LOST


class NotReadyError(MaplibreError):
    """Error for the not ready status."""

    status = Status.NOT_READY


class NotFoundError(MaplibreError):
    """Error for the not found status."""

    status = Status.NOT_FOUND


class UnknownStatusError(MaplibreError):
    """Error for an unknown native status."""


__all__ = [
    "BusyError",
    "CancelledError",
    "InvalidArgumentError",
    "InvalidStateError",
    "MaplibreError",
    "NativeError",
    "NotFoundError",
    "NotReadyError",
    "TargetLostError",
    "UnknownStatusError",
    "UnsupportedFeatureError",
    "WrongThreadError",
]

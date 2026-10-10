"""Exception and status types for the Python binding."""

from ._generated_values import Status


class MaplibreError(Exception):
    """Base class for MapLibre Native binding errors.

    ``native_status_code`` is the C ABI status the native library reported, and
    None for a failure this binding raised on its own.
    """

    status: Status | None = None
    """Status category every instance of this exception type carries."""

    def __init__(
        self, diagnostic: str = "", native_status_code: int | None = None
    ) -> None:
        self.diagnostic = diagnostic
        self.native_status_code = native_status_code
        if native_status_code is not None:
            self.status = Status(native_status_code)
        super().__init__(
            diagnostic
            or (
                self.status.name.lower().replace("_", " ")
                if self.status is not None
                else "native error"
            )
        )


class InvalidArgumentError(MaplibreError):
    """Error for invalid C ABI arguments or invalid Python-owned inputs."""

    status = Status.INVALID_ARGUMENT


class InvalidStateError(MaplibreError):
    """Error for otherwise valid objects in the wrong lifecycle state."""

    status = Status.INVALID_STATE


class WrongThreadError(MaplibreError):
    """Error for thread-affine native handles called from the wrong thread."""

    status = Status.WRONG_THREAD


class UnsupportedFeatureError(MaplibreError):
    """Error for entry points or requested behavior unavailable in this build."""

    status = Status.UNSUPPORTED


class NativeError(MaplibreError):
    """Error for native MapLibre failures converted to C status."""

    status = Status.NATIVE_ERROR


class CancelledError(MaplibreError):
    """Error for an operation that reached its cancelled disposition."""

    status = Status.CANCELLED


class BusyError(MaplibreError):
    """Error for a conflicting driver call or lifecycle transition."""

    status = Status.BUSY


class TargetLostError(MaplibreError):
    """Error for an irreversibly lost render target or graphics receiver."""

    status = Status.TARGET_LOST


class NotReadyError(MaplibreError):
    """Error for a nonblocking call that has no result yet."""

    status = Status.NOT_READY


class NotFoundError(MaplibreError):
    """Error for a lookup whose ID names no object."""

    status = Status.NOT_FOUND


class UnknownStatusError(MaplibreError):
    """Error for future native status values unknown to this binding."""


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

"""Exception and status types for the Python binding."""

from enum import Enum
from typing import ClassVar


class MaplibreStatus(Enum):
    """Stable status categories reported by the native C ABI."""

    OK = 0
    INVALID_ARGUMENT = -1
    INVALID_STATE = -2
    WRONG_THREAD = -3
    UNSUPPORTED = -4
    NATIVE_ERROR = -5
    CANCELLED = -6
    BUSY = -7
    TARGET_LOST = -8
    NOT_READY = -9
    NOT_FOUND = -10
    UNKNOWN = None

    @property
    def native_code(self) -> int | None:
        """Return the C status value for known status categories."""
        return self.value

    @classmethod
    def _from_native(cls, native_code: int) -> MaplibreStatus:
        """Return the Python status category for a C ABI status value."""
        for status in cls:
            if status.value == native_code:
                return status
        return cls.UNKNOWN


class MaplibreError(Exception):
    """Base class for MapLibre Native binding errors.

    ``native_status_code`` is the C ABI status the native library reported, and
    None for a failure this binding raised on its own.
    """

    status: ClassVar[MaplibreStatus] = MaplibreStatus.UNKNOWN
    """Status category every instance of this exception type carries."""

    def __init__(
        self, diagnostic: str = "", native_status_code: int | None = None
    ) -> None:
        self.diagnostic = diagnostic
        self.native_status_code = native_status_code
        super().__init__(diagnostic or self.status.name.lower().replace("_", " "))


class InvalidArgumentError(MaplibreError):
    """Error for invalid C ABI arguments or invalid Python-owned inputs."""

    status = MaplibreStatus.INVALID_ARGUMENT


class InvalidStateError(MaplibreError):
    """Error for otherwise valid objects in the wrong lifecycle state."""

    status = MaplibreStatus.INVALID_STATE


class WrongThreadError(MaplibreError):
    """Error for thread-affine native handles called from the wrong thread."""

    status = MaplibreStatus.WRONG_THREAD


class UnsupportedFeatureError(MaplibreError):
    """Error for entry points or requested behavior unavailable in this build."""

    status = MaplibreStatus.UNSUPPORTED


class NativeError(MaplibreError):
    """Error for native MapLibre failures converted to C status."""

    status = MaplibreStatus.NATIVE_ERROR


class CancelledError(MaplibreError):
    """Error for an operation that reached its cancelled disposition."""

    status = MaplibreStatus.CANCELLED


class BusyError(MaplibreError):
    """Error for a conflicting driver call or lifecycle transition."""

    status = MaplibreStatus.BUSY


class TargetLostError(MaplibreError):
    """Error for an irreversibly lost render target or graphics receiver."""

    status = MaplibreStatus.TARGET_LOST


class NotReadyError(MaplibreError):
    """Error for a nonblocking call that has no result yet."""

    status = MaplibreStatus.NOT_READY


class NotFoundError(MaplibreError):
    """Error for a lookup whose ID names no object."""

    status = MaplibreStatus.NOT_FOUND


class UnknownStatusError(MaplibreError):
    """Error for future native status values unknown to this binding."""

    status = MaplibreStatus.UNKNOWN


_ERROR_TYPES: dict[MaplibreStatus, type[MaplibreError]] = {
    MaplibreStatus.INVALID_ARGUMENT: InvalidArgumentError,
    MaplibreStatus.INVALID_STATE: InvalidStateError,
    MaplibreStatus.WRONG_THREAD: WrongThreadError,
    MaplibreStatus.UNSUPPORTED: UnsupportedFeatureError,
    MaplibreStatus.NATIVE_ERROR: NativeError,
    MaplibreStatus.CANCELLED: CancelledError,
    MaplibreStatus.BUSY: BusyError,
    MaplibreStatus.TARGET_LOST: TargetLostError,
    MaplibreStatus.NOT_READY: NotReadyError,
    MaplibreStatus.NOT_FOUND: NotFoundError,
}


def _from_native_status(native_status_code: int, diagnostic: str) -> MaplibreError:
    status = MaplibreStatus._from_native(native_status_code)
    error_type = _ERROR_TYPES.get(status, UnknownStatusError)
    return error_type(diagnostic, native_status_code)


__all__ = [
    "BusyError",
    "CancelledError",
    "InvalidArgumentError",
    "InvalidStateError",
    "MaplibreError",
    "MaplibreStatus",
    "NativeError",
    "NotFoundError",
    "NotReadyError",
    "TargetLostError",
    "UnknownStatusError",
    "UnsupportedFeatureError",
    "WrongThreadError",
]

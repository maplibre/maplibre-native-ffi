"""Exception and status types for the Python binding."""

from enum import Enum


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
    """Base class for MapLibre Native binding errors."""

    def __init__(
        self,
        status: MaplibreStatus,
        native_status_code: int | None = None,
        diagnostic: str = "",
    ) -> None:
        self.status = status
        self.native_status_code = native_status_code
        self.diagnostic = diagnostic
        message = diagnostic or status.name.lower().replace("_", " ")
        super().__init__(message)


class InvalidArgumentError(MaplibreError):
    """Error for invalid C ABI arguments or invalid Python-owned inputs."""

    def __init__(
        self, native_status_code: int | None = -1, diagnostic: str = ""
    ) -> None:
        super().__init__(
            MaplibreStatus.INVALID_ARGUMENT, native_status_code, diagnostic
        )


class InvalidStateError(MaplibreError):
    """Error for otherwise valid objects in the wrong lifecycle state."""

    def __init__(
        self, native_status_code: int | None = -2, diagnostic: str = ""
    ) -> None:
        super().__init__(MaplibreStatus.INVALID_STATE, native_status_code, diagnostic)


class WrongThreadError(MaplibreError):
    """Error for thread-affine native handles called from the wrong thread."""

    def __init__(
        self, native_status_code: int | None = -3, diagnostic: str = ""
    ) -> None:
        super().__init__(MaplibreStatus.WRONG_THREAD, native_status_code, diagnostic)


class UnsupportedFeatureError(MaplibreError):
    """Error for entry points or requested behavior unavailable in this build."""

    def __init__(
        self, native_status_code: int | None = -4, diagnostic: str = ""
    ) -> None:
        super().__init__(MaplibreStatus.UNSUPPORTED, native_status_code, diagnostic)


class NativeError(MaplibreError):
    """Error for native MapLibre failures converted to C status."""

    def __init__(
        self, native_status_code: int | None = -5, diagnostic: str = ""
    ) -> None:
        super().__init__(MaplibreStatus.NATIVE_ERROR, native_status_code, diagnostic)


class CancelledError(MaplibreError):
    """Error for an operation that reached its cancelled disposition."""

    def __init__(
        self, native_status_code: int | None = -6, diagnostic: str = ""
    ) -> None:
        super().__init__(MaplibreStatus.CANCELLED, native_status_code, diagnostic)


class BusyError(MaplibreError):
    """Error for a conflicting driver call or lifecycle transition."""

    def __init__(
        self, native_status_code: int | None = -7, diagnostic: str = ""
    ) -> None:
        super().__init__(MaplibreStatus.BUSY, native_status_code, diagnostic)


class TargetLostError(MaplibreError):
    """Error for an irreversibly lost render target or graphics receiver."""

    def __init__(
        self, native_status_code: int | None = -8, diagnostic: str = ""
    ) -> None:
        super().__init__(MaplibreStatus.TARGET_LOST, native_status_code, diagnostic)


class NotReadyError(MaplibreError):
    """Error for a nonblocking call that has no result yet."""

    def __init__(
        self, native_status_code: int | None = -9, diagnostic: str = ""
    ) -> None:
        super().__init__(MaplibreStatus.NOT_READY, native_status_code, diagnostic)


class NotFoundError(MaplibreError):
    """Error for a lookup whose ID names no object."""

    def __init__(
        self, native_status_code: int | None = -10, diagnostic: str = ""
    ) -> None:
        super().__init__(MaplibreStatus.NOT_FOUND, native_status_code, diagnostic)


class UnknownStatusError(MaplibreError):
    """Error for future native status values unknown to this binding."""

    def __init__(self, native_status_code: int, diagnostic: str = "") -> None:
        super().__init__(MaplibreStatus.UNKNOWN, native_status_code, diagnostic)


class _OperationResultConsumedError(Exception):
    """Internal marker for a failed adaptation after native result transfer."""


def _from_native_status(native_status_code: int, diagnostic: str) -> MaplibreError:
    error_types: dict[MaplibreStatus, type[MaplibreError]] = {
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
    status = MaplibreStatus._from_native(native_status_code)
    error_type = error_types.get(status)
    if error_type is None:
        return UnknownStatusError(native_status_code, diagnostic)
    return error_type(native_status_code, diagnostic)


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

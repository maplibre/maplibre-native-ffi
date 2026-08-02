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
    UNKNOWN = None

    @property
    def native_code(self) -> int | None:
        """Return the C status value for known status categories."""
        return self.value

    @classmethod
    def _from_native(cls, native_code: int) -> "MaplibreStatus":
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


class UnknownStatusError(MaplibreError):
    """Error for future native status values unknown to this binding."""

    def __init__(self, native_status_code: int, diagnostic: str = "") -> None:
        super().__init__(MaplibreStatus.UNKNOWN, native_status_code, diagnostic)


__all__ = [
    "InvalidArgumentError",
    "InvalidStateError",
    "MaplibreError",
    "MaplibreStatus",
    "NativeError",
    "UnknownStatusError",
    "UnsupportedFeatureError",
    "WrongThreadError",
]

"""Process-global logging callback APIs."""

from __future__ import annotations

from ._enum import UnknownIntEnum
from dataclasses import dataclass
from enum import IntFlag
from typing import Any

from . import _native

_LOG_RECEIVER_CREATE_KEY = object()


class LogSeverity(UnknownIntEnum):
    """Log severity values emitted by MapLibre Native."""

    INFO = 1
    WARNING = 2
    ERROR = 3


class LogSeverityMask(IntFlag):
    """Mask of severities MapLibre Native may dispatch asynchronously."""

    INFO = 1 << LogSeverity.INFO
    WARNING = 1 << LogSeverity.WARNING
    ERROR = 1 << LogSeverity.ERROR
    DEFAULT = INFO | WARNING
    ALL = INFO | WARNING | ERROR


class LogEvent(UnknownIntEnum):
    """Log event categories emitted by MapLibre Native."""

    GENERAL = 0
    SETUP = 1
    SHADER = 2
    PARSE_STYLE = 3
    PARSE_TILE = 4
    RENDER = 5
    STYLE = 6
    DATABASE = 7
    HTTP_REQUEST = 8
    SPRITE = 9
    IMAGE = 10
    OPENGL = 11
    JNI = 12
    ANDROID = 13
    CRASH = 14
    GLYPH = 15
    TIMING = 16


@dataclass(frozen=True, slots=True)
class LogRecord:
    """Copied MapLibre Native log record."""

    severity: LogSeverity
    event: LogEvent
    code: int
    message: str

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> "LogRecord":
        """Build a copied log record from private native values."""
        return cls(
            severity=LogSeverity(raw["severity"]),
            event=LogEvent(raw["event"]),
            code=raw["code"],
            message=raw["message"],
        )


class LogReceiver:
    """Bounded queue receiving copied process-global native log records."""

    def __init__(self, native: Any, *, _create_key: object | None = None) -> None:
        if _create_key is not _LOG_RECEIVER_CREATE_KEY:
            msg = "LogReceiver instances are created by set_log_callback"
            raise TypeError(msg)
        self._native = native

    @classmethod
    def _from_native(cls, native: Any) -> "LogReceiver":
        return cls(native, _create_key=_LOG_RECEIVER_CREATE_KEY)

    @property
    def dropped_record_count(self) -> int:
        """Return records dropped because the queue was full."""
        return int(self._native.dropped_record_count)

    def poll_record(self) -> LogRecord | None:
        """Return one copied log record from the receiver queue."""
        record = self._native.poll_record()
        if record is None:
            return None
        return LogRecord._from_native(record)


def set_log_callback(
    *,
    max_queued_records: int = 1024,
    consume: bool = False,
) -> LogReceiver:
    """Install a process-global bounded log-record receiver."""
    return LogReceiver._from_native(
        _native.set_log_callback(max_queued_records, consume)
    )


def clear_log_callback() -> None:
    """Clear the process-global native log callback."""
    _native.clear_log_callback()


def set_async_log_severity_mask(mask: LogSeverityMask) -> None:
    """Configure severities that may dispatch asynchronously."""
    _native.set_async_log_severity_mask(int(mask))


__all__ = [
    "LogEvent",
    "LogReceiver",
    "LogRecord",
    "LogSeverity",
    "LogSeverityMask",
    "clear_log_callback",
    "set_async_log_severity_mask",
    "set_log_callback",
]

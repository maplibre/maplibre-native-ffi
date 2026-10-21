"""Exception and status types for the Python binding."""

from ._generated_values import Status


class MaplibreError(Exception):
    """A failed call's status and diagnostic."""

    def __init__(self, status: Status | int, diagnostic: str = "") -> None:
        self.status = Status(status)
        self.diagnostic = diagnostic
        super().__init__(f"{self.status.name}: {diagnostic}")


class AbiVersionMismatchError(MaplibreError):
    """The linked library's ABI version differs from the binding's version."""

    def __init__(self, expected: int, actual: int) -> None:
        self.expected = expected
        self.actual = actual
        super().__init__(
            Status.UNSUPPORTED,
            f"unsupported MapLibre Native C ABI version {actual}; expected {expected}",
        )

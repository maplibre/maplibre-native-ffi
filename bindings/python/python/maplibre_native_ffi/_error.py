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

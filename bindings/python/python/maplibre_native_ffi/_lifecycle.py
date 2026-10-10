"""Lifecycle behavior that every public handle shares."""

from __future__ import annotations

import warnings
from contextlib import suppress
from types import TracebackType
from typing import Any, Self


class NativeHandleMixin:
    """Mixin for public handles backed by a private `_native` handle."""

    _handle_name = "Handle"
    _native: Any

    @property
    def closed(self) -> bool:
        """Return whether the private native handle has been closed."""
        return bool(self._native.closed)

    @property
    def id(self) -> int:
        """Return the native handle ID that runtime events report as a source.

        The ID stays readable after close, so events drained later still match
        the handle that produced them.
        """
        return int(self._native.id)

    def close(self) -> object:
        """Release the private native handle exactly once.

        A handle whose release reports asynchronous teardown returns a future
        here. Leaving the ``with`` block discards it, so a host that must
        observe teardown calls ``close`` itself and waits.
        """
        return self._native.close()

    def __enter__(self) -> Self:
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc_value: BaseException | None,
        traceback: TracebackType | None,
    ) -> None:
        self.close()

    def __del__(self) -> None:
        # Finalizers cannot report warning-delivery failures, including during
        # interpreter shutdown when Python globals may be partially torn down.
        with suppress(BaseException):
            if not getattr(self, "closed", True):
                warnings.warn(
                    f"{self._handle_name} was not explicitly closed",
                    ResourceWarning,
                    stacklevel=1,
                )

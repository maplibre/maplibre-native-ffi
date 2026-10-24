"""Lifecycle behavior that every public handle shares."""

from __future__ import annotations

import warnings
from contextlib import suppress
from types import TracebackType
from typing import Any, Self


class NativeHandleMixin:
    """Lifecycle methods that every handle shares."""

    _handle_name = "Handle"
    _native: Any
    _parent: NativeHandleMixin | None

    @property
    def is_closed(self) -> bool:
        """Whether the binding has accepted a release, dispose, close, discard,
        or ``with`` block exit of the handle."""
        return bool(self._native._is_closed)

    def close(self) -> object:
        """Release the private native handle.

        Synchronous close is idempotent. Repeated asynchronous releases reach
        native and report an invalid-state error.

        A handle whose release reports asynchronous teardown returns a future
        here, and a host that must observe teardown calls ``close`` itself and
        waits. Leaving the ``with`` block disposes such a handle instead, as
        ``dispose`` does, without observing teardown. Leaving the block after an explicit close
        does nothing.
        """
        return self._native.close()

    def __repr__(self) -> str:
        closed = "true" if self.is_closed else "false"
        return f"{type(self).__name__}(id={self._native.id:#x}, closed={closed})"

    def __del__(self) -> None:
        # Finalizers cannot report warning-delivery failures, including during
        # interpreter shutdown when Python globals may be partially torn down.
        with suppress(BaseException):
            if hasattr(self, "_native") and not self._native._is_closed:
                warnings.warn(
                    f"{self._handle_name} was not explicitly closed",
                    ResourceWarning,
                    stacklevel=1,
                )


class ScopedHandleMixin(NativeHandleMixin):
    """The ``with`` block of a handle whose scope can clean it up."""

    def __enter__(self) -> Self:
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc_value: BaseException | None,
        traceback: TracebackType | None,
    ) -> None:
        self._native._exit()

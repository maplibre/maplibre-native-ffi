"""Shared Python handle lifecycle helpers."""

from __future__ import annotations

import warnings as _warnings
from collections.abc import Callable
from contextlib import suppress
from types import TracebackType
from typing import Any, Self


def warn_unclosed(
    handle_name: str,
    closed: bool,
    _warn: Callable[..., None] = _warnings.warn,
) -> None:
    """Report an owner that reached garbage collection without explicit close."""
    if closed:
        return
    _warn(
        f"{handle_name} was not explicitly closed",
        ResourceWarning,
        stacklevel=2,
    )


class ContextHandleMixin:
    """Provide context-manager behavior for explicit-close handles."""

    def close(self) -> object:
        """Release this handle.

        A handle whose release reports asynchronous teardown returns a future
        here. Leaving the ``with`` block discards it, so a host that must
        observe teardown calls ``close`` itself and waits.
        """
        raise NotImplementedError

    def __enter__(self) -> Self:
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc_value: BaseException | None,
        traceback: TracebackType | None,
    ) -> None:
        self.close()


class WarnUnclosedMixin:
    """Warn when a handle is garbage-collected while still open."""

    _handle_name = "Handle"

    @property
    def closed(self) -> bool:
        """Return whether this handle has been closed."""
        raise NotImplementedError

    def __del__(self) -> None:
        # Finalizers cannot report warning-delivery failures, including during
        # interpreter shutdown when Python globals may be partially torn down.
        with suppress(BaseException):
            warn_unclosed(self._handle_name, getattr(self, "closed", True))


class NativeHandleMixin(WarnUnclosedMixin, ContextHandleMixin):
    """Mixin for public handles backed by a private `_native` handle."""

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
        """Release the private native handle exactly once."""
        return self._native.close()

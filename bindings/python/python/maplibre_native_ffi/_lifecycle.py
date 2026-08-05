"""Shared Python handle lifecycle helpers."""

from __future__ import annotations

import warnings as _warnings
from collections.abc import Callable
from types import TracebackType
from typing import Any, Self


def warn_unclosed(
    handle_name: str,
    closed: bool,
    _warn: Callable[..., None] = _warnings.warn,
) -> None:
    """Report an unclosed handle without destroying thread-affine native state."""
    if closed:
        return
    _warn(
        f"{handle_name} was not closed; native state was intentionally leaked",
        ResourceWarning,
        stacklevel=2,
    )


class ContextHandleMixin:
    """Provide context-manager behavior for explicit-close handles."""

    def close(self) -> None:
        """Release this handle."""
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
        try:
            warn_unclosed(self._handle_name, getattr(self, "closed", True))
        except AttributeError, RuntimeError, Warning:
            # Finalizers may run during interpreter shutdown, with Python
            # globals half torn down or warnings promoted to exceptions.
            return


class NativeHandleMixin(WarnUnclosedMixin, ContextHandleMixin):
    """Mixin for public handles backed by a private `_native` handle."""

    _native: Any

    @property
    def closed(self) -> bool:
        """Return whether the private native handle has been closed."""
        return bool(self._native.closed)

    def close(self) -> None:
        """Release the private native handle exactly once."""
        self._native.close()

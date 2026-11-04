"""Lifecycle behavior that every public handle shares."""

from __future__ import annotations

import warnings
from contextlib import suppress
from types import TracebackType
from typing import Any, Self


class NativeHandleMixin:
    _handle_name = "Handle"
    _native: Any
    _parent: NativeHandleMixin | None

    @property
    def is_closed(self) -> bool:
        """Whether a release, dispose, discard, or ``with`` block exit has
        ended the handle."""
        return bool(self._native._is_closed)

    def __repr__(self) -> str:
        closed = "true" if self.is_closed else "false"
        return f"{type(self).__name__}(id={self._native.id:#x}, closed={closed})"

    def __del__(self) -> None:
        # Finalizers cannot report warning-delivery failures, including during
        # interpreter shutdown when Python globals may be partially torn down.
        with suppress(BaseException):
            native = getattr(self, "_native", None)
            # A callback's borrowed handle is its owner's to release.
            if native is not None and not native._is_closed and not native._is_borrowed:
                warnings.warn(
                    f"{self._handle_name} was not explicitly released",
                    ResourceWarning,
                    stacklevel=1,
                )


class ScopedHandleMixin(NativeHandleMixin):
    """The ``with`` block of a handle.

    Leaving the block disposes the handle, as ``dispose`` does, without
    waiting, whatever state the handle is in, such as a runtime whose map is
    still live. To wait until the release finishes, call ``release()`` and wait
    for its future instead. Leaving the block after a release or dispose does
    nothing.
    """

    def __enter__(self) -> Self:
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc_value: BaseException | None,
        traceback: TracebackType | None,
    ) -> None:
        native = self._native
        if not native._is_closed:
            native._dispose()

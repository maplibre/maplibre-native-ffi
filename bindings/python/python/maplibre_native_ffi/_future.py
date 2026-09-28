"""Helpers for adapting native completion futures."""

from __future__ import annotations

import logging
from collections import deque
from collections.abc import Callable
from concurrent.futures import Future, ThreadPoolExecutor
from threading import Lock

_callback_executor = ThreadPoolExecutor(thread_name_prefix="maplibre-callback")
_logger = logging.getLogger(__name__)


class NativeFuture[T](Future[T]):
    """A native result whose user callbacks run away from the native worker.

    Callbacks for one future run in registration order. Internal value conversion
    finishes inline, so a user callback can wait for another binding future.
    """

    def __init__(self) -> None:
        super().__init__()
        self._callback_lock = Lock()
        self._pending_callbacks: deque[Callable[[Future[T]], object]] = deque()
        self._dispatching = False

    def add_done_callback(self, fn: Callable[[Future[T]], object]) -> None:
        def dispatch(_: Future[T]) -> None:
            with self._callback_lock:
                self._pending_callbacks.append(fn)
                if self._dispatching:
                    return
                self._dispatching = True
            _callback_executor.submit(self._run_callbacks)

        super().add_done_callback(dispatch)

    def _run_callbacks(self) -> None:
        while True:
            with self._callback_lock:
                if not self._pending_callbacks:
                    self._dispatching = False
                    return
                callback = self._pending_callbacks.popleft()
            try:
                callback(self)
            except BaseException:
                _logger.exception("exception in native future callback")

    def _add_internal_callback(self, fn: Callable[[Future[T]], object]) -> None:
        super().add_done_callback(fn)


def map_future[T, U](
    source: Future[T],
    transform: Callable[[T], U],
    *,
    retained: object = None,
) -> Future[U]:
    """Return an eager future that transforms a native completion result.

    Native work is already running once its submission is accepted, so the
    derived future refuses ``cancel()`` for the same reason its source does,
    and it always reports the source's outcome.

    ``retained`` is kept alive until the source future is terminal, for a
    completion whose native side borrows a Python-owned handle.
    """
    result: Future[U] = NativeFuture()
    result.set_running_or_notify_cancel()

    def complete(completed: Future[T]) -> None:
        nonlocal retained
        try:
            value = transform(completed.result())
        except BaseException as error:  # noqa: BLE001 - preserve every terminal failure.
            result.set_exception(error)
        else:
            result.set_result(value)
        finally:
            # Future keeps its callbacks after completion; release the owner now.
            retained = None

    if isinstance(source, NativeFuture):
        source._add_internal_callback(complete)
    else:
        source.add_done_callback(complete)
    return result

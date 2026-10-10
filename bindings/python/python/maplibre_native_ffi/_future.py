"""Helpers for adapting native completion futures."""

from __future__ import annotations

import logging
import traceback
import weakref
from collections import deque
from collections.abc import Callable
from concurrent.futures import Future, ThreadPoolExecutor
from concurrent.futures._base import CANCELLED
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

    def cancel(self) -> bool:
        """Abandon the wait, and wake every waiter at once.

        Native work continues to its terminal disposition. A plain future
        counts as done for ``wait()`` and ``as_completed()`` only once its
        runner claims it, which native work may not reach for a long time, so
        a successful cancellation notifies them now.
        """
        if not super().cancel():
            return False
        with self._condition:
            if self._state == CANCELLED:
                self.set_running_or_notify_cancel()
        return True

    def _claim(self) -> bool:
        """Claim the future for a result, or return False once it is cancelled.

        A cancelled future takes no result, so the caller disposes any value
        that the result transfers.
        """
        with self._condition:
            if self.cancelled():
                return False
            return self.set_running_or_notify_cancel()


def map_future[T, U](
    source: Future[T],
    transform: Callable[[T], U],
    discard: Callable[[T], object] | None = None,
) -> Future[U]:
    """Return a future that transforms a native completion result.

    ``cancel()`` on the derived future abandons the wait and cancels its
    source, and native work continues to its terminal disposition. When the
    source resolves after the derived future was cancelled, ``discard``
    releases the value that nothing adopts.
    """
    result: NativeFuture[U] = NativeFuture()
    # A weak reference keeps the derived future from holding its source, and
    # the source's value with it, after both resolve.
    source_ref = weakref.ref(source)

    def complete(completed: Future[T]) -> None:
        if not result._claim():
            # A cancelled source released its value natively, and a failed
            # one carries none.
            if (
                discard is not None
                and not completed.cancelled()
                and completed.exception() is None
            ):
                discard(completed.result())
            return
        try:
            try:
                raw = completed.result()
            except BaseException as error:  # noqa: BLE001 - preserve every terminal failure.
                result.set_exception(error)
            else:
                result.set_result(transform(raw))
        except BaseException as error:  # noqa: BLE001 - preserve every terminal failure.
            # A traceback keeps its frames alive, and they hold the value that
            # failed to convert. Carrying the trace as text instead disposes an
            # owned native value now rather than when this future is collected.
            trace = error.__traceback__
            error.__traceback__ = None
            error.add_note("".join(traceback.format_tb(trace)).rstrip())
            del trace
            result.set_exception(error)

    def propagate_cancel(derived: Future[U]) -> None:
        if derived.cancelled() and (pending := source_ref()) is not None:
            pending.cancel()

    result._add_internal_callback(propagate_cancel)
    if isinstance(source, NativeFuture):
        source._add_internal_callback(complete)
    else:
        source.add_done_callback(complete)
    return result

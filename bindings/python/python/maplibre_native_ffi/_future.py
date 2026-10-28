"""Helpers for adapting native completion futures."""

from __future__ import annotations

import logging
import traceback
import weakref
from collections import deque
from collections.abc import Callable
from concurrent.futures import Future, ThreadPoolExecutor
from threading import Lock

_callback_executor = ThreadPoolExecutor(thread_name_prefix="maplibre-callback")
_logger = logging.getLogger(__name__)


class NativeFuture[T](Future[T]):
    """A native result that the binding's dispatcher thread resolves.

    Done callbacks for one future run in registration order on host worker
    threads, so a callback can wait for another binding future. Waiting for a
    pending future on the dispatcher thread, from a registered callback, raises
    ``RuntimeError``, since its completion runs on that thread only after the
    callback returns.
    """

    def __init__(self) -> None:
        super().__init__()
        self._callback_lock = Lock()
        self._pending_callbacks: deque[Callable[[Future[T]], object]] = deque()
        self._dispatching = False
        # Whether set_running_or_notify_cancel has run, which it does once:
        # for the claim, or to wake waiters after a cancel.
        self._settle_lock = Lock()
        self._settled = False

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

    def result(self, timeout: float | None = None) -> T:
        self._check_wait()
        return super().result(timeout)

    def exception(self, timeout: float | None = None) -> BaseException | None:
        self._check_wait()
        return super().exception(timeout)

    def _check_wait(self) -> None:
        from . import _native

        if not self.done() and _native._on_dispatcher_thread():
            raise RuntimeError(
                "a callback cannot wait for a pending binding future, which "
                "resolves on the dispatcher thread after the callback returns"
            )

    def _add_internal_callback(self, fn: Callable[[Future[T]], object]) -> None:
        super().add_done_callback(fn)

    def cancel(self) -> bool:
        """Abandon the wait, and wake every waiter at once.

        Native work continues to its terminal disposition.
        """
        if not super().cancel():
            return False
        self._settle()
        return True

    def _claim(self) -> bool:
        """Claim the future for a result, or return False once it is cancelled."""
        return self._settle()

    def _settle(self) -> bool:
        # A pending future becomes running and takes the result. A cancelled
        # one notifies the waiters that ``Future.cancel`` leaves to an executor.
        with self._settle_lock:
            if self._settled:
                return False
            self._settled = True
            return self.set_running_or_notify_cancel()


def map_future[T, U](
    source: Future[T],
    transform: Callable[[T], U],
) -> Future[U]:
    """Return a future that transforms a native completion result.

    ``cancel()`` on the derived future abandons the wait and cancels its
    source, and native work continues to its terminal disposition.
    """
    result: NativeFuture[U] = NativeFuture()
    # A weak reference keeps the derived future from holding its source, and
    # the source's value with it, after both resolve.
    source_ref = weakref.ref(source)

    def complete(completed: Future[T]) -> None:
        if not result._claim():
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

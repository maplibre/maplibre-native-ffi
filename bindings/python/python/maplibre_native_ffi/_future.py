"""Helpers for adapting native completion futures."""

from __future__ import annotations

from collections.abc import Callable
from concurrent.futures import Future


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
    result: Future[U] = Future()
    result.set_running_or_notify_cancel()

    def complete(completed: Future[T]) -> None:
        try:
            result.set_result(transform(completed.result()))
        except Exception as error:  # noqa: BLE001 - preserve the source failure.
            result.set_exception(error)
        finally:
            # Reading the capture here is what keeps the retained owner alive
            # until this callback runs.
            _ = retained

    source.add_done_callback(complete)
    return result

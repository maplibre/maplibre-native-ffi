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

    source.add_done_callback(complete)
    return result

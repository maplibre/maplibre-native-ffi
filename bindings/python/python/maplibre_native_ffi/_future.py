"""Helpers for adapting native completion futures."""

from __future__ import annotations

from collections.abc import Callable
from concurrent.futures import Future


def map_future[T, U](source: Future[T], transform: Callable[[T], U]) -> Future[U]:
    """Return an eager future that transforms a native completion result."""
    result: Future[U] = Future()

    def complete(completed: Future[T]) -> None:
        if result.cancelled():
            return
        try:
            result.set_result(transform(completed.result()))
        except Exception as error:  # noqa: BLE001 - preserve the source failure.
            result.set_exception(error)

    source.add_done_callback(complete)
    return result


def retain_future[T](source: Future[T], retained: object) -> Future[T]:
    """Keep an owner alive until a source future becomes terminal."""
    result: Future[T] = Future()

    def complete(completed: Future[T]) -> None:
        try:
            if result.cancelled():
                return
            result.set_result(completed.result())
        except Exception as error:  # noqa: BLE001 - preserve the source failure.
            if not result.cancelled():
                result.set_exception(error)
        finally:
            _ = retained

    source.add_done_callback(complete)
    return result

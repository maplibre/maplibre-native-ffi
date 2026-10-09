import asyncio
import gc
import weakref
from concurrent.futures import Future
from threading import Event

import pytest
from maplibre_native_ffi._future import NativeFuture, map_future
from support import TIMEOUT


def test_derived_future_reports_the_transformed_source_result() -> None:
    source: Future[int] = Future()
    result = map_future(source, lambda value: value * 2)

    source.set_result(21)

    assert result.result(timeout=TIMEOUT) == 42


@pytest.mark.parametrize(
    "failure", [RuntimeError("native completion failed"), asyncio.CancelledError()]
)
def test_derived_future_reports_a_source_failure(failure: BaseException) -> None:
    source: Future[int] = Future()
    result = map_future(source, lambda value: value)

    source.set_exception(failure)

    with pytest.raises(type(failure)) as raised:
        result.result(timeout=TIMEOUT)
    assert raised.value is failure


@pytest.mark.parametrize("failure", [ZeroDivisionError(), asyncio.CancelledError()])
def test_derived_future_reports_a_transform_failure(failure: BaseException) -> None:
    source: Future[int] = Future()

    def transform(value: int) -> int:
        raise failure

    result = map_future(source, transform)

    source.set_result(0)

    with pytest.raises(type(failure)) as raised:
        result.result(timeout=TIMEOUT)
    assert raised.value is failure


def test_accepted_derived_future_refuses_cancellation() -> None:
    source: Future[int] = Future()
    result = map_future(source, lambda value: value)

    assert result.cancel() is False
    source.set_result(42)

    assert result.result(timeout=TIMEOUT) == 42


def test_downstream_callback_failure_preserves_the_completed_result(caplog) -> None:
    source: Future[int] = Future()
    result = map_future(source, lambda value: value * 2)
    failure = KeyboardInterrupt()

    def callback(completed: Future[int]) -> None:
        raise failure

    finished = Event()
    result.add_done_callback(callback)
    result.add_done_callback(lambda _: finished.set())
    source.set_result(21)

    assert finished.wait(TIMEOUT)
    assert result.result(timeout=TIMEOUT) == 42
    assert "exception in native future callback" in caplog.text


def test_callbacks_run_once_in_order_and_a_failed_conversion_releases_its_value() -> (
    None
):
    class Raw:
        pass

    raw = Raw()
    released = weakref.ref(raw)
    source: NativeFuture[Raw] = NativeFuture()
    source.set_running_or_notify_cancel()

    def convert(value: Raw) -> int:
        raise ValueError("conversion failed")

    derived = map_future(source, convert)
    calls: list[int] = []
    finished = Event()
    derived.add_done_callback(lambda _: calls.append(1))
    derived.add_done_callback(lambda _: calls.append(2))
    source.set_result(raw)
    derived.add_done_callback(lambda _: (calls.append(3), finished.set()))
    del raw, source

    assert finished.wait(TIMEOUT)
    assert calls == [1, 2, 3]
    with pytest.raises(ValueError, match="conversion failed") as raised:
        derived.result(timeout=0)
    # The trace survives as text, without the frames that held the value.
    assert "convert" in "\n".join(raised.value.__notes__)
    gc.collect()
    assert released() is None

import asyncio
import weakref
from concurrent.futures import Future

import pytest
from maplibre_native_ffi._future import map_future


def test_derived_future_reports_the_transformed_source_result() -> None:
    source: Future[int] = Future()
    result = map_future(source, lambda value: value * 2)

    source.set_result(21)

    assert result.result(timeout=5) == 42


@pytest.mark.parametrize(
    "failure", [RuntimeError("native completion failed"), asyncio.CancelledError()]
)
def test_derived_future_reports_a_source_failure(failure: BaseException) -> None:
    source: Future[int] = Future()
    result = map_future(source, lambda value: value)

    source.set_exception(failure)

    with pytest.raises(type(failure)) as raised:
        result.result(timeout=5)
    assert raised.value is failure


@pytest.mark.parametrize("failure", [ZeroDivisionError(), asyncio.CancelledError()])
def test_derived_future_reports_a_transform_failure(failure: BaseException) -> None:
    source: Future[int] = Future()

    def transform(value: int) -> int:
        raise failure

    result = map_future(source, transform)

    source.set_result(0)

    with pytest.raises(type(failure)) as raised:
        result.result(timeout=5)
    assert raised.value is failure


def test_accepted_derived_future_refuses_cancellation() -> None:
    source: Future[int] = Future()
    result = map_future(source, lambda value: value)

    assert result.cancel() is False
    source.set_result(42)

    assert result.result(timeout=5) == 42


def test_retained_owner_lives_until_the_source_is_terminal() -> None:
    class Owner:
        pass

    source: Future[int] = Future()
    owner = Owner()
    alive = weakref.ref(owner)
    result = map_future(source, lambda value: value, retained=owner)
    del owner

    assert alive() is not None

    source.set_result(7)
    assert result.result(timeout=5) == 7
    assert alive() is None


def test_downstream_callback_failure_preserves_the_completed_result() -> None:
    source: Future[int] = Future()
    result = map_future(source, lambda value: value * 2)
    failure = KeyboardInterrupt()

    def callback(completed: Future[int]) -> None:
        raise failure

    result.add_done_callback(callback)
    with pytest.raises(KeyboardInterrupt) as raised:
        source.set_result(21)

    assert raised.value is failure
    assert result.result(timeout=5) == 42

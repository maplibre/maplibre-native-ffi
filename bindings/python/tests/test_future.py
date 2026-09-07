import weakref
from concurrent.futures import Future

import pytest
from maplibre_native_ffi._future import map_future


def test_derived_future_reports_the_transformed_source_result() -> None:
    source: Future[int] = Future()
    result = map_future(source, lambda value: value * 2)

    source.set_result(21)

    assert result.result(timeout=5) == 42


def test_derived_future_reports_a_source_failure() -> None:
    source: Future[int] = Future()
    result = map_future(source, lambda value: value)
    failure = RuntimeError("native completion failed")

    source.set_exception(failure)

    with pytest.raises(RuntimeError) as raised:
        result.result(timeout=5)
    assert raised.value is failure


def test_derived_future_reports_a_transform_failure() -> None:
    source: Future[int] = Future()
    result = map_future(source, lambda value: 1 // value)

    source.set_result(0)

    with pytest.raises(ZeroDivisionError):
        result.result(timeout=5)


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

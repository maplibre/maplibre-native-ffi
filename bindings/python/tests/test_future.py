from concurrent.futures import Future

from maplibre_native_ffi._future import retain_future


def test_cancelled_retained_future_ignores_later_native_completion() -> None:
    source: Future[int] = Future()
    result = retain_future(source, object())

    assert result.cancel()
    source.set_result(42)

    assert result.cancelled()

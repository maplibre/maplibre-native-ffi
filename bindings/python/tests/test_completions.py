"""Native completions delivered as concurrent.futures futures."""

import asyncio
import gc
import threading
from concurrent.futures import CancelledError, Future, wait

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi._future import NativeFuture
from maplibre_native_ffi._operation import _adopt_future
from support import EMPTY_STYLE, TIMEOUT, Harness, leak_reports, result


def _live_native_futures() -> int:
    # Any binding call applies the releases native made without the GIL.
    mln.network_status_get()
    gc.collect()
    return sum(isinstance(value, NativeFuture) for value in gc.get_objects())


def test_a_rejected_submission_frees_its_future(map_handle: mln.MapHandle) -> None:
    before = _live_native_futures()
    for _ in range(3):
        with pytest.raises(mln.InvalidArgumentError) as raised:
            map_handle.get_feature_state(
                mln.FeatureStateSelector(source_id="source", state_key="hover")
            )
        # Native rejected the submission, after the binding made its future.
        assert raised.value.native_status_code is not None

    assert _live_native_futures() == before


def test_a_dropped_creation_result_retires_the_map_it_created(
    harness: Harness,
) -> None:
    creation = harness.runtime.map_create()
    result(creation)
    # The barrier's call releases native's reference to the creation future,
    # so the result below holds the last reference to the map.
    result(harness.runtime.barrier())

    with leak_reports() as reports:
        del creation
        gc.collect()
    assert reports == ["MapHandle was not explicitly closed"]
    # Runtime retirement waits for every map, so this resolves only once the
    # abandoned map has retired.
    result(harness.runtime.close())


def test_a_done_callback_runs_off_the_native_thread_and_can_wait_on_native(
    map_handle: mln.MapHandle,
) -> None:
    finished = threading.Event()
    outcomes: list[tuple[str, mln.CameraQueryResult, mln.CameraQueryResult]] = []

    def query_again(first: Future[mln.CameraQueryResult]) -> None:
        try:
            second = result(map_handle.camera_query())
            outcomes.append((threading.current_thread().name, first.result(), second))
        finally:
            finished.set()

    map_handle.camera_query().add_done_callback(query_again)

    assert finished.wait(TIMEOUT)
    ((thread, first, second),) = outcomes
    assert thread.startswith("maplibre-callback")
    assert second.generation > first.generation


def test_a_failed_command_resolves_with_its_status(map_handle: mln.MapHandle) -> None:
    result(map_handle.set_style_json(EMPTY_STYLE))

    completion = result(map_handle.set_style_source_volatile("missing", True))

    assert completion.disposition == mln.CommandDisposition.FAILED
    assert completion.native_status_code == mln.Status.NOT_FOUND.native_code
    assert completion.diagnostic


def test_cancelling_a_wait_abandons_it_and_releases_the_late_map(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    handles: list[mln.ResourceRequestHandle] = []
    provided = threading.Event()
    running = threading.Event()
    release = threading.Event()

    def hold(request: mln.ResourceRequest, handle: mln.ResourceRequestHandle) -> None:
        handles.append(handle)
        provided.set()

    def block_retirement() -> None:
        running.set()
        release.wait(TIMEOUT)

    harness.routes["custom://pending.json"] = hold
    map_handle.set_style_url("custom://pending.json")
    assert provided.wait(TIMEOUT)
    handle = handles.pop()
    handle.set_cancel_callback(block_retirement)

    # Map retirement runs the cancel callback, and the runtime's ordered
    # submissions hold every later creation behind it.
    closing = map_handle.close()
    assert running.wait(TIMEOUT)
    created = harness.runtime.map_create()
    awaited = harness.runtime.map_create()
    with pytest.raises(TimeoutError):
        created.result(timeout=0)
    assert created.cancel() is True
    with pytest.raises(CancelledError):
        created.result(timeout=0)
    # A waiter wakes at once, though native has not reached the creation.
    assert wait([created], timeout=0).done == {created}

    async def wait_briefly() -> None:
        # The timeout cancels the asyncio wait, which cancels its future.
        await asyncio.wait_for(asyncio.wrap_future(awaited), timeout=0)

    with pytest.raises(TimeoutError):
        asyncio.run(wait_briefly())
    assert awaited.cancelled()

    with leak_reports() as reports:
        release.set()
        assert result(closing) is None
        handle.close()
        # The barrier resolves once both creations have delivered their maps
        # to the cancelled futures. Runtime retirement waits for every map,
        # so the close resolves only once both late maps have retired.
        result(harness.runtime.barrier())
        assert result(harness.runtime.close()) is None
        gc.collect()
    assert reports == []


def test_a_map_that_arrives_after_its_claimed_wait_is_cancelled_is_disposed(
    harness: Harness,
) -> None:
    raw = result(harness.runtime._native.map_create())
    # The native completion has claimed the source, so cancelling the public
    # future leaves the map to arrive with nothing to adopt it.
    source: NativeFuture[object] = NativeFuture()
    assert source._claim()
    created = _adopt_future(source, "MapHandle", harness.runtime)
    assert created.cancel() is True

    with leak_reports() as reports:
        source.set_result(raw)
        assert raw.closed
        # Runtime retirement waits for every map, so this resolves only once
        # the discarded map has retired.
        assert result(harness.runtime.close()) is None
        gc.collect()
    assert reports == []

"""Native completions delivered as concurrent.futures futures."""

import asyncio
import gc
import threading
import weakref
from concurrent.futures import CancelledError, Future, wait

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import _future
from maplibre_native_ffi._future import NativeFuture
from maplibre_native_ffi._operation import _adopt_future
from support import EMPTY_STYLE, TIMEOUT, Harness, leak_reports, result


def test_discarded_commands_are_fenced_and_sync_rejection_raises(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    assert (
        map_handle.discarding.set_debug_options(mln.MapDebugOption.TILE_BORDERS) is None
    )
    # A later command on the same map completes after the discarded one.
    result(map_handle.request_repaint())
    assert map_handle.get_snapshot().debug_options == mln.MapDebugOption.TILE_BORDERS
    with pytest.raises(
        mln.MaplibreError,
        check=lambda error: error.status == mln.Status.INVALID_ARGUMENT,
    ):
        map_handle.discarding.resize(
            mln.LogicalExtent(width=0, height=8, scale_factor=1)
        )


def test_a_rejected_submission_frees_its_future(
    map_handle: mln.MapHandle, monkeypatch: pytest.MonkeyPatch
) -> None:
    made: list[weakref.ref[NativeFuture[object]]] = []

    class TrackedFuture(NativeFuture[object]):
        def __init__(self) -> None:
            super().__init__()
            made.append(weakref.ref(self))

    # The binding looks the class up for each submission.
    monkeypatch.setattr(_future, "NativeFuture", TrackedFuture)
    with pytest.raises(mln.MaplibreError) as raised:
        map_handle.get_feature_state(
            mln.FeatureStateSelector(source_id="source", state_key="hover")
        )
    # Native rejected the submission, after the binding made its future.
    assert raised.value.status == mln.Status.INVALID_ARGUMENT
    assert len(made) == 1

    # Any binding call applies the releases native made without the GIL.
    mln.network_get_status()
    gc.collect()
    assert made[0]() is None


def test_a_dropped_creation_result_retires_the_map_it_created(
    harness: Harness,
) -> None:
    creation = harness.runtime.create_map()
    result(creation)
    # A later runtime call releases native's reference to the creation future,
    # so the result below holds the last reference to the map.
    result(harness.runtime.clear_resource_transform())

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
    outcomes: list[tuple[mln.CameraQueryResult, mln.CameraQueryResult]] = []

    def query_again(first: Future[mln.CameraQueryResult]) -> None:
        try:
            second = result(map_handle.get_camera())
            outcomes.append((first.result(), second))
        finally:
            finished.set()

    map_handle.get_camera().add_done_callback(query_again)

    assert finished.wait(TIMEOUT)
    ((first, second),) = outcomes
    assert second.generation > first.generation


def test_a_failed_command_resolves_with_its_status(map_handle: mln.MapHandle) -> None:
    result(map_handle.set_style_json(EMPTY_STYLE))

    completion = result(map_handle.set_style_source_volatile("missing", True))

    assert completion.disposition == mln.CommandDisposition.FAILED
    assert completion.native_status_code == int(mln.Status.NOT_FOUND)
    assert completion.diagnostic


def test_cancelling_a_wait_abandons_it_and_releases_the_late_map(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    handles: list[mln.ResourceRequestHandle] = []
    provided = threading.Event()
    running = threading.Event()
    release = threading.Event()

    def hold(
        request: mln.ResourceRequestInfo, handle: mln.ResourceRequestHandle
    ) -> None:
        handles.append(handle)
        provided.set()

    def block_retirement() -> None:
        running.set()
        release.wait(TIMEOUT)

    harness.routes["custom://pending.json"] = hold
    map_handle.set_style_url("custom://pending.json")
    assert provided.wait(TIMEOUT)
    handle = handles.pop()
    handle.set_cancel_callback(mln.ResourceRequestCancelHandler(block_retirement))

    # Map retirement runs the cancel callback, and the runtime's ordered
    # submissions hold every later creation behind it.
    closing = map_handle.close()
    assert running.wait(TIMEOUT)
    created = harness.runtime.create_map()
    awaited = harness.runtime.create_map()
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
        # A later runtime command resolves once both creations have delivered
        # their maps to the cancelled futures. Runtime retirement waits for
        # every map, so the close resolves only once both late maps have
        # retired.
        result(harness.runtime.clear_resource_transform())
        assert result(harness.runtime.close()) is None
        gc.collect()
    assert reports == []


def test_a_map_that_arrives_after_its_claimed_wait_is_cancelled_is_disposed(
    harness: Harness,
) -> None:
    raw = result(harness.runtime._native.create_map())
    # The native completion has claimed the source, so cancelling the public
    # future leaves the map to arrive with nothing to adopt it.
    source: NativeFuture[object] = NativeFuture()
    assert source._claim()
    created = _adopt_future(source, "MapHandle", harness.runtime)
    assert created.cancel() is True

    with leak_reports() as reports:
        source.set_result(raw)
        with pytest.raises(
            mln.MaplibreError,
            check=lambda error: error.status == mln.Status.INVALID_STATE,
        ):
            raw.get_snapshot()
        # Runtime retirement waits for every map, so this resolves only once
        # the discarded map has retired.
        assert result(harness.runtime.close()) is None
        gc.collect()
    assert reports == []

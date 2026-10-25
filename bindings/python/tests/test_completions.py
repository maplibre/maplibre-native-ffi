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
from support import EMPTY_STYLE, TIMEOUT, Harness, leak_reports, result


def test_discarded_commands_are_fenced_and_sync_rejection_raises(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    assert (
        map_handle.discarding.set_debug_options(mln.MapDebugFlag.TILE_BORDERS) is None
    )
    # A later command on the same map completes after the discarded one.
    result(map_handle.request_repaint())
    assert map_handle.get_snapshot().debug_options == mln.MapDebugFlag.TILE_BORDERS
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


def test_a_failed_command_raises_its_status_and_generation(
    map_handle: mln.MapHandle,
) -> None:
    committed = result(map_handle.set_style_json(EMPTY_STYLE))

    with pytest.raises(mln.MaplibreError) as raised:
        result(map_handle.set_style_source_volatile("missing", True))

    assert raised.value.status == mln.Status.NOT_FOUND
    assert raised.value.diagnostic
    assert raised.value.generation > committed.generation


def test_cancelling_a_wait_abandons_it_and_leaves_the_map_to_its_owner(
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
    ready = created.ready
    with pytest.raises(TimeoutError):
        ready.result(timeout=0)
    assert ready.cancel() is True
    with pytest.raises(CancelledError):
        ready.result(timeout=0)
    # A waiter wakes at once, though native has not reached the creation.
    assert wait([ready], timeout=0).done == {ready}

    async def wait_briefly() -> None:
        # The timeout cancels the asyncio wait, which cancels its future.
        await asyncio.wait_for(asyncio.wrap_future(awaited.ready), timeout=0)

    with pytest.raises(TimeoutError):
        asyncio.run(wait_briefly())
    assert awaited.ready.cancelled()

    with leak_reports() as reports:
        release.set()
        assert result(closing) is None
        handle.close()
        # Native work continues past the abandoned waits. A later runtime
        # command resolves once both creations have finished, and the maps
        # stay with their owner, which closes them.
        result(harness.runtime.clear_resource_transform())
        for owner in (created.map, awaited.map):
            assert owner.get_snapshot() is not None
            assert result(owner.close()) is None
        assert result(harness.runtime.close()) is None
        gc.collect()
    assert reports == []

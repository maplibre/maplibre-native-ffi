"""Native completions delivered as concurrent.futures futures."""

import asyncio
import threading
from concurrent.futures import CancelledError, Future, wait

import maplibre_native_ffi as mln
import pytest
from support import (
    EMPTY_STYLE,
    TIMEOUT,
    Harness,
    denied_response,
    map_options,
    result,
)


def test_discarded_commands_are_fenced_and_sync_rejection_raises(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    assert map_handle.discarding.set_debug_flags(mln.MapDebugFlag.TILE_BORDERS) is None
    # A later command on the same map completes after the discarded one.
    result(map_handle.request_repaint())
    assert map_handle.get_snapshot().debug_flags == mln.MapDebugFlag.TILE_BORDERS
    with pytest.raises(
        mln.MaplibreError,
        check=lambda error: error.status == mln.Status.INVALID_ARGUMENT,
    ):
        map_handle.discarding.resize(
            mln.LogicalExtent(width=0, height=8, scale_factor=1.0)
        )


def test_a_done_callback_runs_off_the_dispatcher_thread_and_can_wait_on_native(
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
    # The second query ran after the first, so it matches the same snapshot or
    # a later one.
    assert second.generation >= first.generation


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
    provided = threading.Event()
    release = threading.Event()

    def hold(
        request: mln.ResourceRequestInfo, handle: mln.ResourceRequestHandle
    ) -> None:
        provided.set()
        release.wait(TIMEOUT)
        handle.complete(denied_response())

    # The provider holds the dispatcher thread, so no completion resolves
    # until it returns.
    harness.routes["custom://pending.json"] = hold
    map_handle.set_style_url("custom://pending.json")
    assert provided.wait(TIMEOUT)
    created = harness.runtime.create_map(map_options())
    awaited = harness.runtime.create_map(map_options())
    ready = created.ready
    with pytest.raises(TimeoutError):
        ready.result(timeout=0)
    assert ready.cancel() is True
    with pytest.raises(CancelledError):
        ready.result(timeout=0)
    # A waiter wakes at once, though the creation has not resolved.
    assert wait([ready], timeout=0).done == {ready}

    async def wait_briefly() -> None:
        # The timeout cancels the asyncio wait, which cancels its future.
        await asyncio.wait_for(asyncio.wrap_future(awaited.ready), timeout=0)

    with pytest.raises(TimeoutError):
        asyncio.run(wait_briefly())
    assert awaited.ready.cancelled()

    release.set()
    # Native work continues past the abandoned waits. A later runtime command
    # resolves once both creations have finished, and the maps stay with
    # their owner, which releases them.
    result(harness.runtime.clear_resource_transform())
    for owner in (created, awaited, map_handle):
        assert owner.get_snapshot() is not None
        assert result(owner.release()) is None
    assert result(harness.runtime.release()) is None

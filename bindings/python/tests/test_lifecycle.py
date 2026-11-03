"""Handle lifetimes: close, dispose, refusal, parents, and shutdown."""

import gc
import subprocess
import sys
import threading
import weakref

import maplibre_native_ffi as mln
import pytest
from support import EMPTY_STYLE, TIMEOUT, Harness, ready_map, result


def test_a_closed_handle_refuses_use_and_a_synchronous_close_repeats_safely() -> None:
    with pytest.raises(TypeError):
        mln.MapHandle()

    with mln.RuntimeHandle.create() as runtime:
        with ready_map(runtime) as map_handle:
            assert map_handle.get_snapshot() is not None
        with pytest.raises(
            mln.MaplibreError,
            check=lambda error: error.status == mln.Status.INVALID_STATE,
        ):
            result(map_handle.close())
        with pytest.raises(
            mln.MaplibreError,
            check=lambda error: error.status == mln.Status.INVALID_STATE,
        ):
            map_handle.get_snapshot()
    with pytest.raises(
        mln.MaplibreError, check=lambda error: error.status == mln.Status.INVALID_STATE
    ):
        runtime.close()

    data = mln.GeoJsonSourceDataHandle.create(
        {"type": "FeatureCollection", "features": []}
    )
    _check_handle(data, closed=False)
    data.close()
    data.close()
    with data:
        pass
    _check_handle(data, closed=True)


def _check_handle(
    handle: mln.GeoJsonSourceDataHandle | mln.MapHandle | mln.RuntimeHandle,
    *,
    closed: bool,
) -> None:
    assert handle.id != 0
    assert handle.is_closed is closed
    name = type(handle).__name__
    assert repr(handle) == f"{name}(id={handle.id:#x}, closed={str(closed).lower()})"


def test_dispose_closes_a_handle_once() -> None:
    runtime = mln.RuntimeHandle.create()
    map_handle = ready_map(runtime)
    for handle in (map_handle, runtime):
        _check_handle(handle, closed=False)
        handle.dispose()
        handle.dispose()
        with handle:
            pass
        _check_handle(handle, closed=True)
    # An asynchronous close after dispose reaches native, which reports the
    # handle stale.
    with pytest.raises(
        mln.MaplibreError, check=lambda error: error.status == mln.Status.INVALID_STATE
    ):
        map_handle.close()


def test_leaving_a_block_after_an_explicit_close_does_nothing() -> None:
    with mln.RuntimeHandle.create() as runtime:
        with ready_map(runtime) as map_handle:
            result(map_handle.close())
        result(runtime.close())


def test_leaving_a_root_block_disposes_it_without_waiting_for_its_children() -> None:
    with mln.RuntimeHandle.create() as runtime:
        # Release refuses a runtime with a live map, and dispose accepts it.
        map_handle = ready_map(runtime)
    _check_handle(runtime, closed=True)
    _check_handle(map_handle, closed=False)
    result(map_handle.close())


def test_a_handle_id_names_its_events_and_outlives_close(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    result(map_handle.set_style_json(EMPTY_STYLE))
    event = harness.wait_event(mln.RuntimeEventType.MAP_STYLE_LOADED)
    assert event.source == map_handle.id
    result(map_handle.close())
    # A host routes events that it drains after close by the id it kept.
    assert map_handle.id == event.source


def test_a_refused_close_leaves_the_handle_usable(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    with pytest.raises(mln.MaplibreError) as raised:
        harness.runtime.close()
    assert raised.value.status == mln.Status.INVALID_STATE

    result(harness.runtime.clear_resource_transform())
    result(map_handle.close())
    result(harness.runtime.close())
    with pytest.raises(
        mln.MaplibreError, check=lambda error: error.status == mln.Status.INVALID_STATE
    ):
        harness.runtime.discarding.clear_resource_transform()


def test_a_child_keeps_its_parent_alive() -> None:
    loaded = threading.Event()

    def receive_events(events: tuple[mln.RuntimeEvent, ...]) -> None:
        if any(event.type == mln.RuntimeEventType.MAP_STYLE_LOADED for event in events):
            loaded.set()

    runtime = mln.RuntimeHandle.create()
    runtime.discarding.set_event_handler(mln.RuntimeEventHandler(receive_events))
    # Kept only to close the runtime after its map.
    parent = weakref.ref(runtime)
    map_handle = ready_map(runtime)
    del runtime, receive_events
    gc.collect()

    # The map still works, and so does the event handler its runtime holds.
    result(map_handle.set_style_json(EMPTY_STYLE))
    assert loaded.wait(TIMEOUT)
    result(map_handle.close())
    runtime = parent()
    assert runtime is not None
    result(runtime.close())


_SHUTDOWN_WITH_LIVE_HANDLES = """
import maplibre_native_ffi as m

runtime = m.RuntimeHandle.create()
runtime.set_resource_provider(
    m.ResourceProvider(callback=lambda request, handle: None)
).result(10)
m.set_log_callback(m.LogHandler(lambda *_: None))
options = m.MapOptions(initial_extent=m.LogicalExtent(256, 256, 1.0))
live = runtime.create_map(options).map
live.set_style_url("custom://never-answered.json")
"""


@pytest.mark.skipif(
    sys.platform in ("android", "ios"),
    reason="an embedded interpreter has no executable to start",
)
def test_interpreter_shutdown_with_live_handles_and_callbacks_exits_cleanly() -> None:
    completed = subprocess.run(
        [sys.executable, "-c", _SHUTDOWN_WITH_LIVE_HANDLES],
        check=False,
        capture_output=True,
        text=True,
        timeout=3 * TIMEOUT,
    )

    assert completed.returncode == 0, completed.stderr
    assert "Exception ignored" not in completed.stderr
    assert "Fatal Python error" not in completed.stderr
    assert "panicked" not in completed.stderr

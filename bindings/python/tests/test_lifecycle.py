"""Handle lifetimes: close, refusal, abandonment, parents, GC, and shutdown."""

import gc
import subprocess
import sys
import threading
import weakref

import maplibre_native_ffi as mln
import pytest
from support import EMPTY_STYLE, TIMEOUT, Harness, leak_reports, result


def test_closing_twice_is_safe_and_a_closed_handle_refuses_use() -> None:
    with pytest.raises(TypeError):
        mln.MapHandle()

    with mln.runtime_create() as runtime:
        with result(runtime.map_create()) as map_handle:
            issued = map_handle.id
            assert not map_handle.closed
        assert map_handle.closed
        assert result(map_handle.close()) is None
        # Events drained after close still name the handle that sent them.
        assert map_handle.id == issued
        with pytest.raises(mln.InvalidStateError, match="closed"):
            map_handle.snapshot_get()
    assert runtime.closed
    runtime.close()

    data = mln.geojson_source_data_create(b'{"type":"FeatureCollection","features":[]}')
    data.close()
    data.close()
    assert data.closed


def test_a_refused_close_leaves_the_handle_usable(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    with pytest.raises(mln.InvalidStateError) as raised:
        harness.runtime.close()
    assert raised.value.native_status_code == mln.Status.INVALID_STATE.native_code

    assert not harness.runtime.closed
    assert result(harness.runtime.barrier()) is None
    result(map_handle.close())
    result(harness.runtime.close())
    assert harness.runtime.closed


def test_a_handle_abandoned_on_a_callback_stack_is_reported_and_disposed(
    harness: Harness,
) -> None:
    handles: list[mln.ResourceRequestHandle] = []
    submitted = threading.Event()
    cancelled = threading.Event()
    owners = {"map": result(harness.runtime.map_create())}

    def abandon_map(
        request: mln.ResourceRequest, handle: mln.ResourceRequestHandle
    ) -> None:
        handles.append(handle)
        handle.set_cancel_callback(cancelled.set)
        # The last reference goes on the provider's stack, where the map
        # cannot be disposed without reentering native.
        assert submitted.wait(TIMEOUT)
        owners.clear()

    harness.routes["custom://abandoned.json"] = abandon_map
    with leak_reports() as reports:
        owners["map"].set_style_url("custom://abandoned.json")
        submitted.set()
        # Disposing the map discards its pending style request.
        assert cancelled.wait(TIMEOUT)

    assert reports == ["MapHandle was not explicitly closed"]
    handles[0].close()
    result(harness.runtime.close())


def test_a_child_keeps_its_parent_alive() -> None:
    runtime = mln.runtime_create()
    parent = weakref.ref(runtime)
    map_handle = result(runtime.map_create())
    del runtime
    gc.collect()

    runtime = parent()
    assert runtime is not None
    assert result(map_handle.camera_query()).generation > 0
    result(map_handle.close())
    result(runtime.close())


def test_cyclic_gc_reclaims_a_request_and_the_callback_that_closes_it(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    handles: list[mln.ResourceRequestHandle] = []
    provided = threading.Event()

    def hold(request: mln.ResourceRequest, handle: mln.ResourceRequestHandle) -> None:
        handles.append(handle)
        provided.set()

    class CloseOnCancel:
        def __init__(self, owner: mln.ResourceRequestHandle) -> None:
            self.owner = owner

        def __call__(self) -> None:
            self.owner.close()

    harness.routes["custom://cycle.json"] = hold
    map_handle.set_style_url("custom://cycle.json")
    assert provided.wait(TIMEOUT)
    handle = handles.pop()
    callback = CloseOnCancel(handle)
    handle.set_cancel_callback(callback)
    owner, root = weakref.ref(handle), weakref.ref(callback)
    del callback, handle
    gc.collect()

    assert owner() is None
    assert root() is None
    result(map_handle.set_style_json(EMPTY_STYLE))
    harness.wait_event(mln.RuntimeEventType.MAP_STYLE_LOADED)


_SHUTDOWN_WITH_LIVE_HANDLES = """
import maplibre_native_ffi as m

runtime = m.runtime_create()
runtime.set_resource_provider(
    m.ResourceProvider(lambda request, handle: m.ResourceProviderDecision.HANDLE)
).result(10)
m.log_set_callback(lambda severity, event, code, message: 1)
live = runtime.map_create().result(10)
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

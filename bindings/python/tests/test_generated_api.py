"""Exercise generated ownership and callback protocols through the public API."""

import gc
import weakref
from dataclasses import replace
from threading import Event

from maplibre_native_ffi import api


def test_generated_values_and_batch_copy_survive_owner_release():
    with (
        api.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.set_style_json(b'{"version":8,"sources":{},"layers":[]}').result(
            timeout=5
        )
        snapshot = map_handle.snapshot_get()
        with runtime.drain_events() as batch:
            copied = batch.get()
        assert snapshot.logical_extent.width == 256
        assert copied.events
        assert all(event.source >= 0 for event in copied.events)
        assert all(isinstance(event.message, str) for event in copied.events)


def test_generated_wake_root_is_visible_to_cyclic_gc():
    woke = Event()
    retained = {}

    def wake(retained=retained):
        if retained.get("runtime") is not None:
            woke.set()

    callback_ref = weakref.ref(wake)
    runtime = api.runtime_create(
        replace(api.RuntimeOptions.default(), event_wake=api.Wake(wake))
    )
    retained["runtime"] = runtime
    runtime_ref = weakref.ref(runtime)
    map_handle = runtime.map_create().result(timeout=5)
    map_handle.set_style_json(b'{"version":8,"sources":{},"layers":[]}').result(
        timeout=5
    )
    assert woke.wait(5)
    map_handle.close().result(timeout=5)
    del map_handle, runtime, wake, retained
    gc.collect()
    assert runtime_ref() is None
    assert callback_ref() is None


def test_future_callback_can_wait_for_another_native_query():
    from threading import current_thread

    finished = Event()
    outcomes = []
    with (
        api.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):

        def completed(future):
            try:
                first = future.result()
                second = map_handle.camera_query().result(timeout=5)
                outcomes.append((current_thread().name, first, second))
            finally:
                finished.set()

        map_handle.camera_query().add_done_callback(completed)
        assert finished.wait(10)
        assert len(outcomes) == 1
        thread_name, first, second = outcomes[0]
        assert thread_name.startswith("maplibre-callback")
        assert first.camera == second.camera
        assert second.generation > first.generation

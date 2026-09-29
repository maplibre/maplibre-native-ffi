from __future__ import annotations

import contextlib
import gc
import http.server
import json
import math
import subprocess
import sys
import threading
import time
import typing
import weakref
from collections.abc import Callable
from concurrent.futures import Future
from dataclasses import replace
from pathlib import Path

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import api as camera
from maplibre_native_ffi import api as geo
from maplibre_native_ffi import api as map_module
from maplibre_native_ffi import api as offline
from maplibre_native_ffi import api as query
from maplibre_native_ffi import api as render
from maplibre_native_ffi import api as resource
from maplibre_native_ffi import api as style
from render_backend_helpers.runtime import drain_events

_EMPTY_STYLE_JSON = '{"version":8,"sources":{},"layers":[]}'
_EMPTY_STYLE_BYTES = _EMPTY_STYLE_JSON.encode()


def _json_object(value: object) -> bytes:
    return json.dumps(value, separators=(",", ":")).encode()


def _json_value(value: object) -> bytes:
    return _json_object(value)


@contextlib.contextmanager
def _online_network() -> typing.Iterator[None]:
    original = mln.network_status_get()
    mln.network_status_set(mln.NetworkStatus.ONLINE)
    try:
        yield
    finally:
        mln.network_status_set(original)


@contextlib.contextmanager
def _http_style_server() -> typing.Iterator[tuple[str, threading.Event]]:
    served = threading.Event()

    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self) -> None:
            served.set()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(_EMPTY_STYLE_BYTES)))
            self.end_headers()
            self.wfile.write(_EMPTY_STYLE_BYTES)

        def log_message(self, format: str, *args: object) -> None:
            return

    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        host, port = server.server_address
        yield f"http://{host}:{port}/style.json", served
    finally:
        server.shutdown()
        thread.join(timeout=2)
        server.server_close()


def _wait_for_runtime_event(
    runtime: mln.RuntimeHandle,
    event_type: mln.RuntimeEventType,
    *,
    iterations: int = 5000,
) -> mln.RuntimeEvent:
    for _ in range(iterations):
        for event in drain_events(runtime).events:
            if event.type == event_type:
                return event
        time.sleep(0.001)
    raise AssertionError(f"runtime event {event_type!r} was not observed")


def _wait_for_provider_handle(
    handles: list[resource.ResourceRequestHandle],
    *,
    iterations: int = 5000,
) -> resource.ResourceRequestHandle:
    for _ in range(iterations):
        if handles:
            return handles.pop(0)
        time.sleep(0.001)
    raise AssertionError("resource provider did not expose a handled request")


def _await[T](future: Future[T]) -> T:
    return future.result(timeout=5)


def _commit(future: Future[mln.CommandCompletion]) -> None:
    """Await one submitted command and require that it committed."""
    assert _await(future).disposition == mln.CommandDisposition.COMMITTED


def _assert_command_failed(
    future: Future[mln.CommandCompletion], status: mln.Status
) -> mln.CommandCompletion:
    completion = _await(future)
    assert completion.disposition == mln.CommandDisposition.FAILED
    assert completion.native_status_code == status.native_code
    assert completion.diagnostic
    return completion


def test_network_status_round_trips_through_public_api() -> None:
    original = mln.network_status_get()
    try:
        mln.network_status_set(mln.NetworkStatus.OFFLINE)
        assert mln.network_status_get() == mln.NetworkStatus.OFFLINE
        mln.network_status_set(mln.NetworkStatus.ONLINE)
        assert mln.network_status_get() == mln.NetworkStatus.ONLINE
    finally:
        mln.network_status_set(original)


def test_unknown_network_status_setter_raises_invalid_argument() -> None:
    with pytest.raises(mln.InvalidArgumentError) as raised:
        mln.network_status_set(mln.NetworkStatus(999_001))

    assert raised.value.status == mln.Status.INVALID_ARGUMENT
    assert raised.value.native_status_code == -1
    assert "invalid" in raised.value.diagnostic


def test_native_status_conversion_preserves_status_and_diagnostic() -> None:
    with pytest.raises(mln.InvalidArgumentError) as raised:
        mln.network_status_set(999_001)

    error = raised.value
    copied = error.diagnostic

    with pytest.raises(mln.InvalidArgumentError) as later:
        mln.projected_meters_for_lat_lng(mln.LatLng(1000.0, 0.0))

    assert error.status == mln.Status.INVALID_ARGUMENT
    assert error.native_status_code == mln.Status.INVALID_ARGUMENT.native_code
    assert "network status" in error.diagnostic
    assert error.diagnostic == copied
    assert later.value.diagnostic != copied


def _native_invalid_network_status_error() -> mln.InvalidArgumentError:
    with pytest.raises(mln.InvalidArgumentError) as raised:
        mln.network_status_set(999_001)
    return raised.value


def test_public_type_hints_are_resolvable():
    from maplibre_native_ffi import api

    for name in api.__all__:
        target = getattr(api, name)
        if isinstance(target, type) and target.__module__.startswith(
            "maplibre_native_ffi"
        ):
            typing.get_type_hints(target)
            for method in vars(target).values():
                if callable(method) and hasattr(method, "__annotations__"):
                    typing.get_type_hints(method)
        elif callable(target):
            typing.get_type_hints(target)
    hints = typing.get_type_hints(mln.MapHandle.set_style_json)
    assert hints["json"] is bytes
    assert typing.get_args(
        typing.get_type_hints(mln.RuntimeHandle.map_create)["return"]
    ) == (mln.MapHandle,)


def test_runtime_handle_context_manager_closes_once() -> None:
    with mln.runtime_create() as runtime:
        assert not runtime.closed

    assert runtime.closed
    runtime.close()
    assert runtime.closed


@pytest.mark.skipif(
    not sys.executable,
    reason="Embedded Python has no interpreter executable for the shutdown subprocess",
)
def test_closed_handle_finalizers_are_quiet_at_interpreter_shutdown():
    result = subprocess.run(
        [
            sys.executable,
            "-Werror::ResourceWarning",
            "-c",
            "import maplibre_native_ffi as m; r=m.runtime_create(); h=r.map_create().result(5); h.close().result(5); r.close().result(5)",
        ],
        check=False,
        capture_output=True,
        text=True,
        timeout=15,
    )
    assert result.returncode == 0, result.stderr
    assert "Exception ignored" not in result.stderr


def test_multiple_runtimes_are_independent() -> None:
    first = mln.runtime_create()
    second = mln.runtime_create()
    try:
        first_map = first.map_create().result(timeout=5)
        second_map = second.map_create().result(timeout=5)
        _await(first_map.set_style_json(_EMPTY_STYLE_BYTES))
        _await(second_map.request_repaint())

        # Each runtime queues only its own map's events, and closing one leaves
        # the other's queue and maps untouched.
        first_events = _drain_runtime_events(first)
        assert first_events
        assert {event.source for event in first_events} == {first_map.id}
        _await(first_map.close())
        _await(first.close())

        assert not second_map.closed
        assert _await(second_map.camera_query()).generation > 0
        # A closed map keeps its ID, so events drained later still identify it.
        assert first_map.id not in {
            event.source for event in _drain_runtime_events(second)
        }
        _await(second_map.close())
    finally:
        second.close()
        first.close()


def test_accepted_native_future_cannot_claim_successful_cancellation() -> None:
    runtime = mln.runtime_create()
    try:
        future = runtime.barrier()

        assert future.cancel() is False
        future.result(timeout=5)
    finally:
        runtime.close().result(timeout=30)


def test_abandoned_creation_result_releases_its_native_map() -> None:
    runtime = mln.runtime_create()
    future = runtime.map_create()
    future.result(timeout=5)
    runtime.barrier().result(timeout=5)
    del future
    gc.collect()
    runtime.close().result(timeout=30)


def test_runtime_and_map_are_usable_across_python_threads() -> None:
    runtime = mln.runtime_create()
    map_handle = runtime.map_create().result(timeout=5)
    failures: list[BaseException] = []
    completions: list[mln.CommandCompletion] = []

    def use_handles() -> None:
        try:
            assert map_handle.snapshot_get().generation > 0
            completions.append(map_handle.request_repaint().result(timeout=5))
            # The runtime barrier is reached from this worker thread too.
            runtime.barrier().result(timeout=5)
            assert _await(map_handle.camera_query()).generation > 0
            map_handle.close()
        except Exception as error:  # noqa: BLE001 - transport worker failures
            failures.append(error)

    thread = threading.Thread(target=use_handles)
    thread.start()
    thread.join()
    assert not failures
    assert completions[0].disposition == mln.CommandDisposition.COMMITTED
    assert completions[0].generation > 0
    assert map_handle.closed
    runtime.close()


def test_render_result_preserves_an_unknown_native_disposition() -> None:
    unknown = render.RenderResult(777)

    assert unknown.is_unknown
    assert unknown.native_code == 777


def test_map_handle_context_manager_closes_once() -> None:
    with mln.runtime_create() as runtime:
        with pytest.raises(TypeError, match="positional argument"):
            mln.MapHandle(
                runtime,
                replace(
                    mln.MapOptions.default(),
                    initial_extent=mln.LogicalExtent(128, 64, 1.0),
                ),
            )

        with runtime.map_create(
            replace(
                mln.MapOptions.default(), initial_extent=mln.LogicalExtent(128, 64, 1.0)
            )
        ).result(timeout=5) as map_handle:
            assert not map_handle.closed
            _await(map_handle.request_repaint())

        assert map_handle.closed
        map_handle.close()
        assert map_handle.closed


def test_map_options_accept_fast_pfor_decoding() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create(
            replace(
                mln.MapOptions.default(),
                fast_pfor_enabled=True,
                initial_extent=mln.LogicalExtent(64, 64, 1.0),
            )
        ).result(timeout=5) as map_handle,
    ):
        snapshot = map_handle.snapshot_get()
        assert (
            snapshot.logical_extent.width,
            snapshot.logical_extent.height,
            snapshot.logical_extent.scale_factor,
        ) == (
            64,
            64,
            1.0,
        )


def test_unset_map_options_take_the_c_creation_defaults() -> None:
    # Unset fields take the C API defaults rather than values this binding
    # repeats, so mln_map_options_default() stays the single source for them.
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        snapshot = map_handle.snapshot_get()
        assert (
            snapshot.logical_extent.width,
            snapshot.logical_extent.height,
            snapshot.logical_extent.scale_factor,
        ) == (
            256,
            256,
            1.0,
        )


def test_runtime_rejects_close_while_map_is_live() -> None:
    runtime = mln.runtime_create()
    map_handle = runtime.map_create(
        replace(mln.MapOptions.default(), initial_extent=mln.LogicalExtent(64, 64, 1.0))
    ).result(timeout=5)
    try:
        with pytest.raises(mln.InvalidStateError) as raised:
            runtime.close()

        assert raised.value.status == mln.Status.INVALID_STATE
        assert raised.value.native_status_code == mln.Status.INVALID_STATE.native_code
    finally:
        map_handle.close()
        runtime.close()


def test_still_image_request_rejects_continuous_map_mode() -> None:

    with (
        mln.runtime_create() as runtime,
        runtime.map_create(
            replace(mln.MapOptions.default(), map_mode=mln.MapMode.CONTINUOUS)
        ).result(timeout=5) as map_handle,
        pytest.raises(mln.InvalidStateError) as raised,
    ):
        _await(map_handle.request_still_image())

    assert raised.value.status == mln.Status.INVALID_STATE


def test_map_create_from_closed_runtime_reports_invalid_state() -> None:
    runtime = mln.runtime_create()
    runtime.close()
    stale = _native_invalid_network_status_error().diagnostic

    with pytest.raises(mln.InvalidStateError) as raised:
        runtime.map_create().result(timeout=5)

    assert raised.value.native_status_code is None
    assert raised.value.diagnostic == "handle is closed"
    assert raised.value.diagnostic != stale


def test_map_debug_and_status_options_round_trip_the_snapshot() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        debug_options = (
            map_module.MapDebugOption.TILE_BORDERS
            | map_module.MapDebugOption.PARSE_STATUS
        )
        completion = map_handle.set_debug_options(debug_options)
        map_handle.set_rendering_stats_view_enabled(True)
        runtime.barrier().result(timeout=5)

        # Snapshot fence: the commit reports the generation that published its
        # effect, and a snapshot at or past that generation observes it.
        finished = _await_command_completion(completion)
        assert finished.disposition == mln.CommandDisposition.COMMITTED
        snapshot = map_handle.snapshot_get()
        assert snapshot.generation >= finished.generation
        assert snapshot.debug_options == debug_options
        assert snapshot.rendering_stats_view_enabled is True
        assert isinstance(snapshot.fully_loaded, bool)

        map_handle.set_debug_options(map_module.MapDebugOption(0))
        map_handle.set_rendering_stats_view_enabled(False)
        runtime.barrier().result(timeout=5)
        snapshot = map_handle.snapshot_get()
        assert snapshot.debug_options == map_module.MapDebugOption(0)
        assert snapshot.rendering_stats_view_enabled is False


def test_style_url_rejects_embedded_nul_before_native_call() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        stale = _native_invalid_network_status_error().diagnostic

        with pytest.raises(mln.InvalidArgumentError) as raised:
            map_handle.set_style_url("bad\0url")

    assert raised.value.status == mln.Status.INVALID_ARGUMENT
    assert raised.value.native_status_code is None
    assert "embedded NUL" in raised.value.diagnostic
    assert raised.value.diagnostic != stale


def _point_collection(*names: str) -> bytes:
    return _json_object(
        {
            "type": "FeatureCollection",
            "features": [
                {
                    "type": "Feature",
                    "geometry": {
                        "type": "Point",
                        "coordinates": [float(index), float(index)],
                    },
                    "properties": {"name": name},
                }
                for index, name in enumerate(names)
            ],
        }
    )


def test_geojson_source_data_prepares_off_thread_without_a_runtime() -> None:
    # Preparation needs no runtime or map and runs on any thread; the handle
    # then installs onto sources owned by a map created afterwards.
    results: list[style.GeojsonSourceDataHandle | BaseException] = []

    def prepare() -> None:
        try:
            results.append(
                style.geojson_source_data_create(_point_collection("worker"))
            )
        except BaseException as error:  # noqa: BLE001 - report into the test
            results.append(error)

    worker = threading.Thread(target=prepare)
    worker.start()
    worker.join()
    (prepared,) = results
    assert isinstance(prepared, style.GeojsonSourceDataHandle)
    with prepared:
        assert prepared.closed is False
        with (
            mln.runtime_create() as runtime,
            runtime.map_create().result(timeout=5) as map_handle,
        ):
            map_handle.set_style_json(_EMPTY_STYLE_BYTES)
            completion = map_handle.add_geojson_source_data("worker-points", prepared)
            finished = _await_command_completion(completion)
            assert finished.disposition == mln.CommandDisposition.COMMITTED
            info = _await(map_handle.get_style_source_info("worker-points"))
            assert info is not None
            assert info.info.type == style.StyleSourceType.GEOJSON
    assert prepared.closed is True


def test_geojson_source_data_installs_on_many_sources_and_outlives_release() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        prepared = style.geojson_source_data_create(_point_collection("one", "two"))
        # Install calls borrow the handle, so one prepared value serves any
        # number of sources, and the submit-time lease keeps the prepared
        # index alive after the handle is released.
        map_handle.add_geojson_source_data("points-a", prepared)
        map_handle.add_geojson_source_data("points-b", prepared)
        update_id = map_handle.set_geojson_source_data("points-a", prepared)
        prepared.close()
        updated = _await_command_completion(update_id)
        assert updated.disposition == mln.CommandDisposition.COMMITTED
        # Release never invalidates a source the data was installed on.
        for source_id in ("points-a", "points-b"):
            info = _await(map_handle.get_style_source_info(source_id))
            assert info is not None
            assert info.info.type == style.StyleSourceType.GEOJSON


def test_geojson_source_data_close_is_idempotent_and_blocks_installs() -> None:
    prepared = style.geojson_source_data_create(_point_collection("one"))
    prepared.close()
    assert prepared.closed is True
    prepared.close()

    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        with pytest.raises(mln.InvalidStateError, match="closed"):
            map_handle.add_geojson_source_data("points", prepared)
        with pytest.raises(mln.InvalidStateError, match="closed"):
            map_handle.set_geojson_source_data("points", prepared)


def test_geojson_source_data_create_validates_cluster_input() -> None:
    # Clustering applies to feature collections of points only, and that
    # validation now happens at preparation time with no runtime involved.
    bare_geometry = _json_object({"type": "Point", "coordinates": [0.0, 0.0]})
    with pytest.raises(mln.InvalidArgumentError):
        style.geojson_source_data_create(
            bare_geometry, style.GeojsonSourceOptions(cluster=True)
        )


def test_set_geojson_source_data_rejects_mismatched_baked_in_options() -> None:
    document = _point_collection("one", "two")
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        with style.geojson_source_data_create(
            document,
            style.GeojsonSourceOptions(
                cluster=True,
                cluster_properties=_json_object({"names": ["+", 1]}),
            ),
        ) as clustered:
            map_handle.add_geojson_source_data("points", clustered)

        # The mismatch is a map-thread validation, so the command fails
        # asynchronously with INVALID_ARGUMENT instead of raising at submit.
        with style.geojson_source_data_create(document) as plain:
            rejected_id = map_handle.set_geojson_source_data("points", plain)
        _assert_command_failed(rejected_id, mln.Status.INVALID_ARGUMENT)

        # Different cluster aggregations would change cluster feature
        # properties under the source's layers, so they are rejected too.
        with style.geojson_source_data_create(
            document,
            style.GeojsonSourceOptions(
                cluster=True,
                cluster_properties=_json_object({"renamed": ["+", 1]}),
            ),
        ) as reclustered:
            reclustered_id = map_handle.set_geojson_source_data("points", reclustered)
        _assert_command_failed(reclustered_id, mln.Status.INVALID_ARGUMENT)

        # Aggregations compare by parsed expression equality, so equivalent
        # cluster_properties JSON matches regardless of formatting.
        with style.geojson_source_data_create(
            document,
            style.GeojsonSourceOptions(
                cluster=True,
                cluster_properties=b' { "names" : ["+", 1] } ',
            ),
        ) as matching:
            matching_id = map_handle.set_geojson_source_data("points", matching)
        accepted = _await_command_completion(matching_id)
        assert accepted.disposition == mln.CommandDisposition.COMMITTED


def test_set_geojson_source_synchronous_tiling_overrides_at_runtime() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        with style.geojson_source_data_create(_point_collection("one")) as prepared:
            map_handle.add_geojson_source_data("points", prepared)
        enabled_id = map_handle.set_geojson_source_synchronous_tiling("points", True)
        disabled_id = map_handle.set_geojson_source_synchronous_tiling("points", False)
        for completion in (_await(enabled_id), _await(disabled_id)):
            assert completion.disposition == mln.CommandDisposition.COMMITTED

        # A missing source is a map-thread lookup, so the command fails
        # asynchronously with NOT_FOUND instead of raising at submit.
        missing_id = map_handle.set_geojson_source_synchronous_tiling("missing", True)
        _assert_command_failed(missing_id, mln.Status.NOT_FOUND)


@pytest.mark.parametrize(
    "submit",
    [
        pytest.param(
            lambda handle: handle.set_geojson_source_url(
                "missing", "https://example.test/points.geojson"
            ),
            id="set_geojson_source_url",
        ),
        pytest.param(
            lambda handle: handle.set_image_source_url(
                "missing", "https://example.test/overlay.png"
            ),
            id="set_image_source_url",
        ),
        pytest.param(
            lambda handle: handle.add_hillshade_layer("hillshade", "missing"),
            id="add_hillshade_layer",
        ),
        pytest.param(
            lambda handle: handle.move_style_layer("missing"),
            id="move_style_layer",
        ),
        pytest.param(
            lambda handle: handle.set_layer_property(
                "missing", "visibility", b'"none"'
            ),
            id="set_layer_property",
        ),
        pytest.param(
            lambda handle: handle.set_layer_source_id("missing", "points"),
            id="set_layer_source_id",
        ),
        pytest.param(
            lambda handle: handle.set_layer_min_zoom("missing", 1.0),
            id="set_layer_min_zoom",
        ),
        pytest.param(
            lambda handle: handle.set_location_indicator_bearing("missing", 90.0),
            id="set_location_indicator_bearing",
        ),
    ],
)
def test_style_mutations_report_not_found_for_a_missing_id(
    submit: Callable[[mln.MapHandle], Future[mln.CommandCompletion]],
) -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        _commit(map_handle.set_style_json(_EMPTY_STYLE_BYTES))

        # A missing ID is a map-worker lookup, so it is reported through the
        # completion rather than as an acceptance failure.
        _assert_command_failed(submit(map_handle), mln.Status.NOT_FOUND)


@pytest.mark.parametrize(
    "start",
    [
        pytest.param(
            lambda handle: handle.get_layer_property("missing", "visibility"),
            id="get_layer_property",
        ),
        pytest.param(
            lambda handle: handle.get_layer_filter("missing"),
            id="get_layer_filter",
        ),
        pytest.param(
            lambda handle: handle.copy_layer_source_id("missing"),
            id="get_layer_source_id",
        ),
    ],
)
def test_style_layer_reads_raise_not_found_for_a_missing_layer(
    start: Callable[[mln.MapHandle], Future[object]],
) -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        _commit(map_handle.set_style_json(_EMPTY_STYLE_BYTES))

        with pytest.raises(mln.NotFoundError) as raised:
            _await(start(map_handle))
        assert raised.value.status == mln.Status.NOT_FOUND
        assert raised.value.diagnostic


def test_style_source_metadata_enums_preserve_unknown_values() -> None:
    for enum_type in (
        style.StyleTileScheme,
        style.StyleVectorTileEncoding,
        style.StyleRasterDemEncoding,
    ):
        value = enum_type(999_040)
        assert value.is_unknown
        assert value.native_code == 999_040


def test_style_source_volatility_round_trips_through_public_api() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        _await_command_completion(map_handle.set_style_json(_EMPTY_STYLE_BYTES))
        _await_command_completion(
            map_handle.add_vector_source_tiles(
                "volatile-source", ("https://example.test/tiles/{z}/{x}/{y}.mvt",)
            ),
        )

        info = _await(map_handle.get_style_source_info("volatile-source"))
        assert info is not None
        assert info.info.is_volatile is False

        _await_command_completion(
            map_handle.set_style_source_volatile("volatile-source", True)
        )
        info = _await(map_handle.get_style_source_info("volatile-source"))
        assert info is not None
        assert info.info.is_volatile is True

        _await_command_completion(
            map_handle.set_style_source_volatile("volatile-source", False)
        )
        info = _await(map_handle.get_style_source_info("volatile-source"))
        assert info is not None
        assert info.info.is_volatile is False

        # A missing source is reported through the completion, not as an
        # acceptance failure.
        _assert_command_failed(
            map_handle.set_style_source_volatile("missing-source", True),
            mln.Status.NOT_FOUND,
        )


def test_loaded_style_document_and_url_read_back_what_was_loaded() -> None:
    style_json = _EMPTY_STYLE_BYTES
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        # Nothing parsed and nothing requested yet.
        assert _await(map_handle.loaded_style_json()) == b""
        assert _await(map_handle.style_url()) == ""

        # The document reads back byte-for-byte, so it can be reloaded
        # unchanged.
        map_handle.set_style_json(style_json)
        assert _await(map_handle.loaded_style_json()) == style_json
        # Inline JSON clears the URL.
        assert _await(map_handle.style_url()) == ""

        # The URL is request state, recorded before the load can succeed,
        # while the document still reports the style that last parsed.
        map_handle.set_style_url("https://example.test/style.json")
        assert _await(map_handle.style_url()) == "https://example.test/style.json"
        assert _await(map_handle.loaded_style_json()) == style_json


def test_style_source_url_metadata_and_removal_public_api() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        map_handle.add_style_source_json(
            "style-json-points",
            _json_object(
                {
                    "type": "geojson",
                    "data": {
                        "type": "FeatureCollection",
                        "features": [],
                    },
                }
            ),
        )
        map_handle.add_geojson_source_url(
            "points",
            "https://example.test/points.geojson",
            style.GeojsonSourceOptions(
                min_zoom=1.0,
                max_zoom=14.0,
                tolerance=0.5,
                tile_size=256,
                buffer=64,
                line_metrics=True,
            ),
        )
        inline_points = _json_object(
            {
                "type": "FeatureCollection",
                "features": [
                    {
                        "type": "Feature",
                        "geometry": {"type": "Point", "coordinates": [2.0, 1.0]},
                        "properties": {"name": "one"},
                        "id": "point-1",
                    }
                ],
            }
        )
        with style.geojson_source_data_create(
            inline_points,
            style.GeojsonSourceOptions(
                cluster=True,
                cluster_radius=40,
                cluster_max_zoom=12.0,
                cluster_min_points=3,
                cluster_properties=_json_object(
                    {"name_count": ["+", ["case", ["has", "name"], 1, 0]]}
                ),
            ),
        ) as inline_data:
            map_handle.add_geojson_source_data("inline-points", inline_data)
            map_handle.set_geojson_source_url(
                "inline-points",
                "https://example.test/inline-points.geojson",
            )
            map_handle.set_geojson_source_data("inline-points", inline_data)
        map_handle.add_vector_source_url(
            "vector-tiles",
            "https://example.test/vector.json",
            style.StyleTileSourceOptions(
                min_zoom=1.0,
                max_zoom=10.0,
                vector_encoding=style.StyleVectorTileEncoding.MVT,
            ),
        )
        map_handle.add_raster_source_url(
            "raster-tiles",
            "https://example.test/raster.json",
            style.StyleTileSourceOptions(tile_size=256),
        )
        map_handle.add_raster_dem_source_url(
            "dem-tiles",
            "https://example.test/dem.json",
            style.StyleTileSourceOptions(
                tile_size=512,
                raster_encoding=style.StyleRasterDemEncoding.MAPBOX,
            ),
        )
        map_handle.add_vector_source_tiles(
            "vector-inline",
            (
                "https://a.example.test/vector/{z}/{x}/{y}.mlt",
                "https://b.example.test/vector/{z}/{x}/{y}.mlt",
            ),
            style.StyleTileSourceOptions(
                min_zoom=2.0,
                max_zoom=7.0,
                attribution="Example attribution",
                scheme=style.StyleTileScheme.TMS,
                bounds=geo.LatLngBounds(
                    geo.LatLng(-5.0, -10.0), geo.LatLng(15.0, 20.0)
                ),
                vector_encoding=style.StyleVectorTileEncoding.MLT,
            ),
        )
        map_handle.add_raster_source_tiles(
            "raster-inline",
            ("https://example.test/raster/{z}/{x}/{y}.png",),
        )
        map_handle.add_raster_dem_source_tiles(
            "dem-inline",
            ("https://example.test/dem/{z}/{x}/{y}.png",),
        )

        def source_type(source_id: str) -> style.StyleSourceType | None:
            info = _await(map_handle.get_style_source_info(source_id))
            return info.info.type if info is not None else None

        # The info getter's found flag is the existence check.
        assert _await(map_handle.get_style_source_info("points")) is not None
        assert _await(map_handle.get_style_source_info("missing")) is None
        assert source_type("style-json-points") == style.StyleSourceType.GEOJSON
        assert source_type("points") == style.StyleSourceType.GEOJSON
        assert source_type("inline-points") == style.StyleSourceType.GEOJSON
        assert source_type("vector-tiles") == style.StyleSourceType.VECTOR
        assert source_type("raster-tiles") == style.StyleSourceType.RASTER
        assert source_type("dem-tiles") == style.StyleSourceType.RASTER_DEM
        assert source_type("vector-inline") == style.StyleSourceType.VECTOR
        assert source_type("raster-inline") == style.StyleSourceType.RASTER
        assert source_type("dem-inline") == style.StyleSourceType.RASTER_DEM
        source_ids = _await(map_handle.list_style_source_ids())
        assert "style-json-points" in source_ids
        assert "points" in source_ids
        assert "inline-points" in source_ids
        assert "vector-tiles" in source_ids
        assert "raster-tiles" in source_ids
        assert "dem-tiles" in source_ids
        assert "vector-inline" in source_ids
        assert "raster-inline" in source_ids
        assert "dem-inline" in source_ids

        info = _await(map_handle.get_style_source_info("points"))
        assert info is not None
        assert info.info.type == style.StyleSourceType.GEOJSON
        assert info.attribution is None
        assert info.url == "https://example.test/points.geojson"
        assert info.info.tilejson is None
        assert _await(map_handle.get_style_source_info("missing")) is None

        remote_info = _await(map_handle.get_style_source_info("vector-tiles"))
        assert remote_info is not None
        assert remote_info.url == "https://example.test/vector.json"
        assert remote_info.info.tilejson is None

        copied_inline = _await(map_handle.get_style_source_info("vector-inline"))
        assert copied_inline is not None
        assert copied_inline.url is None
        assert copied_inline.attribution == "Example attribution"
        assert copied_inline.info.tile_size == 512
        assert copied_inline.info.vector_encoding == style.StyleVectorTileEncoding.MLT
        assert copied_inline.info.tilejson is not None
        assert copied_inline.tile_urls == (
            "https://a.example.test/vector/{z}/{x}/{y}.mlt",
            "https://b.example.test/vector/{z}/{x}/{y}.mlt",
        )
        assert copied_inline.info.tilejson.min_zoom == 2.0
        assert copied_inline.info.tilejson.max_zoom == 7.0
        assert copied_inline.info.tilejson.scheme == style.StyleTileScheme.TMS
        assert copied_inline.info.bounds == geo.LatLngBounds(
            geo.LatLng(-5.0, -10.0), geo.LatLng(15.0, 20.0)
        )

        # The single-field reads see the same retained metadata.
        assert _await(map_handle.copy_style_source_attribution("vector-inline")) == (
            "Example attribution"
        )
        assert _await(map_handle.copy_style_source_url("vector-inline")) is None
        assert _await(
            map_handle.get_style_source_tile_urls("vector-inline")
        ).tile_urls == (
            "https://a.example.test/vector/{z}/{x}/{y}.mlt",
            "https://b.example.test/vector/{z}/{x}/{y}.mlt",
        )
        assert _await(map_handle.copy_style_source_url("vector-tiles")) == (
            "https://example.test/vector.json"
        )
        assert _await(map_handle.copy_style_source_attribution("vector-tiles")) is None
        assert (
            _await(map_handle.get_style_source_tile_urls("vector-tiles")).tile_urls
            == ()
        )
        assert _await(map_handle.copy_style_source_url("points")) == (
            "https://example.test/points.geojson"
        )
        assert _await(map_handle.copy_style_source_url("missing")) is None
        assert _await(map_handle.copy_style_source_attribution("missing")) is None
        assert _await(map_handle.get_style_source_tile_urls("missing")) is None

        removed_id = map_handle.remove_style_source("points")
        removed = _await_command_completion(removed_id)
        assert removed.disposition == mln.CommandDisposition.COMMITTED
        assert _await(map_handle.get_style_source_info("points")) is None

        # Removing the missing source again fails the command with NOT_FOUND.
        missing_id = map_handle.remove_style_source("points")
        _assert_command_failed(missing_id, mln.Status.NOT_FOUND)

        for source_id in (
            "style-json-points",
            "inline-points",
            "vector-tiles",
            "raster-tiles",
            "dem-tiles",
            "vector-inline",
            "raster-inline",
            "dem-inline",
        ):
            map_handle.remove_style_source(source_id)
        source_ids = _await(map_handle.list_style_source_ids())
        assert "style-json-points" not in source_ids
        assert "points" not in source_ids
        assert "inline-points" not in source_ids
        assert "vector-tiles" not in source_ids
        assert "raster-tiles" not in source_ids
        assert "dem-tiles" not in source_ids
        assert "vector-inline" not in source_ids
        assert "raster-inline" not in source_ids
        assert "dem-inline" not in source_ids
        assert copied_inline.info.tilejson is not None
        assert len(copied_inline.tile_urls) == 2


def test_image_source_url_image_and_coordinates_public_api() -> None:
    coordinates = (
        geo.LatLng(1.0, 2.0),
        geo.LatLng(1.0, 3.0),
        geo.LatLng(0.0, 3.0),
        geo.LatLng(0.0, 2.0),
    )
    updated_coordinates = (
        geo.LatLng(2.0, 2.0),
        geo.LatLng(2.0, 3.0),
        geo.LatLng(1.0, 3.0),
        geo.LatLng(1.0, 2.0),
    )
    image = mln.PremultipliedRgba8Image(1, 1, 4, bytes([0, 255, 0, 255]))

    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        map_handle.add_image_source_url(
            "overlay-url",
            coordinates,
            "https://example.test/overlay.png",
        )
        map_handle.add_image_source_image("overlay-inline", coordinates, image)

        url_info = _await(map_handle.get_style_source_info("overlay-url"))
        inline_info = _await(map_handle.get_style_source_info("overlay-inline"))
        assert url_info is not None
        assert url_info.info.type == style.StyleSourceType.IMAGE
        assert inline_info is not None
        assert inline_info.info.type == style.StyleSourceType.IMAGE
        assert (
            _await(map_handle.get_image_source_coordinates("overlay-url"))
            == coordinates
        )
        assert _await(map_handle.get_image_source_coordinates("missing")) is None

        _commit(
            map_handle.set_image_source_url(
                "overlay-url",
                "https://example.test/overlay-2.png",
            )
        )
        _commit(map_handle.set_image_source_image("overlay-url", image))
        _commit(
            map_handle.set_image_source_coordinates("overlay-url", updated_coordinates)
        )
        assert (
            _await(map_handle.get_image_source_coordinates("overlay-url"))
            == updated_coordinates
        )

        _commit(map_handle.remove_style_source("overlay-url"))
        _commit(map_handle.remove_style_source("overlay-inline"))
        assert _await(map_handle.get_style_source_info("overlay-url")) is None
        assert _await(map_handle.get_style_source_info("overlay-inline")) is None


def test_style_json_light_layer_property_and_filter_public_api() -> None:
    background = _json_object({"id": "json-background", "type": "background"})
    circle = _json_object({"id": "json-circle", "type": "circle", "source": "points"})
    raw_filter = ["==", ["get", "kind"], "park"]
    filter_value = _json_value(raw_filter)
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        map_handle.add_geojson_source_url(
            "points",
            "https://example.test/points.geojson",
        )
        with pytest.raises(TypeError, match="instance of 'bytes'"):
            map_handle.add_style_layer_json(
                typing.cast(
                    typing.Any,
                    {"id": "raw-dict", "type": "background"},
                )
            )
        with pytest.raises(TypeError, match="instance of 'bytes'"):
            map_handle.set_style_light_property(
                "intensity",
                typing.cast(typing.Any, 1),
            )
        map_handle.add_style_layer_json(background)
        map_handle.add_style_layer_json(circle)
        map_handle.set_layer_property(
            "json-background",
            "background-color",
            _json_value("#ff0000"),
        )
        map_handle.set_layer_filter("json-circle", filter_value)
        map_handle.set_style_light_json(_json_object({"anchor": "viewport"}))
        map_handle.set_style_light_property("intensity", _json_value(0.5))

        layer_json = _await(map_handle.get_style_layer_json("json-background"))
        assert layer_json is not None
        assert json.loads(layer_json) == {
            "id": "json-background",
            "type": "background",
            "paint": {"background-color": ["rgba", 255, 0, 0, 1]},
        }
        assert _await(map_handle.get_style_layer_json("missing")) is None
        background_color = _await(
            map_handle.get_layer_property(
                "json-background",
                "background-color",
            )
        )
        assert json.loads(background_color) == ["rgba", 255, 0, 0, 1]
        assert (
            json.loads(_await(map_handle.get_layer_filter("json-circle"))) == raw_filter
        )
        assert (
            json.loads(_await(map_handle.get_style_light_property("anchor")))
            == "viewport"
        )
        assert (
            json.loads(_await(map_handle.get_style_light_property("intensity"))) == 0.5
        )

        completion = map_handle.set_style_light_property("intensity", b"Infinity")
        _assert_command_failed(completion, mln.Status.INVALID_ARGUMENT)
        assert (
            json.loads(_await(map_handle.get_style_light_property("intensity"))) == 0.5
        )

        map_handle.set_layer_filter("json-circle", None)
        assert _await(map_handle.get_layer_filter("json-circle")) is None


def test_style_image_metadata_copy_and_removal_public_api() -> None:
    image = mln.PremultipliedRgba8Image(1, 1, 4, bytes([255, 0, 0, 255]))
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        map_handle.set_style_image(
            "marker",
            image,
            style.StyleImageOptions(pixel_ratio=2.0, sdf=True),
        )

        info = _await(map_handle.get_style_image_info("marker"))
        assert info is not None
        assert info.info.width == 1
        assert info.info.height == 1
        assert info.info.stride == 4
        assert info.info.byte_length == 4
        assert info.info.pixel_ratio == pytest.approx(2.0)
        assert info.info.sdf is True
        assert _await(map_handle.get_style_image_info("missing")) is None

        # The narrow copy carries only the tightly packed pixels; its metadata
        # comes from the image-info query above.
        copied = _await(map_handle.copy_style_image_premultiplied_rgba8("marker"))
        assert copied == image.pixels
        assert (
            _await(map_handle.copy_style_image_premultiplied_rgba8("missing")) is None
        )

        removed_id = map_handle.remove_style_image("marker")
        removed = _await_command_completion(removed_id)
        assert removed.disposition == mln.CommandDisposition.COMMITTED
        assert _await(map_handle.get_style_image_info("marker")) is None

        # Removing the missing image again fails the command with NOT_FOUND.
        missing_id = map_handle.remove_style_image("marker")
        _assert_command_failed(missing_id, mln.Status.NOT_FOUND)


def test_builtin_style_layers_and_location_indicator_public_api() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        map_handle.add_raster_dem_source_url(
            "dem",
            "https://example.test/dem.json",
            style.StyleTileSourceOptions(
                tile_size=512,
                raster_encoding=style.StyleRasterDemEncoding.MAPBOX,
            ),
        )
        map_handle.add_hillshade_layer("hillshade", "dem")
        map_handle.add_color_relief_layer("relief", "dem")
        map_handle.add_location_indicator_layer("location")
        map_handle.set_location_indicator_location(
            "location",
            geo.LatLng(1.0, 2.0),
            3.0,
        )
        map_handle.set_location_indicator_bearing("location", 45.0)
        map_handle.set_location_indicator_accuracy_radius("location", 5.0)
        map_handle.set_location_indicator_image_name(
            "location",
            style.LocationIndicatorImageKind.TOP,
            "marker",
        )

        def layer_type(layer_id: str) -> str | None:
            info = _await(map_handle.get_style_layer_info(layer_id))
            return info.info.type if info is not None else None

        assert layer_type("hillshade") == "hillshade"
        assert layer_type("relief") == "color-relief"
        assert layer_type("location") == "location-indicator"
        map_handle.remove_style_layer("hillshade")
        map_handle.remove_style_layer("relief")
        map_handle.remove_style_layer("location")
        assert _await(map_handle.get_style_layer_info("hillshade")) is None
        assert _await(map_handle.get_style_layer_info("relief")) is None
        assert _await(map_handle.get_style_layer_info("location")) is None


def test_nine_patch_style_image_round_trips_public_api() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        image = mln.PremultipliedRgba8Image(2, 2, 8, bytes(16))
        options = style.StyleImageOptions(
            stretch_x=(style.ImageStretch(0.0, 1.0),),
            stretch_y=(style.ImageStretch(0.0, 1.0), style.ImageStretch(1.0, 2.0)),
            content=style.ImageContent(0.5, 0.5, 1.5, 1.5),
            text_fit_height=style.StyleImageTextFit.PROPORTIONAL,
        )
        map_handle.set_style_image("patch", image, options)

        info = _await(map_handle.get_style_image_info("patch"))
        assert info is not None
        assert info.info.stretch_x_count == 1
        assert info.info.stretch_y_count == 2
        assert info.info.content == style.ImageContent(0.5, 0.5, 1.5, 1.5)
        # An absent text fit stays distinguishable from a present default.
        assert info.info.text_fit_width is None
        assert info.info.text_fit_height is style.StyleImageTextFit.PROPORTIONAL

        stretches = _await(map_handle.copy_style_image_stretches("patch"))
        assert stretches is not None
        stretch_x, stretch_y = stretches.stretch_x, stretches.stretch_y
        assert stretch_x == (style.ImageStretch(0.0, 1.0),)
        assert stretch_y == (
            style.ImageStretch(0.0, 1.0),
            style.ImageStretch(1.0, 2.0),
        )
        assert _await(map_handle.copy_style_image_stretches("missing")) is None

        with pytest.raises(mln.InvalidArgumentError, match="positive width"):
            map_handle.set_style_image(
                "bad",
                image,
                style.StyleImageOptions(stretch_x=(style.ImageStretch(2.0, 1.0),)),
            )


def test_style_transition_options_round_trip_public_api() -> None:
    transition_style_json = (
        b'{"version":8,"transition":{"duration":750,"delay":100},'
        b'"sources":{},"layers":[]}'
    )
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        # A map with no style yet reports no duration or delay. The
        # placement flag always reports, because native always holds one.
        empty = _await(map_handle.get_style_transition_options())
        assert empty.duration_ms is None
        assert empty.delay_ms is None
        assert empty.enable_placement_transitions is True

        # The style parser fills in its own 300ms duration for a style that
        # declares no transition.
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        parsed = _await(map_handle.get_style_transition_options())
        assert parsed.duration_ms == 300.0
        assert parsed.delay_ms is None

        map_handle.set_style_json(transition_style_json)
        declared = _await(map_handle.get_style_transition_options())
        assert declared.duration_ms == 750.0
        assert declared.delay_ms == 100.0
        assert declared.enable_placement_transitions is True

        # A present zero stays distinguishable from an absent field, and an
        # absent field clears what the style declared rather than merging.
        options = style.StyleTransitionOptions(
            duration_ms=0.0,
            enable_placement_transitions=False,
        )
        map_handle.set_style_transition_options(options)
        assert _await(map_handle.get_style_transition_options()) == options

        # Omitting the flag leaves the cross-fade on rather than clearing it.
        map_handle.set_style_transition_options(
            style.StyleTransitionOptions(duration_ms=250.0)
        )
        assert (
            _await(
                map_handle.get_style_transition_options()
            ).enable_placement_transitions
            is True
        )

        # Loading a style replaces the override with what that style declares.
        map_handle.set_style_json(transition_style_json)
        assert _await(map_handle.get_style_transition_options()) == declared

        completion = map_handle.set_style_transition_options(
            style.StyleTransitionOptions(delay_ms=-1.0)
        )
        _assert_command_failed(completion, mln.Status.INVALID_ARGUMENT)
        assert _await(map_handle.get_style_transition_options()) == declared


def test_layer_base_accessors_round_trip_public_api() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.set_style_json(
            b'{"version":8,"sources":{"geo":{"type":"geojson","data":'
            b'{"type":"FeatureCollection","features":[]}}},"layers":['
            b'{"id":"bg","type":"background"},'
            b'{"id":"fill","type":"fill","source":"geo"}]}'
        )

        assert _await(map_handle.copy_layer_source_layer("fill")) is None
        map_handle.set_layer_source_layer("fill", "roads")
        assert _await(map_handle.copy_layer_source_layer("fill")) == "roads"
        assert _await(map_handle.copy_layer_source_id("fill")) == "geo"

        # A layer type that takes no source rejects the accepted command.
        completion = map_handle.set_layer_source_layer("bg", "roads")
        _assert_command_failed(completion, mln.Status.INVALID_ARGUMENT)
        assert _await(map_handle.copy_layer_source_id("bg")) is None

        # An unset zoom range crosses the boundary as infinities.
        info = _await(map_handle.get_style_layer_info("fill"))
        assert info is not None
        assert info.info.type == "fill"
        assert info.info.min_zoom == -math.inf
        assert info.info.max_zoom == math.inf
        assert info.info.visibility is style.StyleLayerVisibility.VISIBLE
        # The layer-info string sizes gate the source ID and source-layer
        # copies, which agree with the scalar accessors.
        assert (
            info.source_id == _await(map_handle.copy_layer_source_id("fill")) == "geo"
        )
        assert (
            info.source_layer
            == _await(map_handle.copy_layer_source_layer("fill"))
            == "roads"
        )

        map_handle.set_layer_min_zoom("fill", 4.0)
        map_handle.set_layer_max_zoom("fill", 12.5)
        map_handle.set_layer_visibility("fill", style.StyleLayerVisibility.NONE)
        info = _await(map_handle.get_style_layer_info("fill"))
        assert info is not None
        assert info.info.min_zoom == 4.0
        assert info.info.max_zoom == 12.5
        assert info.info.visibility is style.StyleLayerVisibility.NONE

        # A sourceless layer type reports no source strings at all.
        background = _await(map_handle.get_style_layer_info("bg"))
        assert background is not None
        assert background.info.type == "background"
        assert background.source_id is None
        assert background.source_layer is None

        # The info getter's found flag reports a missing layer as None.
        assert _await(map_handle.get_style_layer_info("missing")) is None


def test_style_layer_metadata_move_and_removal_public_api() -> None:
    style_json = b"""
    {
      "version": 8,
      "sources": {},
      "layers": [
        {"id": "background-a", "type": "background"},
        {"id": "background-b", "type": "background"}
      ]
    }
    """
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.set_style_json(style_json)

        layer_ids = _await(map_handle.list_style_layer_ids())
        assert "background-a" in layer_ids
        assert "background-b" in layer_ids
        assert layer_ids.index("background-a") < layer_ids.index("background-b")
        info = _await(map_handle.get_style_layer_info("background-a"))
        assert info is not None
        assert info.info.type == "background"
        assert _await(map_handle.get_style_layer_info("missing")) is None

        map_handle.move_style_layer("background-b", "background-a")
        layer_ids = _await(map_handle.list_style_layer_ids())
        assert layer_ids.index("background-b") < layer_ids.index("background-a")

        removed_id = map_handle.remove_style_layer("background-b")
        removed = _await_command_completion(removed_id)
        assert removed.disposition == mln.CommandDisposition.COMMITTED
        assert _await(map_handle.get_style_layer_info("background-b")) is None
        assert "background-b" not in _await(map_handle.list_style_layer_ids())

        # Removing the missing layer again fails the command with NOT_FOUND.
        missing_id = map_handle.remove_style_layer("background-b")
        _assert_command_failed(missing_id, mln.Status.NOT_FOUND)


def test_list_style_layers_copies_the_layer_stack_in_style_order() -> None:
    """the layer list carries each layer's type and optional source binding."""
    style_json = b"""
    {
      "version": 8,
      "sources": {
        "tiles": {"type": "vector", "tiles": ["https://example.com/{z}/{x}/{y}.pbf"]}
      },
      "layers": [
        {"id": "roads", "type": "line", "source": "tiles", "source-layer": "transportation"},
        {"id": "background", "type": "background"}
      ]
    }
    """
    with mln.runtime_create() as runtime, _await(runtime.map_create()) as map_handle:
        map_handle.set_style_json(style_json)

        assert _await(map_handle.list_style_layers()) == (
            style.StyleLayerEntry(
                id="roads",
                type="line",
                source_id="tiles",
                source_layer="transportation",
            ),
            style.StyleLayerEntry(id="background", type="background"),
        )


def test_map_viewport_and_tile_options_round_trip_public_values() -> None:
    viewport = map_module.MapViewportOptions(
        north_orientation=map_module.NorthOrientation.RIGHT,
        constrain_mode=map_module.ConstrainMode.WIDTH_AND_HEIGHT,
        viewport_mode=map_module.ViewportMode.DEFAULT,
        frustum_offset=camera.EdgeInsets(top=1.0, left=2.0, bottom=3.0, right=4.0),
    )
    tile = map_module.MapTileOptions(
        prefetch_zoom_delta=1,
        lod_min_radius=1.0,
        lod_scale=1.0,
        lod_pitch_threshold=30.0,
        lod_zoom_shift=0.0,
        lod_mode=map_module.TileLodMode.DEFAULT,
    )

    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        viewport_command = map_handle.set_viewport_options(viewport)
        tile_command = map_handle.set_tile_options(tile)
        runtime.barrier().result(timeout=5)

        # The new snapshot fields round-trip both committed set commands.
        viewport_completion = _await(viewport_command)
        tile_completion = _await(tile_command)
        snapshot = map_handle.snapshot_get()
        assert snapshot.viewport == viewport
        assert snapshot.tile == tile
        assert snapshot.generation >= viewport_completion.generation
        assert snapshot.generation >= tile_completion.generation


def test_camera_snapshot_and_jump_round_trip_public_values() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        target = camera.CameraOptions(
            center=geo.LatLng(10.0, 20.0),
            zoom=2.0,
            bearing=15.0,
            pitch=10.0,
            padding=camera.EdgeInsets(top=1.0, left=2.0, bottom=3.0, right=4.0),
            anchor=camera.ScreenPoint(x=16.0, y=8.0),
        )
        map_handle.update_camera(
            replace(
                mln.CameraUpdate.default(),
                mode=mln.CameraUpdateMode.JUMP,
                camera=target,
                animation=mln.AnimationOptions.default(),
            )
        )
        snapshot = _await(map_handle.camera_query()).camera

        assert snapshot.center is not None
        assert snapshot.center.latitude == pytest.approx(10.0)
        assert snapshot.center.longitude == pytest.approx(20.0)
        assert snapshot.zoom == pytest.approx(2.0)
        assert snapshot.bearing == pytest.approx(15.0)
        assert snapshot.pitch == pytest.approx(10.0)
        assert snapshot.padding == target.padding


def test_published_camera_snapshot_carries_its_generation() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        committed = _await(
            map_handle.update_camera(
                replace(
                    mln.CameraUpdate.default(),
                    mode=mln.CameraUpdateMode.JUMP,
                    camera=camera.CameraOptions(center=geo.LatLng(3.0, 4.0), zoom=5.0),
                    animation=mln.AnimationOptions.default(),
                )
            )
        )

        published = map_handle.camera_snapshot_get()
        assert isinstance(published.camera, camera.CameraOptions)
        # A snapshot at or past the command's generation observes its commit.
        assert published.generation >= committed.generation
        assert published.camera.zoom == pytest.approx(5.0)

        ordered = _await(map_handle.camera_query())
        assert isinstance(ordered, map_module.CameraQueryResult)
        assert ordered.generation >= published.generation
        assert ordered.camera == published.camera


def test_camera_update_carries_its_mode_and_animation() -> None:
    target = camera.CameraOptions(center=geo.LatLng(1.0, 2.0), zoom=3.0)

    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        _drain_runtime_events(runtime)
        _commit(
            map_handle.update_camera(
                replace(
                    camera.CameraUpdate.default(),
                    mode=camera.CameraUpdateMode.EASE,
                    animation=camera.AnimationOptions(
                        duration_ms=0.0, transition_id=909
                    ),
                    camera=target,
                )
            )
        )
        runtime.barrier().result(timeout=5)
        events = _drain_runtime_events(runtime)

        assert _finished_transition_ids(events) == [909]
        assert _await(map_handle.camera_query()).camera.zoom == pytest.approx(3.0)


def test_gesture_phases_bracket_a_camera_update() -> None:
    target = camera.CameraOptions(center=geo.LatLng(0.0, 0.0), zoom=1.0)

    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        assert map_handle.snapshot_get().gesture_in_progress is False

        _commit(
            map_handle.update_camera(
                replace(
                    camera.CameraUpdate.default(),
                    gesture_phase=camera.GesturePhase.BEGIN,
                    camera=target,
                )
            )
        )
        assert map_handle.snapshot_get().gesture_in_progress is True

        _commit(
            map_handle.update_camera(
                replace(
                    camera.CameraUpdate.default(),
                    gesture_phase=camera.GesturePhase.UPDATE,
                    camera=target,
                )
            )
        )
        assert map_handle.snapshot_get().gesture_in_progress is True

        _commit(
            map_handle.update_camera(
                replace(
                    camera.CameraUpdate.default(),
                    gesture_phase=camera.GesturePhase.END,
                    camera=target,
                )
            )
        )
        assert map_handle.snapshot_get().gesture_in_progress is False


def test_cancel_transitions_ends_a_running_transition() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        _drain_runtime_events(runtime)
        _commit(
            map_handle.update_camera(
                replace(
                    mln.CameraUpdate.default(),
                    mode=mln.CameraUpdateMode.EASE,
                    camera=camera.CameraOptions(
                        center=geo.LatLng(30.0, 40.0), zoom=8.0
                    ),
                    animation=camera.AnimationOptions(
                        duration_ms=60_000.0, transition_id=511
                    ),
                )
            )
        )
        runtime.barrier().result(timeout=5)
        # The transition is still running: nothing reports its end yet.
        assert _finished_transition_ids(_drain_runtime_events(runtime)) == []

        _commit(map_handle.cancel_transitions())
        runtime.barrier().result(timeout=5)

        # Cancelling reports the transition's end once, as completing it does.
        assert _finished_transition_ids(_drain_runtime_events(runtime)) == [511]
        settled = _await(map_handle.camera_query()).camera
        assert settled.zoom is not None
        assert settled.zoom < 8.0


def test_gesture_cancel_phase_ends_a_running_transition() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        _drain_runtime_events(runtime)
        _commit(
            map_handle.update_camera(
                replace(
                    mln.CameraUpdate.default(),
                    mode=mln.CameraUpdateMode.EASE,
                    camera=camera.CameraOptions(
                        center=geo.LatLng(10.0, 20.0), zoom=6.0
                    ),
                    animation=camera.AnimationOptions(
                        duration_ms=60_000.0, transition_id=512
                    ),
                )
            )
        )
        _commit(
            map_handle.update_camera(
                replace(
                    camera.CameraUpdate.default(),
                    gesture_phase=camera.GesturePhase.CANCEL,
                    camera=camera.CameraOptions(),
                )
            )
        )
        runtime.barrier().result(timeout=5)

        assert _finished_transition_ids(_drain_runtime_events(runtime)) == [512]
        assert map_handle.snapshot_get().gesture_in_progress is False


@pytest.mark.parametrize("kind", list(camera.CameraDeltaKind))
def test_apply_camera_delta_commits_every_kind(
    kind: camera.CameraDeltaKind,
) -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        _commit(
            map_handle.update_camera(
                replace(
                    mln.CameraUpdate.default(),
                    mode=mln.CameraUpdateMode.JUMP,
                    camera=camera.CameraOptions(
                        center=geo.LatLng(0.0, 0.0), zoom=4.0, bearing=0.0, pitch=0.0
                    ),
                    animation=mln.AnimationOptions.default(),
                )
            )
        )
        before = _await(map_handle.camera_query()).camera

        _commit(
            map_handle.apply_camera_delta(
                replace(
                    camera.CameraDelta.default(),
                    kind=kind,
                    offset=camera.ScreenPoint(8.0, 4.0),
                    amount=2.0,
                )
            )
        )
        after = _await(map_handle.camera_query()).camera

        assert before.center is not None
        assert after.center is not None
        if kind is camera.CameraDeltaKind.MOVE:
            assert after.center != before.center
        elif kind is camera.CameraDeltaKind.SCALE:
            assert after.zoom != before.zoom
        elif kind is camera.CameraDeltaKind.BEARING:
            assert after.bearing != before.bearing
        else:
            # A pitch delta adds to the current pitch.
            assert after.pitch == pytest.approx(2.0)


def test_map_resize_supersedes_the_command_it_replaces() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        # Resizes coalesce: a resize still pending on the map worker is
        # replaced by a later one, which reports SUPERSEDED for the replaced
        # command. How many submissions the worker takes at once is a race, so
        # this submits batches until one of them coalesces.
        for attempt in range(64):
            width, height = 200 + attempt, 100 + attempt
            batch = [
                map_handle.resize(mln.LogicalExtent(width, height, 1.0))
                for _ in range(16)
            ]
            dispositions = [_await(command).disposition for command in batch]
            if mln.CommandDisposition.SUPERSEDED in dispositions:
                break
        else:
            raise AssertionError("coalescing resizes never superseded one another")

        # The command that ran last is the one that took effect.
        assert dispositions[-1] == mln.CommandDisposition.COMMITTED
        snapshot = map_handle.snapshot_get()
        assert (snapshot.logical_extent.width, snapshot.logical_extent.height) == (
            width,
            height,
        )


def test_map_resize_rejects_another_scale_factor() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create(
            replace(
                mln.MapOptions.default(), initial_extent=mln.LogicalExtent(64, 32, 1.0)
            )
        ).result(timeout=5) as map_handle,
    ):
        with pytest.raises(mln.InvalidArgumentError) as raised:
            map_handle.resize(mln.LogicalExtent(64, 32, 2.0))
        assert raised.value.status == mln.Status.INVALID_ARGUMENT

        # The rejected call changed nothing, and the map still resizes.
        _commit(map_handle.resize(mln.LogicalExtent(48, 24, 1.0)))
        snapshot = map_handle.snapshot_get()
        assert (
            snapshot.logical_extent.width,
            snapshot.logical_extent.height,
            snapshot.logical_extent.scale_factor,
        ) == (
            48,
            24,
            pytest.approx(1.0),
        )


def test_map_close_cancels_an_operation_it_did_not_finish() -> None:
    with mln.runtime_create() as runtime:
        map_handle = runtime.map_create(
            replace(mln.MapOptions.default(), map_mode=mln.MapMode.STATIC)
        ).result(timeout=5)
        _commit(map_handle.set_style_json(_EMPTY_STYLE_BYTES))

        # A still image needs a render session to finish, and this map has
        # none, so the request is still outstanding when the map retires.
        still_image = map_handle.request_still_image()
        teardown = map_handle.close()

        with pytest.raises(mln.CancelledError) as raised:
            _await(still_image)
        assert raised.value.status == mln.Status.CANCELLED
        _await(teardown)


def test_free_camera_and_projection_mode_round_trip_public_values() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        free_camera = map_handle.snapshot_get().free_camera
        assert isinstance(free_camera, camera.FreeCameraOptions)
        map_handle.set_free_camera_options(
            camera.FreeCameraOptions(orientation=camera.Quaternion(0.0, 0.0, 0.0, 1.0))
        )
        runtime.barrier().result(timeout=5)
        updated = map_handle.snapshot_get().free_camera
        assert updated.position is not None
        assert updated.orientation is not None

        projection = camera.ProjectionMode(
            axonometric=True,
            x_skew=0.1,
            y_skew=0.2,
        )
        map_handle.set_projection_mode(projection)
        runtime.barrier().result(timeout=5)
        snapshot = map_handle.snapshot_get().projection_mode

        assert snapshot.axonometric is True
        assert snapshot.x_skew == pytest.approx(0.1)
        assert snapshot.y_skew == pytest.approx(0.2)


def test_camera_fit_bounds_and_constraints_public_api() -> None:
    bounds = geo.LatLngBounds(
        southwest=geo.LatLng(-1.0, -1.0),
        northeast=geo.LatLng(1.0, 1.0),
    )
    fit = camera.CameraFitOptions(
        padding=camera.EdgeInsets(1.0, 2.0, 3.0, 4.0),
        bearing=0.0,
        pitch=0.0,
    )
    target = camera.CameraOptions(center=geo.LatLng(0.0, 0.0), zoom=1.0)

    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.set_bounds(
            camera.BoundOptions(
                bounds=bounds,
                min_zoom=0.0,
                max_zoom=10.0,
            )
        )
        runtime.barrier().result(timeout=5)
        constraints = map_handle.snapshot_get().bounds
        fit_bounds = _await(map_handle.camera_for_lat_lng_bounds(bounds, fit))
        fit_coordinates = _await(
            map_handle.camera_for_lat_lngs(
                (bounds.southwest, bounds.northeast),
                fit,
            )
        )
        fit_geometry = _await(
            map_handle.camera_for_geometry(
                _json_object(
                    {
                        "type": "LineString",
                        "coordinates": [
                            [bounds.southwest.longitude, bounds.southwest.latitude],
                            [bounds.northeast.longitude, bounds.northeast.latitude],
                        ],
                    }
                ),
                fit,
            )
        )
        visible_bounds = _await(map_handle.lat_lng_bounds_for_camera(target))
        unwrapped_bounds = _await(
            map_handle.lat_lng_bounds_for_camera_unwrapped(
                target,
            )
        )

        assert constraints.bounds == bounds
        assert constraints.min_zoom == pytest.approx(0.0)
        assert constraints.max_zoom == pytest.approx(10.0)
        assert isinstance(fit_bounds, camera.CameraOptions)
        assert isinstance(fit_coordinates, camera.CameraOptions)
        assert isinstance(fit_geometry, camera.CameraOptions)
        assert isinstance(visible_bounds, geo.LatLngBounds)
        assert isinstance(unwrapped_bounds, geo.LatLngBounds)


def _jumped_longitude(map_handle: mln.MapHandle, longitude: float) -> float:
    map_handle.update_camera(
        replace(
            mln.CameraUpdate.default(),
            mode=mln.CameraUpdateMode.JUMP,
            camera=camera.CameraOptions(center=geo.LatLng(0.0, longitude), zoom=2.0),
            animation=mln.AnimationOptions.default(),
        )
    )
    center = _await(map_handle.camera_query()).camera.center
    assert center is not None
    return center.longitude


def _settled_bounds(
    runtime: mln.RuntimeHandle, map_handle: mln.MapHandle
) -> camera.BoundsConstraint | None:
    runtime.barrier().result(timeout=5)
    return map_handle.snapshot_get().bounds.bounds


def test_camera_bounds_distinguish_unbounded_from_world() -> None:
    world = geo.LatLngBounds(
        southwest=geo.LatLng(-90.0, -180.0),
        northeast=geo.LatLng(90.0, 180.0),
    )

    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        assert _settled_bounds(runtime, map_handle) is None
        # An unbounded map wraps across the antimeridian.
        assert _jumped_longitude(map_handle, 200.0) == pytest.approx(-160.0, abs=1e-6)

        map_handle.set_bounds(camera.BoundOptions(bounds=world))

        constrained = _settled_bounds(runtime, map_handle)
        assert isinstance(constrained, geo.LatLngBounds)
        assert constrained.northeast.longitude == pytest.approx(180.0)
        # World bounds clamp at the antimeridian instead of wrapping.
        assert _jumped_longitude(map_handle, 200.0) == pytest.approx(180.0, abs=1e-6)

        map_handle.set_bounds(camera.BoundOptions(unbounded=True))

        assert _settled_bounds(runtime, map_handle) is None
        # Releasing the constraint restores antimeridian wrapping.
        assert _jumped_longitude(map_handle, 200.0) == pytest.approx(-160.0, abs=1e-6)


def test_camera_transition_commands_accept_public_values() -> None:
    animation = camera.AnimationOptions(
        duration_ms=0.0,
        velocity=1.0,
        min_zoom=0.0,
        easing=camera.UnitBezier(0.0, 0.0, 1.0, 1.0),
    )
    target = camera.CameraOptions(center=geo.LatLng(0.0, 0.0), zoom=1.0)
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        assert (
            _await(
                map_handle.update_camera(
                    replace(
                        mln.CameraUpdate.default(),
                        mode=mln.CameraUpdateMode.EASE,
                        camera=target,
                        animation=animation,
                    )
                )
            ).generation
            > 0
        )
        assert (
            _await(
                map_handle.update_camera(
                    replace(
                        mln.CameraUpdate.default(),
                        mode=mln.CameraUpdateMode.FLY,
                        camera=target,
                        animation=animation,
                    )
                )
            ).generation
            > 0
        )


def _drain_runtime_events(runtime: mln.RuntimeHandle) -> list[mln.RuntimeEvent]:
    return drain_events(runtime).events


def _finished_transition_ids(events: list[mln.RuntimeEvent]) -> list[int]:
    ids: list[int] = []
    for event in events:
        if event.type != mln.RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED:
            continue
        assert isinstance(
            event.payload, mln.RuntimeEventCameraTransitionFinishedVariant
        )
        ids.append(event.payload.value.transition_id)
    return ids


def _await_command_completion(
    future: Future[mln.CommandCompletion],
) -> mln.CommandCompletion:
    return _await(future)


def _camera_change_modes(
    events: list[mln.RuntimeEvent], event_type: mln.RuntimeEventType
) -> list[mln.CameraChangeMode]:
    return [
        mln.CameraChangeMode(event.code) for event in events if event.type == event_type
    ]


def test_zero_duration_ease_reports_transition_finished_once() -> None:
    target = camera.CameraOptions(center=geo.LatLng(12.0, 34.0), zoom=4.0)

    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        _drain_runtime_events(runtime)
        _await(
            map_handle.update_camera(
                replace(
                    mln.CameraUpdate.default(),
                    mode=mln.CameraUpdateMode.EASE,
                    camera=target,
                    animation=camera.AnimationOptions(
                        duration_ms=0.0, transition_id=101
                    ),
                )
            )
        )
        runtime.barrier().result(timeout=5)
        events = _drain_runtime_events(runtime)

    assert _finished_transition_ids(events) == [101]
    assert _camera_change_modes(events, mln.RuntimeEventType.MAP_CAMERA_DID_CHANGE) == [
        mln.CameraChangeMode.IMMEDIATE
    ]


def test_superseded_transition_reports_transition_finished_once() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        _drain_runtime_events(runtime)
        _await(
            map_handle.update_camera(
                replace(
                    mln.CameraUpdate.default(),
                    mode=mln.CameraUpdateMode.EASE,
                    camera=camera.CameraOptions(
                        center=geo.LatLng(20.0, 40.0), zoom=6.0
                    ),
                    animation=camera.AnimationOptions(
                        duration_ms=5_000.0, transition_id=201
                    ),
                )
            )
        )
        runtime.barrier().result(timeout=5)
        started = _drain_runtime_events(runtime)
        _await(
            map_handle.update_camera(
                replace(
                    mln.CameraUpdate.default(),
                    mode=mln.CameraUpdateMode.JUMP,
                    camera=camera.CameraOptions(
                        center=geo.LatLng(-20.0, -40.0), zoom=2.0
                    ),
                    animation=mln.AnimationOptions.default(),
                )
            )
        )
        runtime.barrier().result(timeout=5)
        superseded = _drain_runtime_events(runtime)

    assert _finished_transition_ids(started) == []
    assert _finished_transition_ids(superseded) == [201]


def test_completed_ease_reports_transition_finished_once() -> None:
    target = camera.CameraOptions(center=geo.LatLng(5.0, 10.0), zoom=3.0)
    deadline = time.monotonic() + 10.0
    events: list[mln.RuntimeEvent] = []

    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        _drain_runtime_events(runtime)
        _await(
            map_handle.update_camera(
                replace(
                    mln.CameraUpdate.default(),
                    mode=mln.CameraUpdateMode.EASE,
                    camera=target,
                    animation=camera.AnimationOptions(
                        duration_ms=20.0, transition_id=401
                    ),
                )
            )
        )
        runtime.barrier().result(timeout=5)
        while not _finished_transition_ids(events):
            assert time.monotonic() < deadline, (
                "ease did not finish under autonomous runtime execution"
            )
            _await(map_handle.request_repaint())
            runtime.barrier().result(timeout=5)
            time.sleep(0.001)
            events.extend(_drain_runtime_events(runtime))

        trailing: list[mln.RuntimeEvent] = []
        for _ in range(8):
            _await(map_handle.request_repaint())
            runtime.barrier().result(timeout=5)
            time.sleep(0.001)
            trailing.extend(_drain_runtime_events(runtime))

    assert _finished_transition_ids(events) == [401]
    assert _finished_transition_ids(trailing) == []
    assert mln.CameraChangeMode.ANIMATED in _camera_change_modes(
        events, mln.RuntimeEventType.MAP_CAMERA_DID_CHANGE
    )


def test_camera_change_events_report_immediate_modes_for_jump_and_zero_duration_ease() -> (
    None
):
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        _drain_runtime_events(runtime)
        _await(
            map_handle.update_camera(
                replace(
                    mln.CameraUpdate.default(),
                    mode=mln.CameraUpdateMode.JUMP,
                    camera=camera.CameraOptions(center=geo.LatLng(1.0, 2.0), zoom=3.0),
                    animation=mln.AnimationOptions.default(),
                )
            )
        )
        runtime.barrier().result(timeout=5)
        jumped = _drain_runtime_events(runtime)
        _await(
            map_handle.update_camera(
                replace(
                    mln.CameraUpdate.default(),
                    mode=mln.CameraUpdateMode.EASE,
                    camera=camera.CameraOptions(
                        center=geo.LatLng(-1.0, -2.0), zoom=7.0
                    ),
                    animation=camera.AnimationOptions(duration_ms=0.0),
                )
            )
        )
        runtime.barrier().result(timeout=5)
        eased = _drain_runtime_events(runtime)

    will_change = mln.RuntimeEventType.MAP_CAMERA_WILL_CHANGE
    did_change = mln.RuntimeEventType.MAP_CAMERA_DID_CHANGE
    assert _camera_change_modes(jumped, will_change) == [mln.CameraChangeMode.IMMEDIATE]
    assert _camera_change_modes(jumped, did_change) == [mln.CameraChangeMode.IMMEDIATE]
    assert _camera_change_modes(eased, will_change) == [mln.CameraChangeMode.IMMEDIATE]
    assert _camera_change_modes(eased, did_change) == [mln.CameraChangeMode.IMMEDIATE]


def test_transition_finished_precedes_camera_did_change_in_one_batch() -> None:
    """one drain reports more than one event and preserves queue order."""
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        drain_events(runtime)
        _await(
            map_handle.update_camera(
                replace(
                    mln.CameraUpdate.default(),
                    mode=mln.CameraUpdateMode.EASE,
                    camera=camera.CameraOptions(
                        center=geo.LatLng(12.0, 34.0), zoom=4.0
                    ),
                    animation=camera.AnimationOptions(
                        duration_ms=0.0, transition_id=707
                    ),
                )
            )
        )
        runtime.barrier().result(timeout=5)
        types = [event.type for event in drain_events(runtime).events]

    finished = mln.RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED
    assert types.count(finished) == 1
    assert types.index(finished) < types.index(
        mln.RuntimeEventType.MAP_CAMERA_DID_CHANGE
    )


def test_narrowed_map_mask_drops_the_cleared_event_type() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        _await(
            map_handle.set_event_mask(
                mln.RuntimeEventMask.ALL & ~mln.RuntimeEventMask.MAP_CAMERA_DID_CHANGE
            )
        )
        runtime.barrier().result(timeout=5)
        drain_events(runtime)
        _await(
            map_handle.update_camera(
                replace(
                    mln.CameraUpdate.default(),
                    mode=mln.CameraUpdateMode.JUMP,
                    camera=camera.CameraOptions(center=geo.LatLng(1.0, 2.0), zoom=3.0),
                    animation=mln.AnimationOptions.default(),
                )
            )
        )
        runtime.barrier().result(timeout=5)
        types = [event.type for event in drain_events(runtime).events]

    assert mln.RuntimeEventType.MAP_CAMERA_WILL_CHANGE in types
    assert mln.RuntimeEventType.MAP_CAMERA_DID_CHANGE not in types


def test_map_created_with_a_narrowed_mask_never_delivers_the_cleared_type() -> None:
    narrowed = mln.RuntimeEventMask.ALL & ~mln.RuntimeEventMask.MAP_CAMERA_DID_CHANGE
    options = replace(mln.MapOptions.default(), event_mask=narrowed)

    with (
        mln.runtime_create() as runtime,
        runtime.map_create(options).result(timeout=5) as map_handle,
    ):
        assert map_handle.snapshot_get().event_mask == narrowed
        drain_events(runtime)
        _await(
            map_handle.update_camera(
                replace(
                    mln.CameraUpdate.default(),
                    mode=mln.CameraUpdateMode.JUMP,
                    camera=camera.CameraOptions(center=geo.LatLng(1.0, 2.0), zoom=3.0),
                    animation=mln.AnimationOptions.default(),
                )
            )
        )
        runtime.barrier().result(timeout=5)
        types = [event.type for event in drain_events(runtime).events]

    assert mln.RuntimeEventType.MAP_CAMERA_WILL_CHANGE in types
    assert mln.RuntimeEventType.MAP_CAMERA_DID_CHANGE not in types


def test_event_masks_round_trip_on_the_map_and_the_runtime() -> None:
    created = mln.RuntimeEventMask.ALL & ~(
        mln.RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED
    )

    with mln.runtime_create(
        replace(mln.RuntimeOptions.default(), event_mask=created)
    ) as runtime:
        assert runtime.get_event_mask() == created
        runtime.set_event_mask(mln.RuntimeEventMask.ALL)
        runtime.barrier().result(timeout=5)
        assert runtime.get_event_mask() == mln.RuntimeEventMask.ALL

        with runtime.map_create().result(timeout=5) as map_handle:
            map_handle.set_event_mask(mln.RuntimeEventMask.ALL)
            runtime.barrier().result(timeout=5)
            assert map_handle.snapshot_get().event_mask == mln.RuntimeEventMask.ALL

            # A read-modify-write of one bit keeps every other bit, including the
            # runtime bits a map ignores.
            map_handle.set_event_mask(
                map_handle.snapshot_get().event_mask & ~mln.RuntimeEventMask.MAP_IDLE
            )
            runtime.barrier().result(timeout=5)
            assert (
                map_handle.snapshot_get().event_mask
                == mln.RuntimeEventMask.ALL & ~mln.RuntimeEventMask.MAP_IDLE
            )
            assert (
                mln.RuntimeEventMask.MAP_TILE_ACTION
                in map_handle.snapshot_get().event_mask
            )
            assert (
                mln.RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED
                in map_handle.snapshot_get().event_mask
            )

            runtime.set_event_mask(
                runtime.get_event_mask()
                & ~mln.RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED
            )
            runtime.barrier().result(timeout=5)
            assert (
                runtime.get_event_mask()
                == mln.RuntimeEventMask.ALL
                & ~mln.RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED
            )


def test_empty_creation_mask_queues_nothing() -> None:
    # An empty mask is a selection of no types, not an absent selection.
    options = replace(
        mln.RuntimeOptions.default(), event_mask=mln.RuntimeEventMask.NONE
    )
    map_options = replace(
        mln.MapOptions.default(), event_mask=mln.RuntimeEventMask.NONE
    )

    with mln.runtime_create(options) as runtime:
        assert runtime.get_event_mask() == mln.RuntimeEventMask.NONE
        with runtime.map_create(map_options).result(timeout=5) as map_handle:
            assert map_handle.snapshot_get().event_mask == mln.RuntimeEventMask.NONE
            assert not drain_events(runtime).events
            _await(map_handle.set_style_json(_EMPTY_STYLE_BYTES))
            runtime.barrier().result(timeout=5)

            assert not drain_events(runtime).events


def test_event_mask_bit_outside_all_is_rejected() -> None:
    outside = mln.RuntimeEventMask(1 << 40)

    with pytest.raises(mln.InvalidArgumentError):
        mln.runtime_create(replace(mln.RuntimeOptions.default(), event_mask=outside))

    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        with pytest.raises(mln.InvalidArgumentError):
            runtime.map_create(
                replace(mln.MapOptions.default(), event_mask=outside)
            ).result(timeout=5)
        with pytest.raises(mln.InvalidArgumentError):
            runtime.set_event_mask(outside)
        with pytest.raises(mln.InvalidArgumentError):
            _await(map_handle.set_event_mask(outside))


def test_drained_events_stay_equal_after_the_next_drain_and_map_close() -> None:
    with mln.runtime_create() as runtime:
        map_handle = runtime.map_create().result(timeout=5)
        map_handle.update_camera(
            replace(
                mln.CameraUpdate.default(),
                mode=mln.CameraUpdateMode.JUMP,
                camera=camera.CameraOptions(center=geo.LatLng(1.0, 2.0), zoom=3.0),
                animation=mln.AnimationOptions.default(),
            )
        )
        runtime.barrier().result(timeout=5)
        first = drain_events(runtime)
        snapshot = first.events
        assert snapshot

        map_handle.update_camera(
            replace(
                mln.CameraUpdate.default(),
                mode=mln.CameraUpdateMode.JUMP,
                camera=camera.CameraOptions(center=geo.LatLng(4.0, 5.0), zoom=6.0),
                animation=mln.AnimationOptions.default(),
            )
        )
        runtime.barrier().result(timeout=5)
        assert drain_events(runtime).events

        assert first.events == snapshot
        map_handle.close()
        assert first.events == snapshot


def test_drain_returns_a_copied_map_loading_failure() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        completion = map_handle.set_style_json(b"{")
        _assert_command_failed(completion, mln.Status.NATIVE_ERROR)
        events = drain_events(runtime).events
        failures = [
            event
            for event in events
            if event.type == mln.RuntimeEventType.MAP_LOADING_FAILED
        ]

        assert failures
        loading_failed = failures[0]
        assert loading_failed.source_type == mln.RuntimeEventSourceType.MAP
        assert loading_failed.source == map_handle.id
        assert loading_failed.message


def test_autonomous_runtime_and_drain_return_a_copied_style_loaded_event() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        style_loaded = _wait_for_runtime_event(
            runtime, mln.RuntimeEventType.MAP_STYLE_LOADED
        )

        assert style_loaded.source_type == mln.RuntimeEventSourceType.MAP
        assert style_loaded.source == map_handle.id
        assert style_loaded.payload is None


def test_map_feature_state_set_get_and_remove() -> None:
    selector = query.FeatureStateSelector(
        source_id="points",
        source_layer_id="symbols",
        feature_id="feature-1",
        state_key="hover",
    )
    state = _json_object({"hover": True})
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        _await(map_handle.set_feature_state(selector, state))
        returned = _await(map_handle.get_feature_state(selector))
        _await(map_handle.remove_feature_state(selector))
        empty = _await(
            map_handle.get_feature_state(
                query.FeatureStateSelector(
                    source_id="points",
                    source_layer_id="symbols",
                    feature_id="feature-1",
                )
            )
        )

    assert json.loads(returned) == {"hover": True}
    assert json.loads(empty) == {}


def test_invalid_render_target_attach_reports_native_status() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        with pytest.raises(
            (mln.InvalidArgumentError, mln.UnsupportedFeatureError)
        ) as raised:
            map_handle.metal_owned_texture_attach(
                render.MetalOwnedTextureDescriptor.default(),
                replace(
                    mln.RenderSessionAttachOptions.default(),
                    driver=mln.RenderDriverKind.CORE_WORKER,
                ),
            )

        assert raised.value.status in {
            mln.Status.INVALID_ARGUMENT,
            mln.Status.UNSUPPORTED,
        }


def test_invalid_opengl_render_target_attach_reports_native_status() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        with pytest.raises(
            (mln.InvalidArgumentError, mln.UnsupportedFeatureError)
        ) as raised:
            map_handle.opengl_owned_texture_attach(
                render.OpenglOwnedTextureDescriptor.default()
            )

        assert raised.value.status in {
            mln.Status.INVALID_ARGUMENT,
            mln.Status.UNSUPPORTED,
        }


def test_invalid_webgpu_render_target_attach_reports_native_status() -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        with pytest.raises(
            (mln.InvalidArgumentError, mln.UnsupportedFeatureError)
        ) as raised:
            map_handle.webgpu_owned_texture_attach(
                render.WebgpuOwnedTextureDescriptor.default()
            )

        assert raised.value.status in {
            mln.Status.INVALID_ARGUMENT,
            mln.Status.UNSUPPORTED,
        }


def test_map_coordinate_conversions_round_trip_public_values() -> None:
    coordinate = geo.LatLng(0.0, 0.0)
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.update_camera(
            replace(
                mln.CameraUpdate.default(),
                mode=mln.CameraUpdateMode.JUMP,
                camera=camera.CameraOptions(center=coordinate, zoom=1.0),
                animation=mln.AnimationOptions.default(),
            )
        )
        point = _await(map_handle.pixel_for_lat_lng(coordinate))
        projected = _await(map_handle.lat_lng_for_pixel(point))
        points = _await(
            map_handle.pixels_for_lat_lngs((coordinate, geo.LatLng(1.0, 1.0)))
        )
        coordinates = _await(map_handle.lat_lngs_for_pixels(points))

        assert isinstance(point, camera.ScreenPoint)
        assert math.isfinite(projected.latitude)
        assert math.isfinite(projected.longitude)
        assert len(points) == 2
        assert len(coordinates) == 2
        assert all(isinstance(item, camera.ScreenPoint) for item in points)
        assert all(isinstance(item, geo.LatLng) for item in coordinates)


def test_unwrapped_coordinate_conversions_preserve_visible_world_copies() -> None:
    options = replace(
        map_module.MapOptions.default(),
        initial_extent=mln.LogicalExtent(1024, 512, 1.0),
    )
    with (
        mln.runtime_create() as runtime,
        runtime.map_create(options).result(timeout=5) as map_handle,
    ):
        _await_command_completion(
            map_handle.update_camera(
                replace(
                    mln.CameraUpdate.default(),
                    mode=mln.CameraUpdateMode.JUMP,
                    camera=camera.CameraOptions(
                        center=geo.LatLng(0.0, 180.0), zoom=0.0
                    ),
                    animation=mln.AnimationOptions.default(),
                )
            ),
        )
        points = (camera.ScreenPoint(0.0, 256.0), camera.ScreenPoint(1024.0, 256.0))

        wrapped = _await(map_handle.lat_lngs_for_pixels(points))
        unwrapped = _await(map_handle.lat_lngs_for_pixels_unwrapped(points))
        assert all(-180.0 <= coordinate.longitude <= 180.0 for coordinate in wrapped)
        assert unwrapped[1].longitude - unwrapped[0].longitude > 360.0
        assert (
            -180.0 <= _await(map_handle.lat_lng_for_pixel(points[1])).longitude <= 180.0
        )
        right = _await(map_handle.lat_lng_for_pixel_unwrapped(points[1]))
        assert math.isclose(right.longitude, unwrapped[1].longitude, abs_tol=1e-10)

        with _await(map_handle.projection_create()) as projection:
            assert -180.0 <= projection.lat_lng_for_pixel(points[1]).longitude <= 180.0
            projected_right = projection.lat_lng_for_pixel_unwrapped(points[1])
            assert math.isclose(
                projected_right.longitude, right.longitude, abs_tol=1e-10
            )


def test_meters_per_pixel_matches_projection_and_follows_zoom() -> None:
    with mln.runtime_create() as runtime, _await(runtime.map_create()) as map_handle:
        _await(
            map_handle.update_camera(
                replace(
                    camera.CameraUpdate.default(),
                    camera=camera.CameraOptions(center=geo.LatLng(0.0, 0.0), zoom=3.0),
                )
            )
        )
        meters = _await(map_handle.meters_per_pixel_at_latitude(45.0))

        with _await(map_handle.projection_create()) as projection:
            assert projection.meters_per_pixel_at_latitude(45.0) == pytest.approx(
                meters
            )

        _await(
            map_handle.update_camera(
                replace(
                    camera.CameraUpdate.default(), camera=camera.CameraOptions(zoom=4.0)
                )
            )
        )
        assert _await(map_handle.meters_per_pixel_at_latitude(45.0)) == pytest.approx(
            meters / 2.0
        )
        with pytest.raises(mln.InvalidArgumentError):
            map_handle.meters_per_pixel_at_latitude(91.0)


def test_map_projection_converts_coordinates_and_closes() -> None:
    coordinate = geo.LatLng(0.0, 0.0)
    meters = map_module.projected_meters_for_lat_lng(coordinate)
    round_tripped = map_module.lat_lng_for_projected_meters(meters)

    assert isinstance(meters, map_module.ProjectedMeters)
    assert math.isclose(round_tripped.latitude, coordinate.latitude, abs_tol=1e-6)
    assert math.isclose(round_tripped.longitude, coordinate.longitude, abs_tol=1e-6)

    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
    ):
        map_handle.update_camera(
            replace(
                mln.CameraUpdate.default(),
                mode=mln.CameraUpdateMode.JUMP,
                camera=camera.CameraOptions(center=geo.LatLng(10.0, 20.0), zoom=3.0),
                animation=mln.AnimationOptions.default(),
            )
        )
        with _await(map_handle.projection_create()) as projection:
            # A projection created after a camera command observes it.
            created_camera = projection.get_camera()
            assert created_camera.center is not None
            assert created_camera.center.latitude == pytest.approx(10.0)
            assert created_camera.center.longitude == pytest.approx(20.0)
            assert created_camera.zoom == pytest.approx(3.0)

            # A synchronous conversion round-trips pixel -> latlng -> pixel.
            point = projection.pixel_for_lat_lng(coordinate)
            projected = projection.lat_lng_for_pixel(point)
            replayed = projection.pixel_for_lat_lng(projected)
            assert math.isclose(projected.latitude, coordinate.latitude, abs_tol=1e-6)
            assert math.isclose(projected.longitude, coordinate.longitude, abs_tol=1e-6)
            assert math.isclose(replayed.x, point.x, abs_tol=1e-6)
            assert math.isclose(replayed.y, point.y, abs_tol=1e-6)

            # A setter applies before returning, so it changes later
            # conversions.
            projection.set_camera(camera.CameraOptions(center=coordinate, zoom=2.0))
            recentered = projection.pixel_for_lat_lng(coordinate)
            assert (recentered.x, recentered.y) != (point.x, point.y)
            moved_camera = projection.get_camera()
            assert moved_camera.center is not None
            assert moved_camera.center.latitude == pytest.approx(0.0)
            assert moved_camera.zoom == pytest.approx(2.0)

            # The projection is usable from a second Python thread.
            results: list[geo.LatLng] = []
            thread = threading.Thread(
                target=lambda: results.append(projection.lat_lng_for_pixel(recentered))
            )
            thread.start()
            thread.join()
            assert len(results) == 1
            assert math.isclose(results[0].latitude, coordinate.latitude, abs_tol=1e-6)
            assert math.isclose(
                results[0].longitude, coordinate.longitude, abs_tol=1e-6
            )

            projection.set_visible_coordinates(
                (geo.LatLng(-1.0, -1.0), geo.LatLng(1.0, 1.0)),
                camera.EdgeInsets(0, 0, 0, 0),
            )
            projection.set_visible_geometry(
                _json_object(
                    {
                        "type": "LineString",
                        "coordinates": [[-1.0, -1.0], [1.0, 1.0]],
                    }
                ),
                camera.EdgeInsets(0, 0, 0, 0),
            )
            assert not projection.closed
            assert isinstance(projection.get_camera(), camera.CameraOptions)

        # The close is synchronous, so the handle is retired when it returns
        # and a later call is rejected.
        assert projection.closed
        with pytest.raises(mln.InvalidStateError):
            projection.get_camera()


def test_map_projection_outlives_its_source_handles() -> None:
    runtime = mln.runtime_create()
    map_handle = runtime.map_create().result(timeout=5)
    projection = _await(map_handle.projection_create())

    map_handle.close()
    runtime.close()

    assert isinstance(projection.get_camera(), camera.CameraOptions)
    projection.close()


def test_map_projection_remains_usable_on_another_thread_after_map_close() -> None:
    runtime = mln.runtime_create()
    map_handle = runtime.map_create().result(timeout=5)
    projection = _await(map_handle.projection_create())
    map_handle.close()
    runtime.close()

    failures: list[BaseException] = []

    def use_and_close_projection() -> None:
        try:
            assert isinstance(projection.get_camera(), camera.CameraOptions)
            projection.close()
        except BaseException as error:  # noqa: BLE001 - reported by the test thread
            failures.append(error)

    thread = threading.Thread(target=use_and_close_projection)
    thread.start()
    thread.join()

    assert not failures
    assert projection.closed


def test_map_projection_closes_while_another_thread_projects() -> None:
    runtime = mln.runtime_create()
    map_handle = runtime.map_create().result(timeout=5)
    projection = _await(map_handle.projection_create())
    point = camera.ScreenPoint(8.0, 8.0)

    projecting = threading.Event()
    failures: list[BaseException] = []

    def project_until_closed() -> None:
        try:
            deadline = time.monotonic() + 10.0
            while time.monotonic() < deadline:
                try:
                    projection.lat_lng_for_pixel(point)
                except mln.InvalidArgumentError, mln.InvalidStateError:
                    return
                projecting.set()
            failures.append(AssertionError("the projection never reported its close"))
        except BaseException as error:  # noqa: BLE001 - reported by the test thread
            failures.append(error)

    thread = threading.Thread(target=project_until_closed)
    thread.start()
    try:
        assert projecting.wait(timeout=5)
        # The close serializes behind the calls already running on the other
        # thread. Neither side may hold the GIL waiting on the other.
        projection.close()
    finally:
        thread.join(timeout=15)

    assert not thread.is_alive()
    assert not failures
    assert projection.closed

    map_handle.close()
    runtime.close()


def test_offline_futures_complete_with_public_values(tmp_path: Path) -> None:
    definition = mln.OfflineRegionDefinition(
        mln.OfflineRegionDefinitionTilePyramidVariant(
            mln.OfflineTilePyramidRegionDefinition(
                style_url="https://example.test/style.json",
                bounds=geo.LatLngBounds(geo.LatLng(-1.0, -2.0), geo.LatLng(1.0, 2.0)),
                min_zoom=0.0,
                max_zoom=1.0,
                pixel_ratio=1.0,
                include_ideographs=False,
            )
        )
    )
    with mln.runtime_create(
        replace(mln.RuntimeOptions.default(), cache_path=str(tmp_path / "cache.db"))
    ) as runtime:
        created = _await(runtime.offline_region_create(definition, b"metadata"))
        assert created.metadata == b"metadata"
        assert _await(runtime.offline_region_get(created.id)) == created
        assert created in _await(runtime.offline_regions_list())
        assert (
            _await(runtime.offline_region_get_status(created.id)).download_state
            == offline.OfflineRegionDownloadState.INACTIVE
        )
        assert _await(runtime.offline_region_set_observed(created.id, False)) is None
        assert _await(runtime.offline_region_delete(created.id)) is None


@pytest.mark.parametrize(
    "start",
    [
        pytest.param(
            lambda runtime, region_id: runtime.offline_region_get_status(region_id),
            id="get_offline_region_status",
        ),
        pytest.param(
            lambda runtime, region_id: runtime.offline_region_set_observed(
                region_id, True
            ),
            id="set_offline_region_observed",
        ),
        pytest.param(
            lambda runtime, region_id: runtime.offline_region_set_download_state(
                region_id, offline.OfflineRegionDownloadState.ACTIVE
            ),
            id="set_offline_region_download_state",
        ),
        pytest.param(
            lambda runtime, region_id: runtime.offline_region_invalidate(region_id),
            id="invalidate_offline_region",
        ),
        pytest.param(
            lambda runtime, region_id: runtime.offline_region_delete(region_id),
            id="delete_offline_region",
        ),
        pytest.param(
            lambda runtime, region_id: runtime.offline_region_update_metadata(
                region_id, b"metadata"
            ),
            id="update_offline_region_metadata",
        ),
    ],
)
def test_offline_operations_report_not_found_for_a_missing_region(
    tmp_path: Path,
    start: Callable[[mln.RuntimeHandle, int], Future[object]],
) -> None:
    with mln.runtime_create(
        replace(mln.RuntimeOptions.default(), cache_path=str(tmp_path / "cache.db"))
    ) as runtime:
        missing = 4242

        with pytest.raises(mln.NotFoundError) as raised:
            _await(start(runtime, missing))
        assert raised.value.status == mln.Status.NOT_FOUND
        assert raised.value.diagnostic


def test_offline_region_read_reports_a_missing_region_as_no_value(
    tmp_path: Path,
) -> None:
    with mln.runtime_create(
        replace(mln.RuntimeOptions.default(), cache_path=str(tmp_path / "cache.db"))
    ) as runtime:
        # The read completes successfully with no region, unlike the mutations
        # above, which report NOT_FOUND.
        assert _await(runtime.offline_region_get(4242)) is None


def test_ambient_cache_futures_complete_through_public_api(tmp_path: Path) -> None:
    with mln.runtime_create(
        replace(mln.RuntimeOptions.default(), cache_path=str(tmp_path / "cache.db"))
    ) as runtime:
        assert (
            _await(
                runtime.run_ambient_cache_operation(offline.AmbientCacheOperation.CLEAR)
            )
            is None
        )
        assert _await(runtime.set_maximum_ambient_cache_size(8 << 20)) is None
        with pytest.raises(OverflowError):
            runtime.set_maximum_ambient_cache_size(-1)
        with pytest.raises(OverflowError):
            runtime.set_maximum_ambient_cache_size(2**64)


def test_query_selector_rejects_state_key_without_feature_id():
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(5) as map_handle,
        pytest.raises(mln.InvalidArgumentError),
    ):
        map_handle.get_feature_state(
            mln.FeatureStateSelector(source_id="source", state_key="hover")
        )


def test_logging_copies_records_and_rejects_native_reentry():
    records = []

    def callback(severity, event, code, message):
        with pytest.raises(mln.InvalidStateError):
            mln.c_version()
        records.append((severity, event, code, message))
        return 1

    mln.log_set_callback(callback)
    try:
        with (
            mln.runtime_create() as runtime,
            runtime.map_create().result(5) as map_handle,
        ):
            map_handle.set_style_json(b"{").result(5)
            map_handle.dump_debug_logs().result(5)
        assert records
        assert any(message for _, _, _, message in records)
        assert all(
            isinstance(severity, mln.LogSeverity) for severity, _, _, _ in records
        )
    finally:
        mln.log_clear_callback()


def test_resource_transform_registers_and_clears():
    callback = lambda kind, url, response: mln.Status.OK
    with mln.runtime_create() as runtime:
        assert (
            _await(runtime.set_resource_transform(mln.ResourceTransform(callback)))
            is None
        )
        assert _await(runtime.clear_resource_transform()) is None


def test_resource_provider_pass_through_delegates_to_native_http() -> None:
    calls: list[resource.ResourceRequest] = []
    temporary_handles: list[resource.ResourceRequestHandle] = []

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        calls.append(request)
        temporary_handles.append(handle)
        return resource.ResourceProviderDecision.PASS_THROUGH

    with _online_network(), _http_style_server() as (style_url, served):
        with mln.runtime_create() as runtime:
            _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
            with runtime.map_create().result(timeout=5) as map_handle:
                map_handle.set_style_url(style_url)
                _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)

        assert served.is_set()
        assert any(request.kind == resource.ResourceKind.STYLE for request in calls)
        assert temporary_handles
        assert all(handle.closed for handle in temporary_handles)
        with pytest.raises(
            mln.InvalidStateError, match="closed|completed|completion|cancel callback"
        ):
            temporary_handles[0].complete(
                resource.ResourceResponse(
                    status=resource.ResourceResponseStatus.NO_CONTENT,
                    error_reason=mln.ResourceErrorReason.NONE,
                    bytes=b"",
                    must_revalidate=False,
                )
            )


def test_resource_transform_response_expires_after_callback():
    scopes = []
    calls = []

    def transform(kind, url, response):
        calls.append((kind, url))
        with pytest.raises(mln.InvalidStateError):
            runtime.close()
        failures = []

        def wrong_thread():
            try:
                response.set_url("https://wrong-thread.test")
            except mln.InvalidStateError as error:
                failures.append(error)

        worker = threading.Thread(target=wrong_thread)
        worker.start()
        worker.join(timeout=5)
        assert not worker.is_alive()
        assert len(failures) == 1
        response.set_url(rewritten_style_url)
        scopes.append(response)
        return mln.Status.OK

    with (
        _online_network(),
        _http_style_server() as (rewritten_style_url, served),
        mln.runtime_create() as runtime,
        runtime.map_create().result(5) as map_handle,
    ):
        _await(runtime.set_resource_transform(mln.ResourceTransform(transform)))
        map_handle.set_style_url("http://example.invalid/original-style.json")
        _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)
        assert served.is_set()
        assert calls[0][1] == "http://example.invalid/original-style.json"
        with pytest.raises(mln.InvalidStateError):
            scopes[0].set_url("https://expired.test")


def test_resource_transform_can_be_cleared_after_map_creation():
    calls = []

    def transform(kind, url, response):
        calls.append(url)
        return mln.Status.OK

    with (
        _online_network(),
        _http_style_server() as (url, served),
        mln.runtime_create() as runtime,
        runtime.map_create().result(5) as map_handle,
    ):
        _await(runtime.set_resource_transform(mln.ResourceTransform(transform)))
        _await(runtime.clear_resource_transform())
        map_handle.set_style_url(url)
        _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)
        assert served.is_set()
        assert calls == []


def test_resource_provider_replacement_and_clear_retire_previous_callback() -> None:
    first_urls: list[str] = []
    second_urls: list[str] = []

    def counting_provider(seen: list[str]) -> resource.ResourceProviderCallback:
        def provider(
            request: resource.ResourceRequest,
            handle: resource.ResourceRequestHandle,
        ) -> resource.ResourceProviderDecision:
            seen.append(request.requested_url)
            return resource.ResourceProviderDecision.PASS_THROUGH

        return provider

    def load_unservable_style(
        runtime: mln.RuntimeHandle,
        map_handle: mln.MapHandle,
        style_url: str,
    ) -> None:
        # No file source serves the jar scheme, so a loading failure naming this
        # style URL proves the request reached the network file source.
        map_handle.set_style_url(style_url)
        for _ in range(5000):
            for event in drain_events(runtime).events:
                if (
                    event.type == mln.RuntimeEventType.MAP_LOADING_FAILED
                    and event.message is not None
                    and style_url in event.message
                    and '"jar"' in event.message
                ):
                    return
            time.sleep(0.001)
        raise AssertionError(f"style {style_url!r} did not report a loading failure")

    with mln.runtime_create() as runtime:
        _await(
            runtime.set_resource_provider(
                mln.ResourceProvider(counting_provider(first_urls))
            )
        )
        with runtime.map_create().result(timeout=5) as map_handle:
            load_unservable_style(runtime, map_handle, "jar:file:/packaged/first.json")
            assert "jar:file:/packaged/first.json" in first_urls

            _await(
                runtime.set_resource_provider(
                    mln.ResourceProvider(counting_provider(second_urls))
                )
            )
            first_urls_after_replace = list(first_urls)
            load_unservable_style(runtime, map_handle, "jar:file:/packaged/second.json")
            assert "jar:file:/packaged/second.json" in second_urls
            assert first_urls == first_urls_after_replace

            _await(runtime.clear_resource_provider())
            second_urls_after_clear = list(second_urls)
            load_unservable_style(runtime, map_handle, "jar:file:/packaged/third.json")
            assert first_urls == first_urls_after_replace
            assert second_urls == second_urls_after_clear

            # Clearing an already cleared provider stays a successful no-op.
            _await(runtime.clear_resource_provider())


def test_resource_provider_sees_scheme_alias_and_its_resolved_url() -> None:
    """a configured URI-scheme alias arrives alongside its fetch URL."""
    resolved_urls: list[str] = []

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if request.requested_url != "maplibre://maps/style":
            return resource.ResourceProviderDecision.PASS_THROUGH
        resolved_urls.append(request.resolved_url)
        handle.complete(
            resource.ResourceResponse(
                bytes=_EMPTY_STYLE_BYTES,
                status=mln.ResourceResponseStatus.OK,
                error_reason=mln.ResourceErrorReason.NONE,
                must_revalidate=False,
            )
        )
        return resource.ResourceProviderDecision.HANDLE

    with mln.runtime_create() as runtime:
        _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
        with runtime.map_create().result(timeout=5) as map_handle:
            map_handle.set_style_url("maplibre://maps/style")
            _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)

    assert resolved_urls == ["https://demotiles.maplibre.org/style.json"]


def test_resource_provider_inline_completion_overrides_pass_through_return() -> None:
    completions = 0

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        nonlocal completions
        if request.requested_url != "custom://inline-style.json":
            return resource.ResourceProviderDecision.PASS_THROUGH
        with pytest.raises(mln.InvalidStateError):
            runtime.close()
        handle.complete(
            resource.ResourceResponse(
                bytes=_EMPTY_STYLE_BYTES,
                status=mln.ResourceResponseStatus.OK,
                error_reason=mln.ResourceErrorReason.NONE,
                must_revalidate=False,
            )
        )
        completions += 1
        return resource.ResourceProviderDecision.PASS_THROUGH

    with mln.runtime_create() as runtime:
        _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
        with runtime.map_create().result(timeout=5) as map_handle:
            map_handle.set_style_url("custom://inline-style.json")
            _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)

    assert completions == 1


def test_resource_provider_deferred_completion_loads_style_with_copied_request() -> (
    None
):
    handles: list[resource.ResourceRequestHandle] = []
    requests: list[resource.ResourceRequest] = []

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if not request.requested_url.startswith("custom://deferred-style"):
            return resource.ResourceProviderDecision.PASS_THROUGH
        requests.append(request)
        handles.append(handle)
        return resource.ResourceProviderDecision.HANDLE

    with mln.runtime_create() as runtime:
        _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
        with runtime.map_create().result(timeout=5) as map_handle:
            map_handle.set_style_url("custom://deferred-style.json")
            handle = _wait_for_provider_handle(handles)

            assert handle.cancelled() is False
            with pytest.raises(mln.InvalidArgumentError):
                handle.complete(
                    resource.ResourceResponse(
                        status=resource.ResourceResponseStatus.ERROR,
                        error_reason=resource.ResourceErrorReason.OTHER,
                        error_message="bad\0message",
                        bytes=b"",
                        must_revalidate=False,
                    )
                )
            assert handle.closed is False

            handle.complete(
                resource.ResourceResponse(
                    bytes=_EMPTY_STYLE_BYTES,
                    status=mln.ResourceResponseStatus.OK,
                    error_reason=mln.ResourceErrorReason.NONE,
                    must_revalidate=False,
                )
            )
            with pytest.raises(
                mln.InvalidStateError,
                match="closed|completed|completion|cancel callback",
            ):
                handle.complete(
                    resource.ResourceResponse(
                        bytes=_EMPTY_STYLE_BYTES,
                        status=mln.ResourceResponseStatus.OK,
                        error_reason=mln.ResourceErrorReason.NONE,
                        must_revalidate=False,
                    )
                )
            _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)

    assert requests[0].kind == resource.ResourceKind.STYLE
    assert requests[0].loading_method == resource.ResourceLoadingMethod.ALL
    assert requests[0].priority == resource.ResourcePriority.REGULAR
    assert requests[0].usage == resource.ResourceUsage.ONLINE
    assert requests[0].storage_policy == resource.ResourceStoragePolicy.PERMANENT
    assert requests[0].range is None
    assert requests[0].prior_data == b""


def test_resource_provider_can_complete_request_from_another_thread() -> None:
    handles: list[resource.ResourceRequestHandle] = []

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if request.requested_url != "custom://cross-thread-style.json":
            return resource.ResourceProviderDecision.PASS_THROUGH
        handles.append(handle)
        return resource.ResourceProviderDecision.HANDLE

    with mln.runtime_create() as runtime:
        _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
        with runtime.map_create().result(timeout=5) as map_handle:
            map_handle.set_style_url("custom://cross-thread-style.json")
            handle = _wait_for_provider_handle(handles)

            completed = threading.Event()

            def complete() -> None:
                handle.complete(
                    resource.ResourceResponse(
                        bytes=_EMPTY_STYLE_BYTES,
                        status=mln.ResourceResponseStatus.OK,
                        error_reason=mln.ResourceErrorReason.NONE,
                        must_revalidate=False,
                    ),
                )
                completed.set()

            thread = threading.Thread(target=complete, daemon=True)
            thread.start()
            assert completed.wait(timeout=2), (
                "cross-thread resource completion did not return"
            )
            thread.join(timeout=2)
            assert not thread.is_alive()
            _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)


def test_resource_request_cancel_callback_reports_discarded_request() -> None:
    """a discarded request runs its callback once, and the callback
    releases its own handle."""
    handles: list[resource.ResourceRequestHandle] = []
    cancellations: list[str] = []
    cancelled = threading.Event()

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if request.requested_url != "custom://cancel-callback-style.json":
            return resource.ResourceProviderDecision.PASS_THROUGH
        handles.append(handle)
        return resource.ResourceProviderDecision.HANDLE

    with mln.runtime_create() as runtime:
        _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
        with runtime.map_create().result(timeout=5) as map_handle:
            map_handle.set_style_url("custom://cancel-callback-style.json")
            handle = _wait_for_provider_handle(handles)

            def on_cancel() -> None:
                cancellations.append("cancelled")
                # Completing a cancelled request reports its state, and
                # releasing from inside the callback returns without waiting.
                with pytest.raises(mln.InvalidStateError):
                    handle.complete(
                        resource.ResourceResponse(
                            bytes=_EMPTY_STYLE_BYTES,
                            status=mln.ResourceResponseStatus.OK,
                            error_reason=mln.ResourceErrorReason.NONE,
                            must_revalidate=False,
                        )
                    )
                handle.close()
                cancelled.set()

            handle.set_cancel_callback(on_cancel)
            with pytest.raises(mln.InvalidStateError):
                handle.set_cancel_callback(on_cancel)
            assert handle.cancelled() is False

        assert cancelled.wait(timeout=5), "cancel callback did not run"

    assert cancellations == ["cancelled"]
    assert handle.closed is True


class _CancelProbe:
    """A cancel callback whose collection shows that its registration retired."""

    def __init__(self, calls: list[str]) -> None:
        self.calls = calls

    def __call__(self) -> None:
        self.calls.append("cancelled")


def _wait_until_collected(ref: weakref.ref[object]) -> bool:
    deadline = time.monotonic() + 5
    while ref() is not None and time.monotonic() < deadline:
        time.sleep(0.01)
    return ref() is None


def test_resource_request_cancel_callback_retires_after_running() -> None:
    """native releases a callback once it returns, while its request stays
    open."""
    handles: list[resource.ResourceRequestHandle] = []
    calls: list[str] = []

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if request.requested_url != "custom://retire-after-cancel-style.json":
            return resource.ResourceProviderDecision.PASS_THROUGH
        handles.append(handle)
        return resource.ResourceProviderDecision.HANDLE

    with mln.runtime_create() as runtime:
        _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
        with runtime.map_create().result(timeout=5) as map_handle:
            map_handle.set_style_url("custom://retire-after-cancel-style.json")
            handle = _wait_for_provider_handle(handles)
            probe = _CancelProbe(calls)
            probe_ref = weakref.ref(probe)
            assert handle.set_cancel_callback(probe) is False
            del probe
            # The accepted registration keeps the callback until it can no
            # longer run.
            assert probe_ref() is not None

        assert _wait_until_collected(probe_ref), "cancel callback was not released"
        assert calls == ["cancelled"]
        assert handle.closed is False
        handle.close()


def test_resource_request_cancel_callback_cycle_is_collectable():
    handles = []

    def provider(request, handle):
        handles.append(handle)
        return mln.ResourceProviderDecision.HANDLE

    class Cancellation:
        def __init__(self, owner):
            self.owner = owner

        def __call__(self):
            self.owner.close()

    with mln.runtime_create() as runtime, runtime.map_create().result(5) as map_handle:
        _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
        map_handle.set_style_url("custom://callback-cycle.json")
        handle = _wait_for_provider_handle(handles)
        callback = Cancellation(handle)
        handle.set_cancel_callback(callback)
        owner_ref, callback_ref = weakref.ref(handle), weakref.ref(callback)
        del callback, handle
        gc.collect()
        assert owner_ref() is None
        assert callback_ref() is None
        _await(map_handle.set_style_json(_EMPTY_STYLE_BYTES))
        _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)


@pytest.mark.parametrize("kind", ["geometry", "mvt_vector"])
def test_custom_source_callback_roots_retire_after_remove_and_rejection(kind):
    class Fetch:
        def __init__(self):
            self.retired = threading.Event()

        def __call__(self, tile):
            pass

        def __del__(self):
            self.retired.set()

    with mln.runtime_create() as runtime, runtime.map_create().result(5) as map_handle:
        _await(map_handle.set_style_json(_EMPTY_STYLE_BYTES))
        options_type = getattr(
            mln,
            "CustomGeometrySourceOptions"
            if kind == "geometry"
            else "CustomMvtVectorSourceOptions",
        )
        add = getattr(map_handle, f"add_custom_{kind}_source")
        invalid = Fetch()
        invalid_ref = weakref.ref(invalid)
        with pytest.raises(mln.InvalidArgumentError):
            add("invalid", options_type(fetch_tile=invalid, min_zoom=-1))
        del invalid
        gc.collect()
        assert invalid_ref() is None
        fetch = Fetch()
        live = weakref.ref(fetch)
        live_retired = fetch.retired
        _await(add("custom", options_type(fetch_tile=fetch, min_zoom=0, max_zoom=2)))
        del fetch
        gc.collect()
        assert live() is not None
        rejected = Fetch()
        rejected_ref = weakref.ref(rejected)
        rejected_retired = rejected.retired
        result = _await(add("custom", options_type(fetch_tile=rejected)))
        assert result.disposition == mln.CommandDisposition.FAILED
        del rejected
        gc.collect()
        assert rejected_retired.wait(5)
        assert rejected_ref() is None
        _await(map_handle.remove_style_source("custom"))
        gc.collect()
        assert live_retired.wait(5)
        assert live() is None


def test_resource_request_registration_reports_prior_cancellation() -> None:
    """Registration reports prior cancellation without entering the callback."""
    handles: list[resource.ResourceRequestHandle] = []
    cancellations: list[str] = []

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if request.requested_url != "custom://late-cancel-style.json":
            return resource.ResourceProviderDecision.PASS_THROUGH
        handles.append(handle)
        return resource.ResourceProviderDecision.HANDLE

    with mln.runtime_create() as runtime:
        _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
        map_handle = runtime.map_create().result(timeout=5)
        map_handle.set_style_url("custom://late-cancel-style.json")
        handle = _wait_for_provider_handle(handles)
        # Retiring the map discards the request, so the callback registered
        # below finds it already cancelled.
        _await(map_handle.close())
        assert handle.cancelled() is True

        def on_cancel() -> None:
            cancellations.append("cancelled")
            msg = "host failure inside the cancel callback"
            raise RuntimeError(msg)

        probe = _CancelProbe(cancellations)
        probe_ref = weakref.ref(probe)
        assert handle.set_cancel_callback(probe) is True
        del probe
        # Native stored nothing, so the binding released the callback itself.
        assert probe_ref() is None
        assert cancellations == []

        # The request keeps its one registration after reporting cancellation.
        with pytest.raises(mln.InvalidStateError):
            assert handle.set_cancel_callback(on_cancel) is True
        assert cancellations == []

        # Retirement remains available after the registration reports cancellation.
        handle.close()
        handle.wait_until_retired()
        with runtime.map_create().result(timeout=5) as second_map:
            second_map.set_style_json(_EMPTY_STYLE_BYTES)
            _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)


def test_resource_request_cancel_callback_skips_completed_request() -> None:
    """a request the provider completed never runs its callback, and
    the completed handle rejects registration as closed."""
    handles: list[resource.ResourceRequestHandle] = []
    cancellations: list[str] = []

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if request.requested_url != "custom://completed-style.json":
            return resource.ResourceProviderDecision.PASS_THROUGH
        handles.append(handle)
        return resource.ResourceProviderDecision.HANDLE

    with mln.runtime_create() as runtime:
        _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
        with runtime.map_create().result(timeout=5) as map_handle:
            map_handle.set_style_url("custom://completed-style.json")
            handle = _wait_for_provider_handle(handles)
            probe = _CancelProbe(cancellations)
            probe_ref = weakref.ref(probe)
            handle.set_cancel_callback(probe)
            del probe
            handle.complete(
                resource.ResourceResponse(
                    bytes=_EMPTY_STYLE_BYTES,
                    status=mln.ResourceResponseStatus.OK,
                    error_reason=mln.ResourceErrorReason.NONE,
                    must_revalidate=False,
                )
            )
            _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)

        # A callback that never ran stays registered until the request's
        # release.
        assert probe_ref() is not None
        handle.close()
        assert probe_ref() is None

    assert cancellations == []
    with pytest.raises(
        mln.InvalidStateError, match="closed|completed|completion|cancel callback"
    ):
        handle.set_cancel_callback(lambda: None)


def test_resource_request_cancel_callback_close_waits_from_another_thread() -> None:
    """closing a request while its cancel callback runs on a native
    worker thread waits for the callback and returns."""
    handles: list[resource.ResourceRequestHandle] = []
    running = threading.Event()
    finished = threading.Event()

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if request.requested_url != "custom://close-during-cancel-style.json":
            return resource.ResourceProviderDecision.PASS_THROUGH
        handles.append(handle)
        return resource.ResourceProviderDecision.HANDLE

    def on_cancel() -> None:
        running.set()
        # The callback holds its native worker thread while the close below
        # waits.
        time.sleep(0.2)
        finished.set()

    with mln.runtime_create() as runtime:
        _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
        with runtime.map_create().result(timeout=5) as map_handle:
            map_handle.set_style_url("custom://close-during-cancel-style.json")
            handle = _wait_for_provider_handle(handles)
            handle.set_cancel_callback(on_cancel)

            # A second style discards the pending request of the first one.
            map_handle.set_style_json(_EMPTY_STYLE_BYTES)
            assert running.wait(timeout=5), "cancel callback did not run"

            # The callback is still inside its sleep, so this close has to wait
            # for it.
            handle.close()
            assert finished.is_set()
            assert handle.closed is True


def test_resource_request_cancel_callback_allows_map_use_while_closing() -> None:
    """other host threads keep running while a map retirement waits for
    a cancel callback."""
    handles: list[resource.ResourceRequestHandle] = []
    running = threading.Event()
    stop_reader = threading.Event()

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if request.requested_url != "custom://close-map-during-cancel-style.json":
            return resource.ResourceProviderDecision.PASS_THROUGH
        handles.append(handle)
        return resource.ResourceProviderDecision.HANDLE

    def on_cancel() -> None:
        running.set()
        time.sleep(0.2)

    with mln.runtime_create() as runtime:
        _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
        map_handle = runtime.map_create().result(timeout=5)
        map_handle.set_style_url("custom://close-map-during-cancel-style.json")
        handle = _wait_for_provider_handle(handles)
        handle.set_cancel_callback(on_cancel)

        reads: list[bool] = []

        def read_map_state() -> None:
            while not stop_reader.is_set():
                reads.append(map_handle.closed)

        reader = threading.Thread(target=read_map_state, daemon=True)
        reader.start()
        try:
            # Retirement runs the cancel callback on a native worker thread;
            # the reader keeps reading map state until the future settles.
            _await(map_handle.close())
        finally:
            stop_reader.set()
            reader.join(timeout=2)
        assert running.is_set(), "cancel callback did not run"
        assert reads, "the reader thread never observed map state"
        assert map_handle.closed is True
        handle.close()


def test_resource_provider_error_response_reports_loading_failure_event() -> None:
    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if request.requested_url != "custom://error-style.json":
            return resource.ResourceProviderDecision.PASS_THROUGH
        handle.complete(
            resource.ResourceResponse(
                status=resource.ResourceResponseStatus.ERROR,
                error_reason=resource.ResourceErrorReason.NOT_FOUND,
                error_message="custom style failed",
                bytes=b"",
                must_revalidate=False,
            )
        )
        return resource.ResourceProviderDecision.HANDLE

    with mln.runtime_create() as runtime:
        _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
        with runtime.map_create().result(timeout=5) as map_handle:
            map_handle.set_style_url("custom://error-style.json")
            event = _wait_for_runtime_event(
                runtime,
                mln.RuntimeEventType.MAP_LOADING_FAILED,
            )

    assert event.message


def test_resource_provider_error_response_reports_offline_response_error_event(
    tmp_path: Path,
) -> None:
    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if request.requested_url != "custom://offline-error-style.json":
            return resource.ResourceProviderDecision.PASS_THROUGH
        handle.complete(
            resource.ResourceResponse(
                status=resource.ResourceResponseStatus.ERROR,
                error_reason=resource.ResourceErrorReason.NOT_FOUND,
                error_message="offline style failed",
                bytes=b"",
                must_revalidate=False,
            )
        )
        return resource.ResourceProviderDecision.HANDLE

    definition = mln.OfflineRegionDefinition(
        mln.OfflineRegionDefinitionTilePyramidVariant(
            mln.OfflineTilePyramidRegionDefinition(
                style_url="custom://offline-error-style.json",
                bounds=geo.LatLngBounds(
                    southwest=geo.LatLng(1.0, 2.0),
                    northeast=geo.LatLng(3.0, 4.0),
                ),
                min_zoom=5.0,
                max_zoom=6.0,
                pixel_ratio=1.0,
                include_ideographs=False,
            )
        )
    )

    with mln.runtime_create(
        replace(
            mln.RuntimeOptions.default(), cache_path=str(tmp_path / "offline-cache.db")
        )
    ) as runtime:
        _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
        region = _await(runtime.offline_region_create(definition, b"metadata"))

        _await(runtime.offline_region_set_observed(region.id, True))

        _await(
            runtime.offline_region_set_download_state(
                region.id,
                offline.OfflineRegionDownloadState.ACTIVE,
            )
        )
        event = _wait_for_runtime_event(
            runtime,
            mln.RuntimeEventType.OFFLINE_REGION_RESPONSE_ERROR,
        )
        _await(
            runtime.offline_region_set_download_state(
                region.id,
                offline.OfflineRegionDownloadState.INACTIVE,
            )
        )
        _await(runtime.offline_region_set_observed(region.id, False))

    assert isinstance(event.payload, mln.RuntimeEventOfflineRegionResponseErrorVariant)
    assert event.payload.value.region_id == region.id
    assert event.payload.value.reason == resource.ResourceErrorReason.NOT_FOUND
    assert event.message


def test_resource_request_cancellation_makes_late_completion_terminal() -> None:
    handles: list[resource.ResourceRequestHandle] = []

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if request.requested_url != "custom://cancelled-style.json":
            return resource.ResourceProviderDecision.PASS_THROUGH
        handles.append(handle)
        return resource.ResourceProviderDecision.HANDLE

    with mln.runtime_create() as runtime:
        _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
        map_handle = runtime.map_create().result(timeout=5)
        map_handle.set_style_url("custom://cancelled-style.json")
        handle = _wait_for_provider_handle(handles)

        # Map teardown is the fence: the request is cancelled by the time the
        # close future resolves.
        _await(map_handle.close())
        assert handle.cancelled()

        with pytest.raises(mln.InvalidStateError) as native_error:
            handle.complete(
                resource.ResourceResponse(
                    bytes=_EMPTY_STYLE_BYTES,
                    status=mln.ResourceResponseStatus.OK,
                    error_reason=mln.ResourceErrorReason.NONE,
                    must_revalidate=False,
                )
            )
        assert native_error.value.native_status_code == -2
        assert native_error.value.diagnostic

        with pytest.raises(mln.InvalidStateError) as terminal_error:
            handle.complete(
                resource.ResourceResponse(
                    bytes=_EMPTY_STYLE_BYTES,
                    status=mln.ResourceResponseStatus.OK,
                    error_reason=mln.ResourceErrorReason.NONE,
                    must_revalidate=False,
                )
            )
        assert terminal_error.value.native_status_code == -2


def test_released_resource_request_handle_stays_stale_after_later_request() -> None:
    handles: list[resource.ResourceRequestHandle] = []

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if not request.requested_url.startswith("custom://stale-style"):
            return resource.ResourceProviderDecision.PASS_THROUGH
        handles.append(handle)
        return resource.ResourceProviderDecision.HANDLE

    with mln.runtime_create() as runtime:
        _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
        with runtime.map_create().result(timeout=5) as map_handle:
            map_handle.set_style_url("custom://stale-style-1.json")
            stale_handle = _wait_for_provider_handle(handles)
            stale_handle.close()
            with pytest.raises(
                mln.InvalidStateError,
                match="closed|completed|completion|cancel callback",
            ):
                stale_handle.cancelled()

            map_handle.set_style_url("custom://stale-style-2.json")
            live_handle = _wait_for_provider_handle(handles)
            with pytest.raises(
                mln.InvalidStateError,
                match="closed|completed|completion|cancel callback",
            ):
                stale_handle.complete(
                    resource.ResourceResponse(
                        bytes=_EMPTY_STYLE_BYTES,
                        status=mln.ResourceResponseStatus.OK,
                        error_reason=mln.ResourceErrorReason.NONE,
                        must_revalidate=False,
                    )
                )
            assert live_handle.cancelled() is False
            live_handle.complete(
                resource.ResourceResponse(
                    bytes=_EMPTY_STYLE_BYTES,
                    status=mln.ResourceResponseStatus.OK,
                    error_reason=mln.ResourceErrorReason.NONE,
                    must_revalidate=False,
                )
            )
            _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)


def test_resource_request_release_race_with_cancellation_checks_closes_cleanly() -> (
    None
):
    handles: list[resource.ResourceRequestHandle] = []

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if request.requested_url != "custom://release-race-style.json":
            return resource.ResourceProviderDecision.PASS_THROUGH
        handles.append(handle)
        return resource.ResourceProviderDecision.HANDLE

    with mln.runtime_create() as runtime:
        _await(runtime.set_resource_provider(mln.ResourceProvider(provider)))
        with runtime.map_create().result(timeout=5) as map_handle:
            map_handle.set_style_url("custom://release-race-style.json")
            handle = _wait_for_provider_handle(handles)

            started = threading.Event()
            stop = threading.Event()
            saw_closed = threading.Event()

            def probe_cancelled() -> None:
                while not stop.is_set():
                    try:
                        handle.cancelled()
                        started.set()
                    except mln.InvalidArgumentError, mln.InvalidStateError:
                        saw_closed.set()
                        stop.set()

            probe = threading.Thread(target=probe_cancelled, daemon=True)
            probe.start()
            try:
                assert started.wait(timeout=2)
                handle.close()
                assert saw_closed.wait(timeout=2)
            finally:
                stop.set()
                probe.join(timeout=2)
            assert not probe.is_alive(), "cancellation probe did not return"
            with pytest.raises(
                mln.InvalidStateError,
                match="closed|completed|completion|cancel callback",
            ):
                handle.cancelled()


def _load_style_and_collect_types(
    runtime: mln.RuntimeHandle,
    map_handle: mln.MapHandle,
    until: mln.RuntimeEventType | None,
) -> set[mln.RuntimeEventType]:
    """Load the empty style, collecting event types until `until` arrives."""
    map_handle.set_style_json(_EMPTY_STYLE_BYTES)
    types: set[mln.RuntimeEventType] = set()
    for _ in range(500):
        types.update(event.type for event in drain_events(runtime).events)
        if until is not None and until in types:
            break
        time.sleep(0.001)
    return types


@pytest.mark.parametrize("transition_id", [-1, 2**64])
def test_transition_id_out_of_range_raises_binding_error(transition_id: int) -> None:
    with (
        mln.runtime_create() as runtime,
        runtime.map_create().result(timeout=5) as map_handle,
        pytest.raises(OverflowError),
    ):
        map_handle.update_camera(
            replace(
                mln.CameraUpdate.default(),
                mode=mln.CameraUpdateMode.EASE,
                camera=camera.CameraOptions(zoom=2.0),
                animation=camera.AnimationOptions(
                    duration_ms=0.0, transition_id=transition_id
                ),
            )
        )


# global-state lifetime and copied JSON values.
def test_global_state_defaults_updates_and_style_replacement() -> None:
    with mln.runtime_create() as runtime, _await(runtime.map_create()) as map_handle:
        _assert_command_failed(
            map_handle.set_global_state_property("theme", b"true"),
            mln.Status.INVALID_STATE,
        )
        map_handle.set_style_json(
            b'{"version":8,"sources":{},"layers":[],"state":{"theme":{"default":"light"}}}'
        )
        assert json.loads(_await(map_handle.get_global_state())) == {"theme": "light"}
        _await(
            map_handle.set_global_state_property("theme", b'["dark",{"enabled":true}]')
        )
        snapshot = _await(map_handle.get_global_state())
        _await(map_handle.set_global_state_property("theme", b"null"))
        assert json.loads(_await(map_handle.get_global_state())) == {"theme": "light"}
        assert json.loads(snapshot) == {"theme": ["dark", {"enabled": True}]}
        _await(map_handle.set_style_json(_EMPTY_STYLE_BYTES))
        assert json.loads(_await(map_handle.get_global_state())) == {}
        _await(map_handle.set_global_state_property("theme", b"true"))
        _await(map_handle.set_global_state_property("theme", b"null"))
        assert json.loads(_await(map_handle.get_global_state())) == {"theme": None}

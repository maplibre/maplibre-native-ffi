import concurrent.futures
import contextlib
import http.server
import json
import math
import subprocess
import sys
import textwrap
import threading
import time
import typing
import warnings
from collections.abc import Callable
from pathlib import Path

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import (
    _native,
    camera,
    geo,
    log,
    offline,
    query,
    render,
    resource,
    style,
)
from maplibre_native_ffi import (
    map as map_module,
)

_EMPTY_STYLE_JSON = '{"version":8,"sources":{},"layers":[]}'
_EMPTY_STYLE_BYTES = _EMPTY_STYLE_JSON.encode()


def _json_object(value: object) -> bytes:
    return json.dumps(value, separators=(",", ":")).encode()


def _json_value(value: object) -> bytes:
    return _json_object(value)


@contextlib.contextmanager
def _online_network() -> typing.Iterator[None]:
    original = mln.network_status()
    mln.set_network_status(mln.NetworkStatus.ONLINE)
    try:
        yield
    finally:
        mln.set_network_status(original)


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
        runtime.pump()
        for event in runtime.drain_events().events:
            if event.event_type == event_type:
                return event
        time.sleep(0.001)
    raise AssertionError(f"runtime event {event_type!r} was not observed")


def _wait_for_provider_handle(
    runtime: mln.RuntimeHandle,
    handles: list[resource.ResourceRequestHandle],
    *,
    iterations: int = 5000,
) -> resource.ResourceRequestHandle:
    for _ in range(iterations):
        runtime.pump()
        if handles:
            return handles.pop(0)
        time.sleep(0.001)
    raise AssertionError("resource provider did not expose a handled request")


def _wait_for_offline_operation(
    runtime: mln.RuntimeHandle,
    operation: offline.OfflineOperationHandle,
    *,
    iterations: int = 5000,
) -> mln.RuntimeEvent:
    operation_id = operation._operation_id
    for _ in range(iterations):
        runtime.pump()
        for event in runtime.drain_events().events:
            if (
                event.event_type == mln.RuntimeEventType.OFFLINE_OPERATION_COMPLETED
                and isinstance(event.payload, offline.OfflineOperationCompleted)
                and event.payload.operation_id == operation_id
            ):
                return event
        time.sleep(0.001)
    raise AssertionError(f"offline operation {operation_id!r} did not complete")


def test_c_version_matches_expected_abi_version() -> None:
    assert mln.c_version() == mln.EXPECTED_C_ABI_VERSION


def test_supported_render_backends_returns_flag_value() -> None:
    assert isinstance(mln.supported_render_backends(), mln.RenderBackend)
    assert render.RenderBackend.OPENGL.value == 1 << 2


def test_supported_opengl_context_providers_returns_flag_value() -> None:
    assert isinstance(
        mln.supported_opengl_context_providers(), mln.OpenGLContextProvider
    )


def test_native_pointer_is_opaque_value() -> None:
    pointer = mln.NativePointer(0)

    assert pointer.is_null
    assert pointer == mln.NativePointer.null()


def test_frame_backend_values_are_scoped_to_open_frame_handle() -> None:
    class FakeMetalFrame:
        closed = False

        def texture_address(self) -> int:
            return 0x1234

        def device_address(self) -> int:
            return 0x5678

        def close(self) -> None:
            self.closed = True

    with pytest.raises(TypeError, match="created by RenderSessionHandle"):
        render.MetalOwnedTextureFrameHandle(FakeMetalFrame())

    metal = render.MetalOwnedTextureFrameHandle._from_native(FakeMetalFrame())
    texture = metal.texture
    device = metal.device
    assert texture.address == 0x1234
    assert device.address == 0x5678
    metal.close()
    with pytest.raises(mln.InvalidStateError):
        _ = texture.address
    with pytest.raises(mln.InvalidStateError):
        _ = device.address

    class FakeOpenGLFrame:
        closed = False

        def texture(self) -> int:
            return 42

        def close(self) -> None:
            self.closed = True

    opengl = render.OpenGLOwnedTextureFrameHandle._from_native(FakeOpenGLFrame())
    opengl_texture = opengl.texture
    assert int(opengl_texture) == 42
    opengl.close()
    with pytest.raises(mln.InvalidStateError):
        _ = opengl_texture.value


def test_network_status_round_trips_through_public_api() -> None:
    original = mln.network_status()
    try:
        mln.set_network_status(mln.NetworkStatus.OFFLINE)
        assert mln.network_status() == mln.NetworkStatus.OFFLINE
        mln.set_network_status(mln.NetworkStatus.ONLINE)
        assert mln.network_status() == mln.NetworkStatus.ONLINE
    finally:
        mln.set_network_status(original)


def test_network_status_preserves_unknown_raw_values() -> None:
    status = mln.NetworkStatus(999_001)

    assert status.is_unknown
    assert status.native_code == 999_001


def test_unknown_network_status_setter_raises_invalid_argument() -> None:
    stale = _native_invalid_network_status_error().diagnostic

    with pytest.raises(mln.InvalidArgumentError) as raised:
        mln.set_network_status(mln.NetworkStatus(999_001))

    assert raised.value.status == mln.MaplibreStatus.INVALID_ARGUMENT
    assert raised.value.native_status_code is None
    assert "cannot be set" in raised.value.diagnostic
    assert raised.value.diagnostic != stale


def test_native_status_conversion_preserves_status_and_diagnostic() -> None:
    with pytest.raises(mln.InvalidArgumentError) as raised:
        _native.set_network_status_raw_unchecked_for_test(999_001)

    error = raised.value
    copied = error.diagnostic

    with pytest.raises(mln.InvalidArgumentError) as later:
        _native.projected_meters_for_lat_lng(1000.0, 0.0)

    assert error.status == mln.MaplibreStatus.INVALID_ARGUMENT
    assert error.native_status_code == mln.MaplibreStatus.INVALID_ARGUMENT.native_code
    assert "network status" in error.diagnostic
    assert error.diagnostic == copied
    assert later.value.diagnostic != copied


@pytest.mark.parametrize(
    ("status", "error_type"),
    (
        (mln.MaplibreStatus.INVALID_ARGUMENT, mln.InvalidArgumentError),
        (mln.MaplibreStatus.INVALID_STATE, mln.InvalidStateError),
        (mln.MaplibreStatus.WRONG_THREAD, mln.WrongThreadError),
        (mln.MaplibreStatus.UNSUPPORTED, mln.UnsupportedFeatureError),
        (mln.MaplibreStatus.NATIVE_ERROR, mln.NativeError),
    ),
)
def test_native_status_categories_map_to_public_errors(
    status: mln.MaplibreStatus,
    error_type: type[mln.MaplibreError],
) -> None:
    diagnostic = f"synthetic native diagnostic for {status.name}"

    with pytest.raises(error_type) as raised:
        _native.status_error_for_test(status.native_code, diagnostic)

    assert raised.value.status == status
    assert raised.value.native_status_code == status.native_code
    assert raised.value.diagnostic == diagnostic


def test_ok_native_status_does_not_raise() -> None:
    _native.status_error_for_test(mln.MaplibreStatus.OK.native_code, "unused")


def test_unknown_native_status_preserves_raw_status() -> None:
    with pytest.raises(mln.UnknownStatusError) as raised:
        _native.status_error_for_test(-123_456, "future native status")

    assert raised.value.status == mln.MaplibreStatus.UNKNOWN
    assert raised.value.native_status_code == -123_456
    assert raised.value.diagnostic == "future native status"


def test_support_work_preserves_original_native_diagnostic() -> None:
    with pytest.raises(mln.UnsupportedFeatureError) as raised:
        _native.status_error_after_support_call_for_test(
            mln.MaplibreStatus.UNSUPPORTED.native_code,
            "original native diagnostic",
        )

    assert raised.value.status == mln.MaplibreStatus.UNSUPPORTED
    assert raised.value.native_status_code == mln.MaplibreStatus.UNSUPPORTED.native_code
    assert raised.value.diagnostic == "original native diagnostic"


def test_runtime_abi_mismatch_reports_public_error_before_handle_storage(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    actual_abi_version = mln.EXPECTED_C_ABI_VERSION + 1

    def create_mismatched_runtime(
        asset_path: str | None,
        cache_path: str | None,
        event_mask: int,
    ) -> object:
        return _native.create_runtime_with_abi_version_for_test(
            actual_abi_version,
            asset_path,
            cache_path,
        )

    monkeypatch.setattr(_native, "create_runtime", create_mismatched_runtime)
    runtime = mln.RuntimeHandle.__new__(mln.RuntimeHandle)

    with pytest.raises(mln.UnsupportedFeatureError) as raised:
        mln.RuntimeHandle.__init__(runtime)

    assert not hasattr(runtime, "_native")
    assert raised.value.status == mln.MaplibreStatus.UNSUPPORTED
    assert raised.value.native_status_code is None
    assert "unsupported MapLibre Native C ABI version" in raised.value.diagnostic
    assert str(actual_abi_version) in raised.value.diagnostic


def _native_invalid_network_status_error() -> mln.InvalidArgumentError:
    with pytest.raises(mln.InvalidArgumentError) as raised:
        _native.set_network_status_raw_unchecked_for_test(999_001)
    return raised.value


def test_public_type_hints_are_resolvable() -> None:
    targets = (
        map_module.MapHandle.add_style_source_json,
        map_module.MapHandle.add_style_layer_json,
        map_module.MapHandle.set_style_light_json,
        map_module.MapHandle.set_style_light_property,
        map_module.MapHandle.set_layer_property,
        map_module.MapHandle.set_layer_filter,
        map_module.MapHandle.__init__,
        map_module.MapHandle.set_style_image,
        render.RenderSessionHandle.__init__,
        render.RenderSessionHandle.acquire_opengl_owned_texture_frame,
        render.RenderSessionHandle.query_feature_extensions,
        render.RenderSessionHandle.query_rendered_features,
        render.RenderSessionHandle.query_source_features,
        render.RenderSessionHandle.set_feature_state,
        mln.RuntimeHandle.create_map,
        mln.RuntimeHandle.create_offline_region,
        mln.RuntimeHandle.set_resource_transform,
        mln.RuntimeHandle.set_resource_provider,
        offline.OfflineOperationHandle.__init__,
        offline.OfflineRegionResponseError.__init__,
    )

    for target in targets:
        assert isinstance(typing.get_type_hints(target), dict)

    map_hints = typing.get_type_hints(map_module.MapHandle.add_style_source_json)
    assert map_hints["source_json"] != typing.Any
    assert map_hints["source_json"] is bytes

    layer_hints = typing.get_type_hints(map_module.MapHandle.set_layer_property)
    assert layer_hints["value"] != typing.Any
    assert layer_hints["value"] is bytes

    style_hints = typing.get_type_hints(map_module.MapHandle.get_style_layer_json)
    assert style_hints["return"] != typing.Any
    assert set(typing.get_args(style_hints["return"])) == {bytes, type(None)}

    map_init_hints = typing.get_type_hints(map_module.MapHandle.__init__)
    assert map_init_hints["runtime"] is mln.RuntimeHandle

    image_hints = typing.get_type_hints(map_module.MapHandle.set_style_image)
    assert image_hints["image"] is render.PremultipliedRgba8Image

    session_init_hints = typing.get_type_hints(render.RenderSessionHandle.__init__)
    assert session_init_hints["map_handle"] is map_module.MapHandle

    create_map_hints = typing.get_type_hints(mln.RuntimeHandle.create_map)
    assert create_map_hints["options"] == map_module.MapOptions | None
    assert create_map_hints["return"] is map_module.MapHandle

    offline_handle_hints = typing.get_type_hints(
        offline.OfflineOperationHandle.__init__
    )
    assert offline_handle_hints["runtime"] is mln.RuntimeHandle

    provider_hints = typing.get_type_hints(mln.RuntimeHandle.set_resource_provider)
    assert provider_hints["callback"] != typing.Any
    assert "ResourceRequest" in repr(provider_hints["callback"])

    transform_hints = typing.get_type_hints(mln.RuntimeHandle.set_resource_transform)
    assert transform_hints["callback"] != typing.Any
    assert "ResourceTransformRequest" in repr(transform_hints["callback"])

    extension_hints = typing.get_type_hints(
        render.RenderSessionHandle.query_feature_extensions
    )
    assert extension_hints["feature"] is bytes
    assert extension_hints["return"] is bytes
    assert extension_hints["arguments"] == bytes | None

    rendered_hints = typing.get_type_hints(
        render.RenderSessionHandle.query_rendered_features
    )
    assert rendered_hints["geometry"] is query.RenderedQueryGeometry
    assert rendered_hints["options"] != typing.Any
    assert rendered_hints["return"] == list[query.QueriedFeature]

    source_hints = typing.get_type_hints(
        render.RenderSessionHandle.query_source_features
    )
    assert source_hints["source_id"] is str
    assert source_hints["return"] == list[query.QueriedFeature]

    response_error_hints = typing.get_type_hints(offline.OfflineRegionResponseError)
    assert response_error_hints["reason"] is resource.ResourceErrorReason


def test_public_modules_avoid_runtime_annotation_fallbacks() -> None:
    package_dir = Path(mln.__file__).parent
    for path in package_dir.glob("*.py"):
        source = path.read_text()
        assert "TYPE_CHECKING" not in source, path.name
        assert " = Any" not in source, path.name


def test_runtime_handle_context_manager_closes_once() -> None:
    with mln.RuntimeHandle() as runtime:
        assert not runtime.closed
        runtime.pump()

    assert runtime.closed
    runtime.close()
    assert runtime.closed


def quiesce(runtime: mln.RuntimeHandle) -> None:
    """Pump until the runtime is idle, so a park that follows needs a fresh signal."""
    for _ in range(100):
        runtime.pump()
        if not runtime.drain_events().events:
            return
    pytest.fail("the runtime kept producing events while idle")


def test_parked_owner_thread_wakes_for_native_work_and_for_a_wake_source() -> None:
    with mln.RuntimeHandle() as runtime:
        map_handle = runtime.create_map()
        quiesce(runtime)

        # The style is malformed, so native reports the failure from its own
        # threads and the failure reaches the parked owner thread.
        map_handle.set_style_url("unsupported://style.json")
        loading_failed = False
        load_started = time.monotonic()
        for _ in range(20):
            runtime.pump(10.0)
            assert time.monotonic() - load_started < 5.0, (
                "parks sat out their timeouts while loading was pending"
            )
            for event in runtime.drain_events().events:
                if event.event_type == mln.RuntimeEventType.MAP_LOADING_FAILED:
                    loading_failed = True
            if loading_failed:
                break
        assert loading_failed

        # A source signalled from another thread matches a host's submission path,
        # and the park it releases has no other work to end it. The park also
        # releases the GIL.
        source = runtime.wake_source()
        quiesce(runtime)
        thread = threading.Thread(target=lambda: (time.sleep(0.02), source.signal()))
        thread.start()
        park_started = time.monotonic()
        runtime.pump(10.0)
        assert time.monotonic() - park_started < 5.0, (
            "the parked owner thread timed out instead of taking the signal"
        )
        thread.join()

        map_handle.close()

    # A wake source stays usable after its runtime closes, so hosts tear the two
    # down in either order.
    source.signal()
    source.close()
    assert source.closed


def test_pump_clears_the_wake_flag_it_returns_on() -> None:
    with mln.RuntimeHandle() as runtime, runtime.wake_source() as source:
        quiesce(runtime)

        source.signal()
        signalled_started = time.monotonic()
        runtime.pump(10.0)
        assert time.monotonic() - signalled_started < 5.0, (
            "a pump waited even though the wake flag was set"
        )

        # The pump above cleared the wake flag, so this one waits its
        # full timeout.
        idle_started = time.monotonic()
        runtime.pump(0.2)
        assert time.monotonic() - idle_started >= 0.1, (
            "the first pump left the wake flag set"
        )


@pytest.mark.skipif(
    hasattr(sys, "getandroidapilevel"),
    reason="Android embeds Python and has no standalone interpreter executable",
)
def test_closed_handle_finalizers_are_quiet_at_interpreter_shutdown() -> None:
    script = textwrap.dedent(
        """
        import maplibre_native_ffi as mln
        from maplibre_native_ffi import style

        runtime = mln.RuntimeHandle()
        map_handle = runtime.create_map()
        map_handle.set_style_json(b'{"version":8,"sources":{},"layers":[]}')
        source = map_handle.add_custom_geometry_source(
            "custom",
            style.CustomGeometrySourceOptions(max_queued_events=1),
        )
        source.close()
        map_handle.close()
        runtime.close()
        """
    )

    completed = subprocess.run(
        [sys.executable, "-c", script],
        check=False,
        capture_output=True,
        text=True,
    )

    assert completed.returncode == 0, completed.stderr
    assert "Exception ignored while calling deallocator" not in completed.stderr
    assert "sys.meta_path is None" not in completed.stderr


def test_duplicate_runtime_reports_invalid_state() -> None:
    runtime = mln.RuntimeHandle()
    try:
        with pytest.raises(mln.InvalidStateError) as raised:
            mln.RuntimeHandle()

        assert raised.value.status == mln.MaplibreStatus.INVALID_STATE
        assert (
            raised.value.native_status_code
            == mln.MaplibreStatus.INVALID_STATE.native_code
        )
    finally:
        runtime.close()


def test_runtime_close_from_wrong_thread_reports_wrong_thread() -> None:
    runtime = mln.RuntimeHandle()
    raised_error: list[BaseException] = []

    def close_runtime() -> None:
        try:
            runtime.close()
        except mln.WrongThreadError as error:
            raised_error.append(error)

    thread = threading.Thread(target=close_runtime)
    thread.start()
    thread.join()

    try:
        assert len(raised_error) == 1
        assert_wrong_thread_error(raised_error[0])
    finally:
        runtime.close()


def assert_wrong_thread_error(
    error: BaseException, diagnostic: str | None = None
) -> None:
    assert isinstance(error, mln.WrongThreadError)
    assert error.status == mln.MaplibreStatus.WRONG_THREAD
    assert error.native_status_code == mln.MaplibreStatus.WRONG_THREAD.native_code
    if diagnostic is None:
        assert error.diagnostic
    else:
        assert error.diagnostic == diagnostic


def test_owner_thread_methods_report_wrong_thread_diagnostics() -> None:
    runtime = mln.RuntimeHandle()
    map_handle = runtime.create_map()
    raised_errors: list[BaseException] = []

    def call_owner_thread_methods() -> None:
        calls: tuple[Callable[[], object], ...] = (
            runtime.pump,
            runtime.drain_events,
            map_handle.request_repaint,
            lambda: runtime.set_event_mask(mln.RuntimeEventMask.ALL),
            lambda: map_handle.set_event_mask(mln.RuntimeEventMask.ALL),
        )
        for call in calls:
            try:
                call()
            except mln.WrongThreadError as error:
                raised_errors.append(error)

    thread = threading.Thread(target=call_owner_thread_methods)
    thread.start()
    thread.join()

    try:
        assert len(raised_errors) == 5
        for error in raised_errors:
            assert_wrong_thread_error(error)
    finally:
        map_handle.close()
        runtime.close()


def test_resource_transform_registration_reports_wrong_thread_diagnostics() -> None:
    runtime = mln.RuntimeHandle()
    raised_errors: list[BaseException] = []

    def transform(request: resource.ResourceTransformRequest) -> str | None:
        return request.url

    def call_resource_transform_methods() -> None:
        for call in (
            lambda: runtime.set_resource_transform(transform, max_pending_callbacks=1),
            runtime.clear_resource_transform,
        ):
            try:
                call()
            except mln.WrongThreadError as error:
                raised_errors.append(error)

    thread = threading.Thread(target=call_resource_transform_methods)
    thread.start()
    thread.join()

    try:
        assert len(raised_errors) == 2
        for error in raised_errors:
            assert_wrong_thread_error(error)
    finally:
        runtime.close()


def test_render_session_methods_propagate_wrong_thread_errors() -> None:
    diagnostics = {
        "close": "wrong thread while closing render session",
        "resize": "wrong thread while resizing render session",
        "render_update": "wrong thread while rendering update",
        "detach": "wrong thread while detaching render session",
        "reduce_memory_use": "wrong thread while reducing memory use",
        "clear_data": "wrong thread while clearing data",
        "dump_debug_logs": "wrong thread while dumping debug logs",
        "texture_image_info": "wrong thread while reading texture info",
        "read_premultiplied_rgba8_into": "wrong thread while reading into buffer",
        "acquire_metal_owned_texture_frame": "wrong thread while acquiring Metal frame",
        "acquire_vulkan_owned_texture_frame": (
            "wrong thread while acquiring Vulkan frame"
        ),
        "acquire_opengl_owned_texture_frame": (
            "wrong thread while acquiring OpenGL frame"
        ),
        "query_rendered_features": "wrong thread while querying rendered features",
        "query_source_features": "wrong thread while querying source features",
        "query_feature_extensions": "wrong thread while querying feature extensions",
        "set_feature_state": "wrong thread while setting feature state",
        "get_feature_state": "wrong thread while getting feature state",
        "remove_feature_state": "wrong thread while removing feature state",
    }

    class FakeNativeRenderSession:
        closed = False
        detached = False

        def _wrong_thread(self, method: str) -> None:
            raise mln.WrongThreadError(
                mln.MaplibreStatus.WRONG_THREAD.native_code,
                diagnostics[method],
            )

        def close(self) -> None:
            self._wrong_thread("close")

        def resize(self, width: int, height: int, scale_factor: float) -> None:
            self._wrong_thread("resize")

        def render_update(self) -> None:
            self._wrong_thread("render_update")

        def detach(self) -> None:
            self._wrong_thread("detach")

        def reduce_memory_use(self) -> None:
            self._wrong_thread("reduce_memory_use")

        def clear_data(self) -> None:
            self._wrong_thread("clear_data")

        def dump_debug_logs(self) -> None:
            self._wrong_thread("dump_debug_logs")

        def texture_image_info(self) -> None:
            self._wrong_thread("texture_image_info")

        def read_premultiplied_rgba8_into(self, buffer: object) -> None:
            self._wrong_thread("read_premultiplied_rgba8_into")

        def acquire_metal_owned_texture_frame(self) -> None:
            self._wrong_thread("acquire_metal_owned_texture_frame")

        def acquire_vulkan_owned_texture_frame(self) -> None:
            self._wrong_thread("acquire_vulkan_owned_texture_frame")

        def acquire_opengl_owned_texture_frame(self) -> None:
            self._wrong_thread("acquire_opengl_owned_texture_frame")

        def query_rendered_features(
            self,
            geometry: object,
            layer_ids: tuple[str, ...] | None,
            filter_: object,
        ) -> None:
            self._wrong_thread("query_rendered_features")

        def query_source_features(
            self,
            source_id: str,
            source_layer_ids: tuple[str, ...] | None,
            filter_: object,
        ) -> None:
            self._wrong_thread("query_source_features")

        def query_feature_extensions(
            self,
            source_id: str,
            feature: object,
            extension: str,
            extension_field: str,
            arguments: object,
        ) -> None:
            self._wrong_thread("query_feature_extensions")

        def set_feature_state(
            self,
            source_id: str,
            source_layer_id: str | None,
            feature_id: str | None,
            state_key: str | None,
            state: object,
        ) -> None:
            self._wrong_thread("set_feature_state")

        def get_feature_state(
            self,
            source_id: str,
            source_layer_id: str | None,
            feature_id: str | None,
            state_key: str | None,
        ) -> None:
            self._wrong_thread("get_feature_state")

        def remove_feature_state(
            self,
            source_id: str,
            source_layer_id: str | None,
            feature_id: str | None,
            state_key: str | None,
        ) -> None:
            self._wrong_thread("remove_feature_state")

    session = render.RenderSessionHandle._from_native(
        FakeNativeRenderSession(), object()
    )
    point_query = query.RenderedQueryGeometry.point_geometry(
        camera.ScreenPoint(1.0, 2.0)
    )
    feature = _json_object(
        {
            "type": "Feature",
            "geometry": {"type": "Point", "coordinates": [2.0, 1.0]},
            "properties": {},
        }
    )
    selector = query.FeatureStateSelector(source_id="points", feature_id="feature-1")
    calls = {
        "close": session.close,
        "resize": lambda: session.resize(128, 64, 1.0),
        "render_update": session.render_update,
        "detach": session.detach,
        "reduce_memory_use": session.reduce_memory_use,
        "clear_data": session.clear_data,
        "dump_debug_logs": session.dump_debug_logs,
        "texture_image_info": session.texture_image_info,
        "read_premultiplied_rgba8_into": (
            lambda: session.read_premultiplied_rgba8_into(bytearray(4))
        ),
        "acquire_metal_owned_texture_frame": session.acquire_metal_owned_texture_frame,
        "acquire_vulkan_owned_texture_frame": (
            session.acquire_vulkan_owned_texture_frame
        ),
        "acquire_opengl_owned_texture_frame": (
            session.acquire_opengl_owned_texture_frame
        ),
        "query_rendered_features": lambda: session.query_rendered_features(point_query),
        "query_source_features": lambda: session.query_source_features("points"),
        "query_feature_extensions": (
            lambda: session.query_feature_extensions(
                "points",
                feature,
                "supercluster",
                "leaves",
                _json_object({"limit": 10}),
            )
        ),
        "set_feature_state": (
            lambda: session.set_feature_state(
                selector,
                _json_object({"hover": True}),
            )
        ),
        "get_feature_state": lambda: session.get_feature_state(selector),
        "remove_feature_state": lambda: session.remove_feature_state(selector),
    }
    raised_errors: dict[str, BaseException] = {}

    def call_render_session_methods() -> None:
        for name, call in calls.items():
            try:
                call()
            except mln.WrongThreadError as error:
                raised_errors[name] = error

    thread = threading.Thread(target=call_render_session_methods)
    thread.start()
    thread.join()

    assert set(raised_errors) == set(calls)
    for name, error in raised_errors.items():
        assert_wrong_thread_error(error, diagnostics[name])


def test_render_session_detach_releases_python_map_reference() -> None:
    class FakeDetachedSession:
        closed = False

        def close(self) -> None:
            self.closed = True

    class FakeNativeRenderSession:
        closed = False
        detached = False

        def detach(self) -> FakeDetachedSession:
            self.detached = True
            return FakeDetachedSession()

    session = render.RenderSessionHandle._from_native(
        FakeNativeRenderSession(), object()
    )

    detached = session.detach()

    assert session.detached
    assert session._map is None
    detached.close()


def test_map_handle_context_manager_closes_once() -> None:
    with mln.RuntimeHandle() as runtime:
        with pytest.raises(TypeError, match="RuntimeHandle.create_map"):
            mln.MapHandle(runtime, mln.MapOptions(width=128, height=64))

        with runtime.create_map(mln.MapOptions(width=128, height=64)) as map_handle:
            assert not map_handle.closed
            map_handle.request_repaint()

        assert map_handle.closed
        map_handle.close()
        assert map_handle.closed


def test_map_options_accept_fast_pfor_decoding() -> None:
    with (
        mln.RuntimeHandle() as runtime,
        runtime.create_map(
            mln.MapOptions(width=64, height=64, fast_pfor_enabled=True)
        ) as map_handle,
    ):
        assert map_handle.get_size() == (64, 64, 1.0)


def test_unset_map_options_take_the_c_creation_defaults() -> None:
    # Unset fields take the C API defaults rather than values this binding
    # repeats, so mln_map_options_default() stays the single source for them.
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        assert map_handle.get_size() == (256, 256, 1.0)


def test_runtime_rejects_close_while_map_is_live() -> None:
    runtime = mln.RuntimeHandle()
    map_handle = runtime.create_map(mln.MapOptions(width=64, height=64))
    try:
        with pytest.raises(mln.InvalidStateError) as raised:
            runtime.close()

        assert raised.value.status == mln.MaplibreStatus.INVALID_STATE
        assert (
            raised.value.native_status_code
            == mln.MaplibreStatus.INVALID_STATE.native_code
        )
    finally:
        map_handle.close()
        runtime.close()


def test_still_image_request_uses_map_mode_validation() -> None:
    with (
        mln.RuntimeHandle() as runtime,
        runtime.create_map(mln.MapOptions(mode=mln.MapMode.STATIC)) as static_map,
    ):
        static_map.request_still_image()

    with (
        mln.RuntimeHandle() as runtime,
        runtime.create_map(mln.MapOptions(mode=mln.MapMode.CONTINUOUS)) as map_handle,
        pytest.raises(mln.InvalidStateError) as raised,
    ):
        map_handle.request_still_image()

    assert raised.value.status == mln.MaplibreStatus.INVALID_STATE


def test_map_create_from_closed_runtime_reports_invalid_state() -> None:
    runtime = mln.RuntimeHandle()
    runtime.close()
    stale = _native_invalid_network_status_error().diagnostic

    with pytest.raises(mln.InvalidStateError) as raised:
        runtime.create_map()

    assert raised.value.native_status_code is None
    assert raised.value.diagnostic == "runtime handle is closed"
    assert raised.value.diagnostic != stale


def test_map_debug_and_status_options_round_trip_public_values() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        debug_options = (
            map_module.MapDebugOptions.TILE_BORDERS
            | map_module.MapDebugOptions.PARSE_STATUS
        )
        map_handle.set_debug_options(debug_options)
        map_handle.set_rendering_stats_view_enabled(True)

        assert map_handle.get_debug_options() == debug_options
        assert map_handle.get_rendering_stats_view_enabled() is True
        assert isinstance(map_handle.is_fully_loaded(), bool)

        map_handle.set_debug_options(map_module.MapDebugOptions.NONE)
        map_handle.set_rendering_stats_view_enabled(False)
        assert map_handle.get_debug_options() == map_module.MapDebugOptions.NONE
        assert map_handle.get_rendering_stats_view_enabled() is False


def test_style_url_rejects_embedded_nul_before_native_call() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        stale = _native_invalid_network_status_error().diagnostic

        with pytest.raises(mln.InvalidArgumentError) as raised:
            map_handle.set_style_url("bad\0url")

    assert raised.value.status == mln.MaplibreStatus.INVALID_ARGUMENT
    assert raised.value.native_status_code is None
    assert "embedded NUL" in raised.value.diagnostic
    assert raised.value.diagnostic != stale


@pytest.mark.parametrize(
    "field",
    ("tile_size", "buffer", "cluster_radius", "cluster_min_points"),
)
def test_geojson_source_options_reject_negative_unsigned_fields(field: str) -> None:
    with pytest.raises(mln.InvalidArgumentError) as raised:
        style.GeoJsonSourceOptions(**{field: -1})

    assert raised.value.status == mln.MaplibreStatus.INVALID_ARGUMENT
    assert raised.value.native_status_code is None
    assert field in raised.value.diagnostic


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
    results: list[style.GeoJsonSourceDataHandle | BaseException] = []

    def prepare() -> None:
        try:
            results.append(style.GeoJsonSourceDataHandle(_point_collection("worker")))
        except BaseException as error:  # noqa: BLE001 - report into the test
            results.append(error)

    worker = threading.Thread(target=prepare)
    worker.start()
    worker.join()
    (prepared,) = results
    assert isinstance(prepared, style.GeoJsonSourceDataHandle)
    with prepared:
        assert prepared.closed is False
        with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
            map_handle.set_style_json(_EMPTY_STYLE_BYTES)
            map_handle.add_geojson_source_data("worker-points", prepared)
            assert (
                map_handle.get_style_source_type("worker-points")
                == style.StyleSourceType.GEOJSON
            )
    assert prepared.closed is True


def test_geojson_source_data_installs_on_many_sources_and_outlives_release() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        prepared = style.GeoJsonSourceDataHandle(_point_collection("one", "two"))
        # Install calls borrow the handle, so one prepared value serves any
        # number of sources.
        map_handle.add_geojson_source_data("points-a", prepared)
        map_handle.add_geojson_source_data("points-b", prepared)
        map_handle.set_geojson_source_data("points-a", prepared)
        prepared.close()
        # Release never invalidates a source the data was installed on.
        assert (
            map_handle.get_style_source_type("points-a")
            == style.StyleSourceType.GEOJSON
        )
        assert (
            map_handle.get_style_source_type("points-b")
            == style.StyleSourceType.GEOJSON
        )


def test_geojson_source_data_close_is_idempotent_and_blocks_installs() -> None:
    prepared = style.GeoJsonSourceDataHandle(_point_collection("one"))
    prepared.close()
    assert prepared.closed is True
    prepared.close()

    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
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
        style.GeoJsonSourceDataHandle(
            bare_geometry, style.GeoJsonSourceOptions(cluster=True)
        )


def test_set_geojson_source_data_rejects_mismatched_baked_in_options() -> None:
    document = _point_collection("one", "two")
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        with style.GeoJsonSourceDataHandle(
            document,
            style.GeoJsonSourceOptions(
                cluster=True,
                cluster_properties=_json_object({"names": ["+", 1]}),
            ),
        ) as clustered:
            map_handle.add_geojson_source_data("points", clustered)

        with (
            style.GeoJsonSourceDataHandle(document) as plain,
            pytest.raises(mln.InvalidArgumentError),
        ):
            map_handle.set_geojson_source_data("points", plain)

        # Different cluster aggregations would change cluster feature
        # properties under the source's layers, so they are rejected too.
        with (
            style.GeoJsonSourceDataHandle(
                document,
                style.GeoJsonSourceOptions(
                    cluster=True,
                    cluster_properties=_json_object({"renamed": ["+", 1]}),
                ),
            ) as reclustered,
            pytest.raises(mln.InvalidArgumentError),
        ):
            map_handle.set_geojson_source_data("points", reclustered)

        # Aggregations compare by parsed expression equality, so equivalent
        # cluster_properties JSON matches regardless of formatting.
        with style.GeoJsonSourceDataHandle(
            document,
            style.GeoJsonSourceOptions(
                cluster=True,
                cluster_properties=b' { "names" : ["+", 1] } ',
            ),
        ) as matching:
            map_handle.set_geojson_source_data("points", matching)


def test_set_geojson_source_synchronous_tiling_overrides_at_runtime() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        with style.GeoJsonSourceDataHandle(_point_collection("one")) as prepared:
            map_handle.add_geojson_source_data("points", prepared)
        map_handle.set_geojson_source_synchronous_tiling("points", True)
        map_handle.set_geojson_source_synchronous_tiling("points", False)
        with pytest.raises(mln.InvalidArgumentError):
            map_handle.set_geojson_source_synchronous_tiling("missing", True)


def test_style_source_metadata_enums_preserve_unknown_values() -> None:
    for enum_type in (
        style.TileScheme,
        style.VectorTileEncoding,
        style.RasterDemEncoding,
    ):
        value = enum_type(999_040)
        assert value.is_unknown
        assert value.native_code == 999_040


def test_loaded_style_document_and_url_read_back_what_was_loaded() -> None:
    style_json = _EMPTY_STYLE_BYTES
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        # Nothing parsed and nothing requested yet.
        assert map_handle.get_loaded_style_json() == b""
        assert map_handle.get_style_url() == ""

        # The document reads back byte-for-byte, so it can be reloaded
        # unchanged.
        map_handle.set_style_json(style_json)
        assert map_handle.get_loaded_style_json() == style_json
        # Inline JSON clears the URL.
        assert map_handle.get_style_url() == ""

        # The URL is request state, recorded before the load can succeed,
        # while the document still reports the style that last parsed.
        map_handle.set_style_url("https://example.test/style.json")
        assert map_handle.get_style_url() == "https://example.test/style.json"
        assert map_handle.get_loaded_style_json() == style_json


def test_style_source_url_metadata_and_removal_public_api() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
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
            style.GeoJsonSourceOptions(
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
        with style.GeoJsonSourceDataHandle(
            inline_points,
            style.GeoJsonSourceOptions(
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
            style.TileSourceOptions(
                min_zoom=1.0,
                max_zoom=10.0,
                vector_encoding=style.VectorTileEncoding.MVT,
            ),
        )
        map_handle.add_raster_source_url(
            "raster-tiles",
            "https://example.test/raster.json",
            style.TileSourceOptions(tile_size=256),
        )
        map_handle.add_raster_dem_source_url(
            "dem-tiles",
            "https://example.test/dem.json",
            style.TileSourceOptions(
                tile_size=512,
                raster_dem_encoding=style.RasterDemEncoding.MAPBOX,
            ),
        )
        map_handle.add_vector_source_tiles(
            "vector-inline",
            (
                "https://a.example.test/vector/{z}/{x}/{y}.mlt",
                "https://b.example.test/vector/{z}/{x}/{y}.mlt",
            ),
            style.TileSourceOptions(
                min_zoom=2.0,
                max_zoom=7.0,
                attribution="Example attribution",
                scheme=style.TileScheme.TMS,
                bounds=geo.LatLngBounds(
                    geo.LatLng(-5.0, -10.0), geo.LatLng(15.0, 20.0)
                ),
                vector_encoding=style.VectorTileEncoding.MLT,
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

        assert map_handle.style_source_exists("points") is True
        assert map_handle.style_source_exists("missing") is False
        assert (
            map_handle.get_style_source_type("style-json-points")
            == style.StyleSourceType.GEOJSON
        )
        assert (
            map_handle.get_style_source_type("points") == style.StyleSourceType.GEOJSON
        )
        assert (
            map_handle.get_style_source_type("inline-points")
            == style.StyleSourceType.GEOJSON
        )
        assert (
            map_handle.get_style_source_type("vector-tiles")
            == style.StyleSourceType.VECTOR
        )
        assert (
            map_handle.get_style_source_type("raster-tiles")
            == style.StyleSourceType.RASTER
        )
        assert (
            map_handle.get_style_source_type("dem-tiles")
            == style.StyleSourceType.RASTER_DEM
        )
        assert (
            map_handle.get_style_source_type("vector-inline")
            == style.StyleSourceType.VECTOR
        )
        assert (
            map_handle.get_style_source_type("raster-inline")
            == style.StyleSourceType.RASTER
        )
        assert (
            map_handle.get_style_source_type("dem-inline")
            == style.StyleSourceType.RASTER_DEM
        )
        assert map_handle.get_style_source_type("missing") is None
        source_ids = map_handle.list_style_source_ids()
        assert "style-json-points" in source_ids
        assert "points" in source_ids
        assert "inline-points" in source_ids
        assert "vector-tiles" in source_ids
        assert "raster-tiles" in source_ids
        assert "dem-tiles" in source_ids
        assert "vector-inline" in source_ids
        assert "raster-inline" in source_ids
        assert "dem-inline" in source_ids

        info = map_handle.get_style_source_info("points")
        assert info is not None
        assert info.source_type == style.StyleSourceType.GEOJSON
        assert info.attribution is None
        assert info.url == "https://example.test/points.geojson"
        assert info.tile_json is None
        assert map_handle.get_style_source_info("missing") is None

        remote_info = map_handle.get_style_source_info("vector-tiles")
        assert remote_info is not None
        assert remote_info.url == "https://example.test/vector.json"
        assert remote_info.tile_json is None

        copied_inline = map_handle.get_style_source_info("vector-inline")
        assert copied_inline is not None
        assert copied_inline.url is None
        assert copied_inline.attribution == "Example attribution"
        assert copied_inline.tile_size == 512
        assert copied_inline.vector_encoding == style.VectorTileEncoding.MLT
        assert copied_inline.tile_json is not None
        assert copied_inline.tile_json.tiles == (
            "https://a.example.test/vector/{z}/{x}/{y}.mlt",
            "https://b.example.test/vector/{z}/{x}/{y}.mlt",
        )
        assert copied_inline.tile_json.min_zoom == 2.0
        assert copied_inline.tile_json.max_zoom == 7.0
        assert copied_inline.tile_json.scheme == style.TileScheme.TMS
        assert copied_inline.tile_json.bounds == geo.LatLngBounds(
            geo.LatLng(-5.0, -10.0), geo.LatLng(15.0, 20.0)
        )

        assert map_handle.remove_style_source("style-json-points") is True
        assert map_handle.remove_style_source("points") is True
        assert map_handle.remove_style_source("points") is False
        assert map_handle.remove_style_source("inline-points") is True
        assert map_handle.remove_style_source("vector-tiles") is True
        assert map_handle.remove_style_source("raster-tiles") is True
        assert map_handle.remove_style_source("dem-tiles") is True
        assert map_handle.remove_style_source("vector-inline") is True
        assert map_handle.remove_style_source("raster-inline") is True
        assert map_handle.remove_style_source("dem-inline") is True
        source_ids = map_handle.list_style_source_ids()
        assert "style-json-points" not in source_ids
        assert "points" not in source_ids
        assert "inline-points" not in source_ids
        assert "vector-tiles" not in source_ids
        assert "raster-tiles" not in source_ids
        assert "dem-tiles" not in source_ids
        assert "vector-inline" not in source_ids
        assert "raster-inline" not in source_ids
        assert "dem-inline" not in source_ids
        assert copied_inline.tile_json is not None
        assert len(copied_inline.tile_json.tiles) == 2


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
    image = render.PremultipliedRgba8Image(
        info=render.TextureImageInfo(width=1, height=1, stride=4, byte_length=4),
        data=bytes([0, 255, 0, 255]),
    )

    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        map_handle.add_image_source_url(
            "overlay-url",
            coordinates,
            "https://example.test/overlay.png",
        )
        map_handle.add_image_source_image("overlay-inline", coordinates, image)

        assert (
            map_handle.get_style_source_type("overlay-url")
            == style.StyleSourceType.IMAGE
        )
        assert (
            map_handle.get_style_source_type("overlay-inline")
            == style.StyleSourceType.IMAGE
        )
        assert map_handle.get_image_source_coordinates("overlay-url") == coordinates
        assert map_handle.get_image_source_coordinates("missing") is None

        map_handle.set_image_source_url(
            "overlay-url",
            "https://example.test/overlay-2.png",
        )
        map_handle.set_image_source_image("overlay-url", image)
        map_handle.set_image_source_coordinates("overlay-url", updated_coordinates)
        assert (
            map_handle.get_image_source_coordinates("overlay-url")
            == updated_coordinates
        )

        assert map_handle.remove_style_source("overlay-url") is True
        assert map_handle.remove_style_source("overlay-inline") is True


def test_style_json_light_layer_property_and_filter_public_api() -> None:
    background = _json_object({"id": "json-background", "type": "background"})
    circle = _json_object({"id": "json-circle", "type": "circle", "source": "points"})
    raw_filter = ["==", ["get", "kind"], "park"]
    filter_value = _json_value(raw_filter)
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
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

        layer_json = map_handle.get_style_layer_json("json-background")
        assert layer_json is not None
        assert json.loads(layer_json) == {
            "id": "json-background",
            "type": "background",
            "paint": {"background-color": ["rgba", 255, 0, 0, 1]},
        }
        assert map_handle.get_style_layer_json("missing") is None
        background_color = map_handle.get_layer_property(
            "json-background",
            "background-color",
        )
        assert json.loads(background_color) == ["rgba", 255, 0, 0, 1]
        assert json.loads(map_handle.get_layer_filter("json-circle")) == raw_filter
        assert json.loads(map_handle.get_style_light_property("anchor")) == "viewport"
        assert json.loads(map_handle.get_style_light_property("intensity")) == 0.5

        with pytest.raises(mln.InvalidArgumentError, match="not valid JSON"):
            map_handle.set_style_light_property("intensity", b"Infinity")

        map_handle.set_layer_filter("json-circle", None)
        assert map_handle.get_layer_filter("json-circle") is None


def test_style_image_metadata_copy_and_removal_public_api() -> None:
    image = render.PremultipliedRgba8Image(
        info=render.TextureImageInfo(width=1, height=1, stride=4, byte_length=4),
        data=bytes([255, 0, 0, 255]),
    )
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        map_handle.set_style_image(
            "marker",
            image,
            style.StyleImageOptions(pixel_ratio=2.0, sdf=True),
        )

        assert map_handle.style_image_exists("marker") is True
        assert map_handle.style_image_exists("missing") is False
        info = map_handle.get_style_image_info("marker")
        assert info is not None
        assert info.width == 1
        assert info.height == 1
        assert info.stride == 4
        assert info.byte_length == 4
        assert info.pixel_ratio == pytest.approx(2.0)
        assert info.sdf is True
        assert map_handle.get_style_image_info("missing") is None

        copied = map_handle.copy_style_image_premultiplied_rgba8("marker")
        assert copied is not None
        assert copied.image == image
        assert copied.pixel_ratio == pytest.approx(2.0)
        assert copied.sdf is True
        assert map_handle.copy_style_image_premultiplied_rgba8("missing") is None

        assert map_handle.remove_style_image("marker") is True
        assert map_handle.remove_style_image("marker") is False


def test_builtin_style_layers_and_location_indicator_public_api() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        map_handle.add_raster_dem_source_url(
            "dem",
            "https://example.test/dem.json",
            style.TileSourceOptions(
                tile_size=512,
                raster_dem_encoding=style.RasterDemEncoding.MAPBOX,
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

        assert map_handle.get_style_layer_type("hillshade") == "hillshade"
        assert map_handle.get_style_layer_type("relief") == "color-relief"
        assert map_handle.get_style_layer_type("location") == "location-indicator"
        assert map_handle.remove_style_layer("hillshade") is True
        assert map_handle.remove_style_layer("relief") is True
        assert map_handle.remove_style_layer("location") is True


def test_nine_patch_style_image_round_trips_public_api() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        image = render.PremultipliedRgba8Image(
            render.TextureImageInfo(width=2, height=2, stride=8, byte_length=16),
            bytes(16),
        )
        options = style.StyleImageOptions(
            stretch_x=(style.ImageStretch(0.0, 1.0),),
            stretch_y=(style.ImageStretch(0.0, 1.0), style.ImageStretch(1.0, 2.0)),
            content=style.ImageContent(0.5, 0.5, 1.5, 1.5),
            text_fit_height=style.StyleImageTextFit.PROPORTIONAL,
        )
        map_handle.set_style_image("patch", image, options)

        info = map_handle.get_style_image_info("patch")
        assert info is not None
        assert info.stretch_x_count == 1
        assert info.stretch_y_count == 2
        assert info.content == style.ImageContent(0.5, 0.5, 1.5, 1.5)
        # An absent text fit stays distinguishable from a present default.
        assert info.text_fit_width is None
        assert info.text_fit_height is style.StyleImageTextFit.PROPORTIONAL

        stretches = map_handle.get_style_image_stretches("patch")
        assert stretches is not None
        stretch_x, stretch_y = stretches
        assert stretch_x == (style.ImageStretch(0.0, 1.0),)
        assert stretch_y == (
            style.ImageStretch(0.0, 1.0),
            style.ImageStretch(1.0, 2.0),
        )
        assert map_handle.get_style_image_stretches("missing") is None

        # A backwards interval is rejected by C.
        with pytest.raises(mln.InvalidArgumentError):
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
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        # A map with no style yet reports no duration or delay. The
        # placement flag always reports, because native always holds one.
        empty = map_handle.get_style_transition_options()
        assert empty.duration_ms is None
        assert empty.delay_ms is None
        assert empty.enable_placement_transitions is True

        # The style parser fills in its own 300ms duration for a style that
        # declares no transition.
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        parsed = map_handle.get_style_transition_options()
        assert parsed.duration_ms == 300.0
        assert parsed.delay_ms is None

        map_handle.set_style_json(transition_style_json)
        declared = map_handle.get_style_transition_options()
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
        assert map_handle.get_style_transition_options() == options

        # Omitting the flag leaves the cross-fade on rather than clearing it.
        map_handle.set_style_transition_options(
            style.StyleTransitionOptions(duration_ms=250.0)
        )
        assert (
            map_handle.get_style_transition_options().enable_placement_transitions
            is True
        )

        # Loading a style replaces the override with what that style declares.
        map_handle.set_style_json(transition_style_json)
        assert map_handle.get_style_transition_options() == declared

        with pytest.raises(mln.InvalidArgumentError, match="delay_ms"):
            map_handle.set_style_transition_options(
                style.StyleTransitionOptions(delay_ms=-1.0)
            )


def test_layer_base_accessors_round_trip_public_api() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.set_style_json(
            b'{"version":8,"sources":{"geo":{"type":"geojson","data":'
            b'{"type":"FeatureCollection","features":[]}}},"layers":['
            b'{"id":"bg","type":"background"},'
            b'{"id":"fill","type":"fill","source":"geo"}]}'
        )

        assert map_handle.get_layer_source_layer("fill") == ""
        map_handle.set_layer_source_layer("fill", "roads")
        assert map_handle.get_layer_source_layer("fill") == "roads"
        assert map_handle.get_layer_source_id("fill") == "geo"

        # A layer type that takes no source is rejected, not silently ignored.
        with pytest.raises(mln.InvalidArgumentError, match="source-layer"):
            map_handle.set_layer_source_layer("bg", "roads")
        assert map_handle.get_layer_source_id("bg") == ""

        # An unset zoom range crosses the boundary as infinities.
        assert map_handle.get_layer_min_zoom("fill") == -math.inf
        assert map_handle.get_layer_max_zoom("fill") == math.inf
        map_handle.set_layer_min_zoom("fill", 4.0)
        map_handle.set_layer_max_zoom("fill", 12.5)
        assert map_handle.get_layer_min_zoom("fill") == 4.0
        assert map_handle.get_layer_max_zoom("fill") == 12.5

        assert (
            map_handle.get_layer_visibility("fill")
            is style.StyleLayerVisibility.VISIBLE
        )
        map_handle.set_layer_visibility("fill", style.StyleLayerVisibility.NONE)
        assert (
            map_handle.get_layer_visibility("fill") is style.StyleLayerVisibility.NONE
        )

        with pytest.raises(mln.InvalidArgumentError):
            map_handle.get_layer_min_zoom("missing")


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
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.set_style_json(style_json)

        layer_ids = map_handle.list_style_layer_ids()
        assert "background-a" in layer_ids
        assert "background-b" in layer_ids
        assert layer_ids.index("background-a") < layer_ids.index("background-b")
        assert map_handle.style_layer_exists("background-a") is True
        assert map_handle.style_layer_exists("missing") is False
        assert map_handle.get_style_layer_type("background-a") == "background"
        assert map_handle.get_style_layer_type("missing") is None

        map_handle.move_style_layer("background-b", "background-a")
        layer_ids = map_handle.list_style_layer_ids()
        assert layer_ids.index("background-b") < layer_ids.index("background-a")

        assert map_handle.remove_style_layer("background-b") is True
        assert map_handle.remove_style_layer("background-b") is False
        assert "background-b" not in map_handle.list_style_layer_ids()


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

    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.set_viewport_options(viewport)
        map_handle.set_tile_options(tile)

        assert map_handle.get_viewport_options() == viewport
        assert map_handle.get_tile_options() == tile


def test_camera_snapshot_and_jump_round_trip_public_values() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        target = camera.CameraOptions(
            center=geo.LatLng(10.0, 20.0),
            zoom=2.0,
            bearing=15.0,
            pitch=10.0,
            padding=camera.EdgeInsets(top=1.0, left=2.0, bottom=3.0, right=4.0),
            anchor=camera.ScreenPoint(x=16.0, y=8.0),
        )
        map_handle.jump_to(target)
        snapshot = map_handle.get_camera()
        map_handle.move_by(1.0, 1.0)
        map_handle.cancel_transitions()

        assert snapshot.center is not None
        assert snapshot.center.latitude == pytest.approx(10.0)
        assert snapshot.center.longitude == pytest.approx(20.0)
        assert snapshot.zoom == pytest.approx(2.0)
        assert snapshot.bearing == pytest.approx(15.0)
        assert snapshot.pitch == pytest.approx(10.0)
        assert snapshot.padding == target.padding


def test_gesture_in_progress_brackets_host_driven_camera_commands() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        assert map_handle.is_gesture_in_progress() is False

        map_handle.set_gesture_in_progress(True)
        map_handle.move_by(8.0, -4.0)
        assert map_handle.is_gesture_in_progress() is True

        map_handle.set_gesture_in_progress(False)
        assert map_handle.is_gesture_in_progress() is False


def test_free_camera_and_projection_mode_round_trip_public_values() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        free_camera = map_handle.get_free_camera_options()
        assert isinstance(free_camera, camera.FreeCameraOptions)

        projection = camera.ProjectionMode(
            axonometric=True,
            x_skew=0.1,
            y_skew=0.2,
        )
        map_handle.set_projection_mode(projection)
        snapshot = map_handle.get_projection_mode()

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

    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.set_bounds(
            camera.BoundOptions(
                bounds=camera.Bounded(bounds),
                min_zoom=0.0,
                max_zoom=10.0,
            )
        )
        constraints = map_handle.get_bounds()
        fit_bounds = map_handle.camera_for_lat_lng_bounds(bounds, fit)
        fit_coordinates = map_handle.camera_for_lat_lngs(
            (bounds.southwest, bounds.northeast),
            fit,
        )
        fit_geometry = map_handle.camera_for_geometry(
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
        visible_bounds = map_handle.lat_lng_bounds_for_camera(target)
        unwrapped_bounds = map_handle.lat_lng_bounds_for_camera(
            target,
            unwrapped=True,
        )

        assert constraints.bounds == camera.Bounded(bounds)
        assert constraints.min_zoom == pytest.approx(0.0)
        assert constraints.max_zoom == pytest.approx(10.0)
        assert isinstance(fit_bounds, camera.CameraOptions)
        assert isinstance(fit_coordinates, camera.CameraOptions)
        assert isinstance(fit_geometry, camera.CameraOptions)
        assert isinstance(visible_bounds, geo.LatLngBounds)
        assert isinstance(unwrapped_bounds, geo.LatLngBounds)


def _jumped_longitude(map_handle: mln.MapHandle, longitude: float) -> float:
    map_handle.jump_to(
        camera.CameraOptions(center=geo.LatLng(0.0, longitude), zoom=2.0)
    )
    center = map_handle.get_camera().center
    assert center is not None
    return center.longitude


def test_camera_bounds_distinguish_unbounded_from_world() -> None:
    world = geo.LatLngBounds(
        southwest=geo.LatLng(-90.0, -180.0),
        northeast=geo.LatLng(90.0, 180.0),
    )

    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        assert map_handle.get_bounds().bounds == camera.Unbounded()
        # An unbounded map wraps across the antimeridian.
        assert _jumped_longitude(map_handle, 200.0) == pytest.approx(-160.0, abs=1e-6)

        map_handle.set_bounds(camera.BoundOptions(bounds=camera.Bounded(world)))

        constrained = map_handle.get_bounds().bounds
        assert isinstance(constrained, camera.Bounded)
        assert constrained.bounds.northeast.longitude == pytest.approx(180.0)
        # World bounds clamp at the antimeridian instead of wrapping.
        assert _jumped_longitude(map_handle, 200.0) == pytest.approx(180.0, abs=1e-6)

        map_handle.set_bounds(camera.BoundOptions(bounds=camera.Unbounded()))

        assert map_handle.get_bounds().bounds == camera.Unbounded()
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
    first = camera.ScreenPoint(0.0, 0.0)
    second = camera.ScreenPoint(1.0, 1.0)

    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.ease_to(target, animation)
        map_handle.fly_to(target, animation)
        map_handle.move_by_animated(1.0, 1.0, animation)
        map_handle.scale_by(1.0, first)
        map_handle.scale_by_animated(1.0, first, animation)
        map_handle.rotate_by(first, second)
        map_handle.rotate_by_animated(first, second, animation)
        map_handle.pitch_by(0.0)
        map_handle.pitch_by_animated(0.0, animation)
        map_handle.cancel_transitions()


def _drain_runtime_events(runtime: mln.RuntimeHandle) -> list[mln.RuntimeEvent]:
    return runtime.drain_events().events


def _finished_transition_ids(events: list[mln.RuntimeEvent]) -> list[int]:
    ids: list[int] = []
    for event in events:
        if event.event_type != mln.RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED:
            continue
        assert isinstance(event.payload, mln.CameraTransitionFinishedPayload)
        ids.append(event.payload.transition_id)
    return ids


def _camera_change_modes(
    events: list[mln.RuntimeEvent], event_type: mln.RuntimeEventType
) -> list[mln.CameraChangeMode]:
    return [
        mln.CameraChangeMode(event.code)
        for event in events
        if event.event_type == event_type
    ]


def test_zero_duration_ease_reports_transition_finished_once() -> None:
    target = camera.CameraOptions(center=geo.LatLng(12.0, 34.0), zoom=4.0)

    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        _drain_runtime_events(runtime)
        map_handle.ease_to(
            target,
            camera.AnimationOptions(duration_ms=0.0, transition_id=101),
        )
        events = _drain_runtime_events(runtime)

    assert _finished_transition_ids(events) == [101]
    assert _camera_change_modes(events, mln.RuntimeEventType.MAP_CAMERA_DID_CHANGE) == [
        mln.CameraChangeMode.IMMEDIATE
    ]


def test_superseded_transition_reports_transition_finished_once() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        _drain_runtime_events(runtime)
        map_handle.ease_to(
            camera.CameraOptions(center=geo.LatLng(20.0, 40.0), zoom=6.0),
            camera.AnimationOptions(duration_ms=5_000.0, transition_id=201),
        )
        started = _drain_runtime_events(runtime)
        map_handle.jump_to(
            camera.CameraOptions(center=geo.LatLng(-20.0, -40.0), zoom=2.0)
        )
        superseded = _drain_runtime_events(runtime)

    assert _finished_transition_ids(started) == []
    assert _finished_transition_ids(superseded) == [201]


def test_cancelled_transition_reports_transition_finished_once() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        _drain_runtime_events(runtime)
        map_handle.fly_to(
            camera.CameraOptions(center=geo.LatLng(30.0, 60.0), zoom=8.0),
            camera.AnimationOptions(duration_ms=5_000.0, transition_id=301),
        )
        started = _drain_runtime_events(runtime)
        map_handle.cancel_transitions()
        cancelled = _drain_runtime_events(runtime)
        map_handle.cancel_transitions()
        cancelled_again = _drain_runtime_events(runtime)

    assert _finished_transition_ids(started) == []
    assert _finished_transition_ids(cancelled) == [301]
    assert _finished_transition_ids(cancelled_again) == []


def test_completed_ease_reports_transition_finished_once() -> None:
    target = camera.CameraOptions(center=geo.LatLng(5.0, 10.0), zoom=3.0)
    deadline = time.monotonic() + 10.0
    events: list[mln.RuntimeEvent] = []

    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        _drain_runtime_events(runtime)
        map_handle.ease_to(
            target,
            camera.AnimationOptions(duration_ms=20.0, transition_id=401),
        )
        while not _finished_transition_ids(events):
            assert time.monotonic() < deadline, (
                "ease did not finish while the runtime was pumped"
            )
            map_handle.request_repaint()
            runtime.pump()
            events.extend(_drain_runtime_events(runtime))

        trailing: list[mln.RuntimeEvent] = []
        for _ in range(8):
            map_handle.request_repaint()
            runtime.pump()
            trailing.extend(_drain_runtime_events(runtime))

    assert _finished_transition_ids(events) == [401]
    assert _finished_transition_ids(trailing) == []
    assert mln.CameraChangeMode.ANIMATED in _camera_change_modes(
        events, mln.RuntimeEventType.MAP_CAMERA_DID_CHANGE
    )


def test_camera_change_events_report_immediate_and_animated_modes() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        _drain_runtime_events(runtime)
        map_handle.jump_to(camera.CameraOptions(center=geo.LatLng(1.0, 2.0), zoom=3.0))
        jumped = _drain_runtime_events(runtime)
        map_handle.ease_to(
            camera.CameraOptions(center=geo.LatLng(-1.0, -2.0), zoom=7.0),
            camera.AnimationOptions(duration_ms=5_000.0),
        )
        eased = _drain_runtime_events(runtime)
        map_handle.cancel_transitions()
        eased.extend(_drain_runtime_events(runtime))

    will_change = mln.RuntimeEventType.MAP_CAMERA_WILL_CHANGE
    did_change = mln.RuntimeEventType.MAP_CAMERA_DID_CHANGE
    assert _camera_change_modes(jumped, will_change) == [mln.CameraChangeMode.IMMEDIATE]
    assert _camera_change_modes(jumped, did_change) == [mln.CameraChangeMode.IMMEDIATE]
    assert _camera_change_modes(eased, will_change) == [mln.CameraChangeMode.ANIMATED]
    assert _camera_change_modes(eased, did_change) == [mln.CameraChangeMode.ANIMATED]


def test_transition_finished_precedes_camera_did_change_in_one_batch() -> None:
    """BND-090: one drain reports more than one event and preserves queue order."""
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        runtime.drain_events()
        map_handle.ease_to(
            camera.CameraOptions(center=geo.LatLng(12.0, 34.0), zoom=4.0),
            camera.AnimationOptions(duration_ms=0.0, transition_id=707),
        )
        types = [event.event_type for event in runtime.drain_events().events]

    finished = mln.RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED
    assert types.count(finished) == 1
    assert types.index(finished) < types.index(
        mln.RuntimeEventType.MAP_CAMERA_DID_CHANGE
    )


@pytest.mark.parametrize("max_events", [-1, 2**64])
def test_drain_events_out_of_range_bound_raises_binding_error(max_events: int) -> None:
    # PyO3 extracts this as `usize`, so without a range check the caller would
    # see a bare OverflowError instead of the binding's error shape.
    with mln.RuntimeHandle() as runtime, pytest.raises(mln.InvalidArgumentError):
        runtime.drain_events(max_events)


def test_option_default_masks_keep_native_bits_this_build_does_not_name(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """BND-091: the option defaults copy the C default mask, unnamed bits included."""
    native_default = int(mln.RuntimeEventMask.ALL) | 1 << 62
    monkeypatch.setattr(
        _native, "runtime_options_default_event_mask", lambda: native_default
    )
    monkeypatch.setattr(
        _native, "map_options_default_event_mask", lambda: native_default
    )

    assert int(mln.RuntimeOptions().event_mask) == native_default
    assert int(mln.MapOptions().event_mask) == native_default


def test_narrowed_map_mask_drops_the_cleared_event_type() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.set_event_mask(
            mln.RuntimeEventMask.ALL & ~mln.RuntimeEventMask.MAP_CAMERA_DID_CHANGE
        )
        runtime.drain_events()
        map_handle.jump_to(camera.CameraOptions(center=geo.LatLng(1.0, 2.0), zoom=3.0))
        types = [event.event_type for event in runtime.drain_events().events]

    assert mln.RuntimeEventType.MAP_CAMERA_WILL_CHANGE in types
    assert mln.RuntimeEventType.MAP_CAMERA_DID_CHANGE not in types


def test_map_created_with_a_narrowed_mask_never_delivers_the_cleared_type() -> None:
    narrowed = mln.RuntimeEventMask.ALL & ~mln.RuntimeEventMask.MAP_CAMERA_DID_CHANGE
    options = mln.MapOptions(event_mask=narrowed)

    with mln.RuntimeHandle() as runtime, runtime.create_map(options) as map_handle:
        assert map_handle.event_mask == narrowed
        runtime.drain_events()
        map_handle.jump_to(camera.CameraOptions(center=geo.LatLng(1.0, 2.0), zoom=3.0))
        types = [event.event_type for event in runtime.drain_events().events]

    assert mln.RuntimeEventType.MAP_CAMERA_WILL_CHANGE in types
    assert mln.RuntimeEventType.MAP_CAMERA_DID_CHANGE not in types


def test_narrowed_runtime_mask_drops_offline_completion_events(tmp_path: Path) -> None:
    cleared = mln.RuntimeEventMask.ALL & ~(
        mln.RuntimeEventMask.OFFLINE_OPERATION_COMPLETED
    )

    with mln.RuntimeHandle(
        mln.RuntimeOptions(cache_path=str(tmp_path / "masked-cache.db"))
    ) as runtime:
        # The same drive reports a completion while the type is selected, so the
        # narrowed phase below asserts a real negative.
        selected = runtime.set_maximum_ambient_cache_size(8 << 20)
        _wait_for_offline_operation(runtime, selected)
        selected.close()

        runtime.set_event_mask(cleared)
        assert runtime.event_mask == cleared
        operation = runtime.set_maximum_ambient_cache_size(4 << 20)
        completions = 0
        for _ in range(200):
            runtime.pump(0.002)
            completions += sum(
                event.event_type == mln.RuntimeEventType.OFFLINE_OPERATION_COMPLETED
                for event in runtime.drain_events().events
            )
        operation.close()

    assert completions == 0


def test_event_masks_round_trip_on_the_map_and_the_runtime() -> None:
    created = mln.RuntimeEventMask.ALL & ~(
        mln.RuntimeEventMask.OFFLINE_REGION_STATUS_CHANGED
    )

    with mln.RuntimeHandle(mln.RuntimeOptions(event_mask=created)) as runtime:
        assert runtime.event_mask == created
        runtime.set_event_mask(mln.RuntimeEventMask.ALL)
        assert runtime.event_mask == mln.RuntimeEventMask.ALL

        with runtime.create_map() as map_handle:
            map_handle.set_event_mask(mln.RuntimeEventMask.ALL)
            assert map_handle.event_mask == mln.RuntimeEventMask.ALL

            # A read-modify-write of one bit keeps every other bit, including the
            # runtime bits a map ignores.
            map_handle.set_event_mask(
                map_handle.event_mask & ~mln.RuntimeEventMask.MAP_IDLE
            )
            assert (
                map_handle.event_mask
                == mln.RuntimeEventMask.ALL & ~mln.RuntimeEventMask.MAP_IDLE
            )
            assert mln.RuntimeEventMask.MAP_TILE_ACTION in map_handle.event_mask
            assert (
                mln.RuntimeEventMask.OFFLINE_OPERATION_COMPLETED
                in map_handle.event_mask
            )

            runtime.set_event_mask(
                runtime.event_mask & ~mln.RuntimeEventMask.OFFLINE_OPERATION_COMPLETED
            )
            assert (
                runtime.event_mask
                == mln.RuntimeEventMask.ALL
                & ~mln.RuntimeEventMask.OFFLINE_OPERATION_COMPLETED
            )


def test_empty_creation_mask_queues_nothing() -> None:
    # An empty mask is a selection of no types, not an absent selection.
    options = mln.RuntimeOptions(event_mask=mln.RuntimeEventMask.NONE)
    map_options = mln.MapOptions(event_mask=mln.RuntimeEventMask.NONE)

    with mln.RuntimeHandle(options) as runtime:
        assert runtime.event_mask == mln.RuntimeEventMask.NONE
        with runtime.create_map(map_options) as map_handle:
            assert map_handle.event_mask == mln.RuntimeEventMask.NONE
            assert not runtime.drain_events().events
            map_handle.set_style_json(_EMPTY_STYLE_BYTES)
            for _ in range(64):
                runtime.pump()
                time.sleep(0.001)

            assert not runtime.drain_events().events


def test_event_mask_bit_outside_all_is_rejected() -> None:
    outside = mln.RuntimeEventMask(1 << 40)

    with pytest.raises(mln.InvalidArgumentError):
        mln.RuntimeHandle(mln.RuntimeOptions(event_mask=outside))

    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        with pytest.raises(mln.InvalidArgumentError):
            runtime.create_map(mln.MapOptions(event_mask=outside))
        with pytest.raises(mln.InvalidArgumentError):
            runtime.set_event_mask(outside)
        with pytest.raises(mln.InvalidArgumentError):
            map_handle.set_event_mask(outside)


def test_bounded_drain_reports_remaining_count_until_the_queue_empties() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        runtime.drain_events()
        map_handle.jump_to(camera.CameraOptions(center=geo.LatLng(1.0, 2.0), zoom=3.0))
        map_handle.jump_to(camera.CameraOptions(center=geo.LatLng(3.0, 4.0), zoom=5.0))

        bounded = runtime.drain_events(1)
        assert len(bounded.events) == 1
        assert bounded.remaining_count > 0

        # Nothing pumps between the two drains, so the rest is exactly what the
        # bounded drain left behind.
        rest = runtime.drain_events()
        assert len(rest.events) == bounded.remaining_count
        assert rest.remaining_count == 0
        assert not runtime.drain_events().events


def test_drained_events_stay_equal_after_the_next_drain_and_map_close() -> None:
    with mln.RuntimeHandle() as runtime:
        map_handle = runtime.create_map()
        map_handle.jump_to(camera.CameraOptions(center=geo.LatLng(1.0, 2.0), zoom=3.0))
        first = runtime.drain_events()
        snapshot = list(first.events)
        assert snapshot

        map_handle.jump_to(camera.CameraOptions(center=geo.LatLng(4.0, 5.0), zoom=6.0))
        assert runtime.drain_events().events

        assert first == mln.RuntimeEventBatch(events=snapshot, remaining_count=0)
        map_handle.close()
        assert first.events == snapshot


def test_drain_steps_by_the_reported_event_size() -> None:
    # The decoder indexes a batch by the stride the batch reports, so a library
    # that widened the event record would misdecode if the two ever diverged.
    with mln.RuntimeHandle() as runtime:
        reported, compiled = _native.runtime_event_stride_for_test(runtime._native)

    assert reported == compiled


def test_drain_returns_a_copied_map_loading_failure() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        with pytest.raises((mln.InvalidArgumentError, mln.NativeError)):
            map_handle.set_style_json(b"{")

        failures = [
            event
            for event in runtime.drain_events().events
            if event.event_type == mln.RuntimeEventType.MAP_LOADING_FAILED
        ]

        assert failures
        loading_failed = failures[0]
        assert loading_failed.source.source_type == mln.RuntimeEventSourceType.MAP
        assert loading_failed.source.map_handle is map_handle
        assert loading_failed.source.source_id != 0
        assert loading_failed.message


def test_pump_and_drain_return_a_copied_style_loaded_event() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        style_loaded = _wait_for_runtime_event(
            runtime, mln.RuntimeEventType.MAP_STYLE_LOADED
        )

        assert style_loaded.source.source_type == mln.RuntimeEventSourceType.MAP
        assert style_loaded.source.map_handle is map_handle
        assert style_loaded.payload is None
        runtime.pump()
        assert not runtime.drain_events().events


def test_synthetic_batch_decodes_every_payload_and_preserves_unknown_domains() -> None:
    """BND-083: unknown event, source, and payload domains keep their raw values."""
    batch = mln.RuntimeEventBatch._from_native(
        _native.synthetic_runtime_event_batch_for_test()
    )
    render_frame, tile_action, image_missing, region_status, transition, unknown = (
        batch.events
    )

    assert batch.remaining_count == 3

    assert isinstance(render_frame.payload, mln.RenderFramePayload)
    assert render_frame.payload.mode == mln.RenderMode.FULL
    assert render_frame.payload.needs_repaint is True
    assert render_frame.payload.placement_changed is False
    assert render_frame.payload.stats.encoding_time == pytest.approx(1.25)
    assert render_frame.payload.stats.rendering_time == pytest.approx(2.5)
    assert render_frame.payload.stats.frame_count == 3
    assert render_frame.payload.stats.draw_call_count == 4
    assert render_frame.payload.stats.total_draw_call_count == 5

    assert isinstance(tile_action.payload, mln.TileActionPayload)
    assert tile_action.payload.operation == mln.TileOperation.LOAD_FROM_NETWORK
    assert tile_action.payload.tile_id == mln.TileId(
        overscaled_z=6, wrap=-1, canonical_z=5, canonical_x=12, canonical_y=34
    )
    # The source ID lives once, in the event message arena.
    assert tile_action.message == "source-a"
    # The event names a map that no live wrapper resolves, and the raw identity
    # the C API delivered stays readable anyway.
    assert tile_action.source.source_type == mln.RuntimeEventSourceType.MAP
    assert tile_action.source.map_handle is None
    assert tile_action.source.source_id == 0x0100_0000_0000_0007

    assert image_missing.event_type == mln.RuntimeEventType.MAP_STYLE_IMAGE_MISSING
    assert image_missing.payload is None
    assert image_missing.message == "missing-image"

    status_changed = region_status.payload
    assert isinstance(status_changed, offline.OfflineRegionStatusChanged)
    assert status_changed.region_id == 42
    assert (
        status_changed.status.download_state
        == offline.OfflineRegionDownloadState.ACTIVE
    )
    assert status_changed.status.completed_resource_count == 7
    assert status_changed.status.completed_resource_size == 8
    assert status_changed.status.completed_tile_count == 9
    assert status_changed.status.required_tile_count == 10
    assert status_changed.status.completed_tile_size == 11
    assert status_changed.status.required_resource_count == 12
    assert status_changed.status.required_resource_count_is_precise is True
    assert status_changed.status.complete is False

    assert transition.payload == mln.CameraTransitionFinishedPayload(transition_id=909)

    assert unknown.event_type.is_unknown
    assert int(unknown.event_type) == 999_001
    assert unknown.source.source_type.is_unknown
    assert int(unknown.source.source_type) == 999_003
    assert unknown.source.source_id == 0x0200_0000_0000_002A
    assert unknown.source.map_handle is None
    assert unknown.code == -7
    assert unknown.message == "future payload"
    assert isinstance(unknown.payload, mln.UnknownRuntimeEventPayload)
    assert unknown.payload.raw_type == 999_002
    # The helper overwrites its event array and arena before returning, so these
    # bytes and this text only survive because the decode copied them.
    assert unknown.payload.data[:4] == b"\x01\x02\x03\x04"
    assert set(unknown.payload.data[4:]) == {0}


def test_render_descriptors_are_public_python_values() -> None:
    extent = render.RenderTargetExtent(width=320, height=240, scale_factor=2.0)
    pointer = render.NativePointer(0x1234)
    metal = render.MetalOwnedTextureDescriptor(
        extent=extent,
        context=render.MetalContextDescriptor(device=pointer),
    )
    vulkan = render.VulkanBorrowedTextureDescriptor(
        extent=extent,
        physical_width=641,
        physical_height=481,
        context=render.VulkanContextDescriptor(
            graphics_queue_family_index=7,
            get_instance_proc_addr=render.NativePointer(0x1111),
            get_device_proc_addr=render.NativePointer(0x2222),
        ),
        image=pointer,
        image_view=render.NativePointer(0x5678),
        format=44,
        initial_layout=1,
        final_layout=2,
    )

    assert metal.extent == extent
    assert metal.context.device.address == 0x1234
    opengl_egl = render.OpenGLOwnedTextureDescriptor(
        extent=extent,
        context=render.EglContextDescriptor(
            display=pointer,
            config=render.NativePointer(0x7777),
            share_context=render.NativePointer(0x8888),
            get_proc_address=render.NativePointer(0x9999),
        ),
    )
    opengl_wgl = render.OpenGLBorrowedTextureDescriptor(
        extent=extent,
        physical_width=641,
        physical_height=481,
        context=render.WglContextDescriptor(
            device_context=pointer,
            share_context=render.NativePointer(0x8888),
            get_proc_address=render.NativePointer(0x9999),
        ),
        texture=5,
        target=0x0DE1,
    )

    # Odd sizes no logical extent maps onto, so these must survive as stated.
    assert (vulkan.physical_width, vulkan.physical_height) == (641, 481)
    assert (opengl_wgl.physical_width, opengl_wgl.physical_height) == (641, 481)
    assert vulkan.context.graphics_queue_family_index == 7
    assert vulkan.context.get_instance_proc_addr.address == 0x1111
    assert vulkan.context.get_device_proc_addr.address == 0x2222
    assert vulkan.image_view.address == 0x5678
    assert vulkan.format == 44
    assert opengl_egl.context.display.address == 0x1234
    assert opengl_egl.context.config.address == 0x7777
    assert opengl_wgl.context.device_context.address == 0x1234
    assert opengl_wgl.texture == 5
    assert opengl_wgl.target == 0x0DE1


def test_render_session_query_public_api_returns_queried_features() -> None:
    class FakeNativeRenderSession:
        closed = False
        detached = False

        def __init__(self) -> None:
            self.rendered_call = None
            self.source_call = None
            self.extension_call = None

        def query_rendered_features(
            self,
            geometry: object,
            layer_ids: tuple[str, ...] | None,
            filter_: object,
        ) -> list[dict[str, object]]:
            self.rendered_call = (geometry, layer_ids, filter_)
            return [queried_feature_wire]

        def query_source_features(
            self,
            source_id: str,
            source_layer_ids: tuple[str, ...] | None,
            filter_: object,
        ) -> list[dict[str, object]]:
            self.source_call = (source_id, source_layer_ids, filter_)
            return [queried_feature_wire]

        def query_feature_extensions(
            self,
            source_id: str,
            feature: object,
            extension: str,
            extension_field: str,
            arguments: object,
        ) -> bytes:
            self.extension_call = (
                source_id,
                feature,
                extension,
                extension_field,
                arguments,
            )
            return _json_value(7)

    feature = _json_object(
        {
            "type": "Feature",
            "geometry": {"type": "Point", "coordinates": [2.0, 1.0]},
            "properties": {"name": "one"},
            "id": "feature-1",
        }
    )
    state = _json_object({"hover": True})
    queried_feature_wire = {
        "feature": feature,
        "source_id": "points",
        "source_layer_id": None,
        "state": state,
    }
    queried = query.QueriedFeature(
        feature=feature,
        source_id="points",
        source_layer_id=None,
        state=state,
    )

    fake_native = FakeNativeRenderSession()
    with pytest.raises(TypeError, match="created by MapHandle"):
        render.RenderSessionHandle(fake_native, object())

    session = render.RenderSessionHandle._from_native(fake_native, object())
    geometry = query.RenderedQueryGeometry.point_geometry(camera.ScreenPoint(1.0, 2.0))
    rendered_options = query.RenderedFeatureQueryOptions(
        layer_ids=("circle",),
        filter=_json_value(["==", ["get", "kind"], "park"]),
    )
    source_options = query.SourceFeatureQueryOptions(
        source_layer_ids=("landuse",),
        filter=_json_value(["==", ["get", "kind"], "park"]),
    )
    rendered = session.query_rendered_features(geometry, rendered_options)
    source = session.query_source_features("points", source_options)
    leaves_arguments = _json_object({"limit": 10})
    extension = session.query_feature_extensions(
        "points",
        feature,
        "supercluster",
        "leaves",
        leaves_arguments,
    )

    assert fake_native.rendered_call == (
        {"type": "point", "point": (1.0, 2.0)},
        ("circle",),
        _json_value(["==", ["get", "kind"], "park"]),
    )
    assert fake_native.source_call[0] == "points"
    assert fake_native.source_call[1] == ("landuse",)
    assert rendered == [queried]
    assert source == [queried]
    assert fake_native.extension_call == (
        "points",
        feature,
        "supercluster",
        "leaves",
        leaves_arguments,
    )
    assert extension == _json_value(7)


def test_opengl_owned_texture_frame_public_api_uses_native_values() -> None:
    class FakeNativeFrame:
        closed = False

        def frame(self) -> dict[str, object]:
            return {
                "generation": 1,
                "width": 64,
                "height": 32,
                "scale_factor": 2.0,
                "frame_id": 3,
                "target": 0x0DE1,
                "internal_format": 0x8058,
                "format": 0x1908,
                "type": 0x1401,
            }

        def texture(self) -> int:
            return 5

        def close(self) -> None:
            self.closed = True

    frame = render.OpenGLOwnedTextureFrameHandle._from_native(FakeNativeFrame())

    assert frame.frame == render.OpenGLOwnedTextureFrame(
        generation=1,
        width=64,
        height=32,
        scale_factor=2.0,
        frame_id=3,
        target=0x0DE1,
        internal_format=0x8058,
        format=0x1908,
        type=0x1401,
    )
    assert frame.texture == 5
    frame.close()
    assert frame.closed


def test_render_session_feature_state_public_api_uses_json_buffers() -> None:
    class FakeNativeRenderSession:
        closed = False
        detached = False

        def __init__(self) -> None:
            self.set_call = None
            self.remove_call = None

        def set_feature_state(
            self,
            source_id: str,
            source_layer_id: str | None,
            feature_id: str | None,
            state_key: str | None,
            state: object,
        ) -> None:
            self.set_call = (
                source_id,
                source_layer_id,
                feature_id,
                state_key,
                state,
            )

        def get_feature_state(
            self,
            source_id: str,
            source_layer_id: str | None,
            feature_id: str | None,
            state_key: str | None,
        ) -> bytes:
            assert (source_id, source_layer_id, feature_id, state_key) == (
                "points",
                "symbols",
                "feature-1",
                "hover",
            )
            return _json_object({"hover": True})

        def remove_feature_state(
            self,
            source_id: str,
            source_layer_id: str | None,
            feature_id: str | None,
            state_key: str | None,
        ) -> None:
            self.remove_call = (source_id, source_layer_id, feature_id, state_key)

    fake_native = FakeNativeRenderSession()
    session = render.RenderSessionHandle._from_native(fake_native, object())
    selector = query.FeatureStateSelector(
        source_id="points",
        source_layer_id="symbols",
        feature_id="feature-1",
        state_key="hover",
    )
    state = _json_object({"hover": True})

    session.set_feature_state(selector, state)
    returned = session.get_feature_state(selector)
    session.remove_feature_state(selector)

    assert fake_native.set_call == (
        "points",
        "symbols",
        "feature-1",
        "hover",
        state,
    )
    assert json.loads(returned) == {"hover": True}
    assert fake_native.remove_call == ("points", "symbols", "feature-1", "hover")


def test_render_session_feature_state_empty_result_is_empty_object() -> None:
    class FakeNativeRenderSession:
        closed = False
        detached = False

        def get_feature_state(
            self,
            source_id: str,
            source_layer_id: str | None,
            feature_id: str | None,
            state_key: str | None,
        ) -> bytes:
            assert (source_id, source_layer_id, feature_id, state_key) == (
                "points",
                None,
                "feature-1",
                None,
            )
            return b"{}"

    session = render.RenderSessionHandle._from_native(
        FakeNativeRenderSession(), object()
    )
    selector = query.FeatureStateSelector(source_id="points", feature_id="feature-1")

    assert json.loads(session.get_feature_state(selector)) == {}


def test_invalid_render_target_attach_reports_native_status() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        with pytest.raises(
            (mln.InvalidArgumentError, mln.UnsupportedFeatureError)
        ) as raised:
            map_handle.attach_metal_owned_texture(render.MetalOwnedTextureDescriptor())

        assert raised.value.status in {
            mln.MaplibreStatus.INVALID_ARGUMENT,
            mln.MaplibreStatus.UNSUPPORTED,
        }


def test_invalid_opengl_render_target_attach_reports_native_status() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        with pytest.raises(
            (mln.InvalidArgumentError, mln.UnsupportedFeatureError)
        ) as raised:
            map_handle.attach_opengl_owned_texture(
                render.OpenGLOwnedTextureDescriptor()
            )

        assert raised.value.status in {
            mln.MaplibreStatus.INVALID_ARGUMENT,
            mln.MaplibreStatus.UNSUPPORTED,
        }


def test_map_coordinate_conversions_round_trip_public_values() -> None:
    coordinate = geo.LatLng(0.0, 0.0)
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.jump_to(camera.CameraOptions(center=coordinate, zoom=1.0))
        point = map_handle.pixel_for_lat_lng(coordinate)
        projected = map_handle.lat_lng_for_pixel(point)
        points = map_handle.pixels_for_lat_lngs((coordinate, geo.LatLng(1.0, 1.0)))
        coordinates = map_handle.lat_lngs_for_pixels(points)

        assert isinstance(point, camera.ScreenPoint)
        assert math.isfinite(projected.latitude)
        assert math.isfinite(projected.longitude)
        assert len(points) == 2
        assert len(coordinates) == 2
        assert all(isinstance(item, camera.ScreenPoint) for item in points)
        assert all(isinstance(item, geo.LatLng) for item in coordinates)


def test_map_projection_converts_coordinates_and_closes() -> None:
    coordinate = geo.LatLng(0.0, 0.0)
    meters = map_module.projected_meters_for_lat_lng(coordinate)
    round_tripped = map_module.lat_lng_for_projected_meters(meters)

    assert isinstance(meters, map_module.ProjectedMeters)
    assert math.isclose(round_tripped.latitude, coordinate.latitude, abs_tol=1e-6)
    assert math.isclose(round_tripped.longitude, coordinate.longitude, abs_tol=1e-6)

    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.jump_to(camera.CameraOptions(center=coordinate, zoom=1.0))
        with map_handle.create_projection() as projection:
            point = projection.pixel_for_lat_lng(coordinate)
            projected = projection.lat_lng_for_pixel(point)
            projection.set_camera(camera.CameraOptions(center=coordinate, zoom=2.0))
            projection.set_visible_coordinates(
                (geo.LatLng(-1.0, -1.0), geo.LatLng(1.0, 1.0)),
                camera.EdgeInsets(),
            )
            projection.set_visible_geometry(
                _json_object(
                    {
                        "type": "LineString",
                        "coordinates": [[-1.0, -1.0], [1.0, 1.0]],
                    }
                ),
                camera.EdgeInsets(),
            )

            assert not projection.closed
            assert isinstance(projection.get_camera(), camera.CameraOptions)
            assert isinstance(point, camera.ScreenPoint)
            assert math.isfinite(projected.latitude)
            assert math.isfinite(projected.longitude)

        assert projection.closed


def test_map_projection_remains_usable_on_another_thread_after_map_close() -> None:
    runtime = mln.RuntimeHandle()
    map_handle = runtime.create_map()
    projection = map_handle.create_projection()
    map_handle.close()
    runtime.close()

    def use_and_close_projection() -> None:
        assert projection.get_camera() is not None
        projection.close()

    with concurrent.futures.ThreadPoolExecutor(max_workers=1) as executor:
        executor.submit(use_and_close_projection).result()

    assert projection.closed


def test_offline_region_operation_starts_return_public_handles(tmp_path: Path) -> None:
    definition = offline.OfflineTilePyramidRegionDefinition(
        style_url="https://example.test/style.json",
        bounds=geo.LatLngBounds(
            southwest=geo.LatLng(-1.0, -1.0),
            northeast=geo.LatLng(1.0, 1.0),
        ),
        min_zoom=0.0,
        max_zoom=1.0,
        pixel_ratio=1.0,
    )
    with mln.RuntimeHandle(
        mln.RuntimeOptions(cache_path=str(tmp_path / "cache.db"))
    ) as runtime:
        operations = [
            runtime.create_offline_region(definition, b"metadata"),
            runtime.list_offline_regions(),
            runtime.get_offline_region(1),
            runtime.update_offline_region_metadata(1, b"updated"),
            runtime.get_offline_region_status(1),
            runtime.set_offline_region_observed(1, False),
            runtime.set_offline_region_download_state(
                1,
                offline.OfflineRegionDownloadState.INACTIVE,
            ),
            runtime.invalidate_offline_region(1),
            runtime.delete_offline_region(1),
        ]

        for operation in operations:
            assert isinstance(operation, offline.OfflineOperationHandle)
            assert not operation.closed
            operation.close()
            assert operation.closed


def test_offline_operation_take_results_convert_public_values() -> None:
    class FakeNativeRuntime:
        def __init__(self) -> None:
            self.discarded = []

        def offline_region_create_take_result(
            self, operation_id: int
        ) -> dict[str, object]:
            assert operation_id == 1
            return region_wire(b"created")

        def offline_region_get_take_result(
            self, operation_id: int
        ) -> dict[str, object] | None:
            assert operation_id == 2
            return region_wire(b"found")

        def offline_regions_list_take_result(
            self, operation_id: int
        ) -> list[dict[str, object]]:
            assert operation_id == 3
            return [region_wire(b"listed")]

        def offline_regions_merge_database_take_result(
            self,
            operation_id: int,
        ) -> list[dict[str, object]]:
            assert operation_id == 4
            return [region_wire(b"merged")]

        def offline_region_update_metadata_take_result(
            self,
            operation_id: int,
        ) -> dict[str, object]:
            assert operation_id == 5
            return region_wire(b"updated")

        def offline_region_get_status_take_result(
            self, operation_id: int
        ) -> dict[str, object]:
            assert operation_id == 6
            return {
                "download_state": 1,
                "completed_resource_count": 2,
                "completed_resource_size": 3,
                "completed_tile_count": 4,
                "required_tile_count": 5,
                "completed_tile_size": 6,
                "required_resource_count": 7,
                "required_resource_count_is_precise": True,
                "complete": False,
            }

        def offline_operation_discard(self, operation_id: int) -> None:
            self.discarded.append(operation_id)

    class FakeRuntime:
        def __init__(self) -> None:
            self._native = FakeNativeRuntime()
            self.operations: set[offline.OfflineOperationHandle] = set()

        def _register_offline_operation(
            self, operation: offline.OfflineOperationHandle
        ) -> None:
            self.operations.add(operation)

        def _unregister_offline_operation(
            self, operation: offline.OfflineOperationHandle
        ) -> None:
            self.operations.discard(operation)

    def region_wire(metadata: bytes) -> dict[str, object]:
        return {
            "id": 42,
            "definition": {
                "type": "tile_pyramid",
                "style_url": "https://example.test/style.json",
                "bounds": {
                    "southwest": {"latitude": -1.0, "longitude": -2.0},
                    "northeast": {"latitude": 1.0, "longitude": 2.0},
                },
                "min_zoom": 0.0,
                "max_zoom": 10.0,
                "pixel_ratio": 1.0,
                "include_ideographs": True,
            },
            "metadata": metadata,
        }

    runtime = FakeRuntime()

    with pytest.raises(TypeError, match="created by RuntimeHandle"):
        offline.OfflineOperationHandle(runtime, 99)

    region = offline.OfflineOperationHandle._from_native(runtime, 1).take_region()
    optional = offline.OfflineOperationHandle._from_native(
        runtime, 2
    ).take_optional_region()
    listed = offline.OfflineOperationHandle._from_native(runtime, 3).take_region_list()
    merged = offline.OfflineOperationHandle._from_native(runtime, 4).take_region_list(
        merge_result=True,
    )
    updated = offline.OfflineOperationHandle._from_native(
        runtime, 5
    ).take_updated_region()
    status = offline.OfflineOperationHandle._from_native(runtime, 6).take_status()

    assert region.id == 42
    assert isinstance(region.definition, offline.OfflineTilePyramidRegionDefinition)
    assert region.definition.bounds.southwest == geo.LatLng(-1.0, -2.0)
    assert optional is not None
    assert optional.metadata == b"found"
    assert listed[0].metadata == b"listed"
    assert merged[0].metadata == b"merged"
    assert updated.metadata == b"updated"
    assert status.download_state == offline.OfflineRegionDownloadState.ACTIVE
    assert status.completed_resource_count == 2
    assert runtime.operations == set()


def test_offline_operation_take_rejects_closed_handles() -> None:
    class FakeNativeRuntime:
        def __init__(self) -> None:
            self.discarded: list[int] = []
            self.status_takes = 0

        def offline_operation_discard(self, operation_id: int) -> None:
            self.discarded.append(operation_id)

        def offline_region_get_status_take_result(
            self,
            operation_id: int,
        ) -> dict[str, object]:
            assert operation_id == 10
            self.status_takes += 1
            return {
                "download_state": 1,
                "completed_resource_count": 0,
                "completed_resource_size": 0,
                "completed_tile_count": 0,
                "required_tile_count": 0,
                "completed_tile_size": 0,
                "required_resource_count": 0,
                "required_resource_count_is_precise": True,
                "complete": True,
            }

        def offline_region_create_take_result(self, operation_id: int) -> None:
            raise AssertionError("closed handle called native take")

        def offline_region_get_take_result(self, operation_id: int) -> None:
            raise AssertionError("closed handle called native take")

        def offline_regions_list_take_result(self, operation_id: int) -> None:
            raise AssertionError("closed handle called native take")

        def offline_regions_merge_database_take_result(self, operation_id: int) -> None:
            raise AssertionError("closed handle called native take")

        def offline_region_update_metadata_take_result(self, operation_id: int) -> None:
            raise AssertionError("closed handle called native take")

    class FakeRuntime:
        def __init__(self) -> None:
            self._native = FakeNativeRuntime()
            self.operations: set[offline.OfflineOperationHandle] = set()

        def _register_offline_operation(
            self, operation: offline.OfflineOperationHandle
        ) -> None:
            self.operations.add(operation)

        def _unregister_offline_operation(
            self, operation: offline.OfflineOperationHandle
        ) -> None:
            self.operations.discard(operation)

    runtime = FakeRuntime()

    status_handle = offline.OfflineOperationHandle._from_native(runtime, 10)
    assert status_handle.take_status().complete is True
    with pytest.raises(mln.InvalidStateError, match="offline operation handle"):
        status_handle.take_status()
    assert runtime._native.status_takes == 1

    closed_takes = (
        lambda handle: handle.take_region(),
        lambda handle: handle.take_optional_region(),
        lambda handle: handle.take_region_list(),
        lambda handle: handle.take_region_list(merge_result=True),
        lambda handle: handle.take_updated_region(),
        lambda handle: handle.take_status(),
    )
    for offset, take in enumerate(closed_takes, start=20):
        handle = offline.OfflineOperationHandle._from_native(runtime, offset)
        handle.close()
        with pytest.raises(mln.InvalidStateError, match="offline operation handle"):
            take(handle)

    assert runtime._native.discarded == list(range(20, 26))
    assert runtime._native.status_takes == 1
    assert runtime.operations == set()


def test_runtime_close_rejects_live_offline_operations(tmp_path: Path) -> None:
    runtime = mln.RuntimeHandle(mln.RuntimeOptions(cache_path=str(tmp_path)))
    operation = runtime.run_ambient_cache_operation(offline.AmbientCacheOperation.CLEAR)
    try:
        assert not operation.closed
        with pytest.raises(mln.InvalidStateError, match="offline operation"):
            runtime.close()

        assert not runtime.closed
        assert not operation.closed
    finally:
        operation.close()
        runtime.close()


def test_ambient_cache_operation_starts_and_discards_through_public_api(
    tmp_path: Path,
) -> None:
    with mln.RuntimeHandle(mln.RuntimeOptions(cache_path=str(tmp_path))) as runtime:
        operation = runtime.run_ambient_cache_operation(
            offline.AmbientCacheOperation.CLEAR,
        )

        assert isinstance(operation, offline.OfflineOperationHandle)
        assert not operation.closed
        operation.close()
        assert operation.closed
        operation.close()


def test_set_maximum_ambient_cache_size_starts_and_discards_through_public_api(
    tmp_path: Path,
) -> None:
    with mln.RuntimeHandle(mln.RuntimeOptions(cache_path=str(tmp_path))) as runtime:
        operation = runtime.set_maximum_ambient_cache_size(8 << 20)

        assert isinstance(operation, offline.OfflineOperationHandle)
        assert not operation.closed

        event = _wait_for_offline_operation(runtime, operation)
        completed = event.payload
        assert isinstance(completed, offline.OfflineOperationCompleted)
        # The completion reports a named kind rather than an unknown value.
        assert (
            completed.operation_kind
            == offline.OfflineOperationKind.SET_MAXIMUM_AMBIENT_CACHE_SIZE
        )
        assert not completed.operation_kind.is_unknown

        operation.close()
        assert operation.closed

        # An out-of-range size stays on the binding's documented error family
        # instead of leaking the extension module's OverflowError.
        with pytest.raises(mln.InvalidArgumentError):
            runtime.set_maximum_ambient_cache_size(-1)
        with pytest.raises(mln.InvalidArgumentError):
            runtime.set_maximum_ambient_cache_size(2**64)


def test_offline_values_wrap_runtime_event_payload_shape() -> None:
    bounds = geo.LatLngBounds(geo.LatLng(1.0, 2.0), geo.LatLng(3.0, 4.0))
    definition = offline.OfflineTilePyramidRegionDefinition(
        style_url="https://example.test/style.json",
        bounds=bounds,
        min_zoom=1.0,
        max_zoom=3.0,
        pixel_ratio=2.0,
    )
    status = offline.OfflineRegionStatus(
        download_state=offline.OfflineRegionDownloadState.ACTIVE,
        completed_resource_count=1,
        completed_resource_size=2,
        completed_tile_count=3,
        required_tile_count=4,
        completed_tile_size=5,
        required_resource_count=6,
        required_resource_count_is_precise=True,
        complete=False,
    )
    completed = offline.OfflineOperationCompleted._from_runtime_payload(
        {
            "operation_id": 7,
            "operation_kind": offline.OfflineOperationKind.REGION_CREATE.native_code,
            "result_kind": offline.OfflineOperationResultKind.REGION,
            "result_status": mln.MaplibreStatus.OK.native_code,
            "found": True,
        }
    )
    response_error = offline.OfflineRegionResponseError._from_runtime_payload(
        {
            "region_id": 8,
            "reason": resource.ResourceErrorReason.NOT_FOUND.native_code,
        }
    )
    tile_limit = offline.OfflineRegionTileCountLimitExceeded._from_runtime_payload(
        {
            "region_id": 9,
            "limit": 10,
        }
    )

    assert (
        definition.definition_type == offline.OfflineRegionDefinitionType.TILE_PYRAMID
    )
    assert status.download_state == offline.OfflineRegionDownloadState.ACTIVE
    assert completed.operation_kind == offline.OfflineOperationKind.REGION_CREATE
    assert completed.result_kind == offline.OfflineOperationResultKind.REGION
    unknown_state = offline.OfflineRegionDownloadState(999_001)
    assert unknown_state.native_code == 999_001
    with pytest.raises(mln.InvalidArgumentError, match="cannot be set"):
        unknown_state.native_code_for_set()
    assert response_error.region_id == 8
    assert response_error.reason == resource.ResourceErrorReason.NOT_FOUND
    assert tile_limit.region_id == 9
    assert tile_limit.limit == 10


def test_query_descriptors_and_results_preserve_public_shape() -> None:
    point = camera.ScreenPoint(1.0, 2.0)
    geometry = query.RenderedQueryGeometry.point_geometry(point)
    box_geometry = query.RenderedQueryGeometry.box_geometry(
        query.ScreenBox(camera.ScreenPoint(0.0, 0.0), camera.ScreenPoint(10.0, 10.0))
    )
    line_geometry = query.RenderedQueryGeometry.line_string_geometry(
        (camera.ScreenPoint(0.0, 0.0), camera.ScreenPoint(5.0, 5.0))
    )
    rendered_options = query.RenderedFeatureQueryOptions(
        layer_ids=("landuse",),
        filter=_json_value(["==", "class", "park"]),
    )
    source_options = query.SourceFeatureQueryOptions(source_layer_ids=("landuse",))
    selector = query.FeatureStateSelector(
        source_id="source",
        source_layer_id="layer",
        feature_id="feature-1",
        state_key="hover",
    )
    assert geometry.point == point
    assert box_geometry.box is not None
    assert line_geometry.line_string == (
        camera.ScreenPoint(0.0, 0.0),
        camera.ScreenPoint(5.0, 5.0),
    )
    assert rendered_options.layer_ids == ("landuse",)
    assert rendered_options.filter == _json_value(["==", "class", "park"])
    assert source_options.source_layer_ids == ("landuse",)
    assert selector.state_key == "hover"


def test_query_selector_rejects_state_key_without_feature_id() -> None:
    with pytest.raises(ValueError, match="state_key requires feature_id"):
        query.FeatureStateSelector(source_id="source", state_key="hover")


def test_query_geometry_rejects_mismatched_variant_value() -> None:
    with pytest.raises(ValueError, match="point query geometry requires point"):
        query.RenderedQueryGeometry(
            query.RenderedQueryGeometryType.POINT,
            box=query.ScreenBox(
                camera.ScreenPoint(0.0, 0.0), camera.ScreenPoint(1.0, 1.0)
            ),
        )


def test_process_global_logging_receiver_copies_native_records() -> None:
    receiver = log.set_log_callback(max_queued_records=8, consume=True)
    try:
        log.set_async_log_severity_mask(log.LogSeverityMask.DEFAULT)
        with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
            with pytest.raises((mln.InvalidArgumentError, mln.NativeError)):
                map_handle.set_style_json(b"{")
            map_handle.dump_debug_logs()

        records = []
        while (record := receiver.poll_record()) is not None:
            records.append(record)

        assert records
        assert any(record.message for record in records)
        assert all(isinstance(record.severity, log.LogSeverity) for record in records)
        assert all(isinstance(record.event, log.LogEvent) for record in records)
    finally:
        log.clear_log_callback()
        log.set_async_log_severity_mask(log.LogSeverityMask.DEFAULT)


def test_log_receiver_reports_dropped_records() -> None:
    receiver = log.set_log_callback(max_queued_records=1, consume=True)
    try:
        with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
            with pytest.raises((mln.InvalidArgumentError, mln.NativeError)):
                map_handle.set_style_json(b"{")
            map_handle.dump_debug_logs()

        assert receiver.poll_record() is not None
        assert receiver.dropped_record_count >= 0
    finally:
        log.clear_log_callback()


def test_resource_values_preserve_native_shape() -> None:
    request = resource.ResourceRequest._from_native(
        {
            "requested_url": "maplibre://tiles/2/1/1.pbf",
            "resolved_url": "https://example.test/tile.pbf",
            "kind": resource.ResourceKind.TILE.native_code,
            "loading_method": resource.ResourceLoadingMethod.NETWORK_ONLY,
            "priority": resource.ResourcePriority.LOW,
            "usage": resource.ResourceUsage.OFFLINE,
            "storage_policy": resource.ResourceStoragePolicy.VOLATILE,
            "range": {"start": 5, "end": 10},
            "prior_modified_unix_ms": 123,
            "prior_expires_unix_ms": 456,
            "prior_etag": "abc",
            "prior_data": b"old",
        }
    )
    response = resource.ResourceResponse(
        status=resource.ResourceResponseStatus.ERROR,
        error_reason=resource.ResourceErrorReason.NOT_FOUND,
        error_message="missing",
    )

    assert request.kind == resource.ResourceKind.TILE
    assert request.requested_url == "maplibre://tiles/2/1/1.pbf"
    assert request.resolved_url == "https://example.test/tile.pbf"
    assert request.range == resource.ByteRange(5, 10)
    assert request.prior_data == b"old"
    assert response._to_native()["status"] == resource.ResourceResponseStatus.ERROR
    assert (
        response._to_native()["error_reason"] == resource.ResourceErrorReason.NOT_FOUND
    )


def test_resource_provider_adapter_pass_through_closes_temporary_handle() -> None:
    class FakeNativeRequest:
        def __init__(self) -> None:
            self.closed = False

        def complete(self, response: dict[str, object]) -> None:
            raise AssertionError(response)

        def is_cancelled(self) -> bool:
            return False

        def close(self) -> None:
            self.closed = True

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        assert request.requested_url == "https://example.test/tile.pbf"
        assert handle.closed is False
        return resource.ResourceProviderDecision.PASS_THROUGH

    native = FakeNativeRequest()
    adapted = resource._adapt_resource_provider_callback(provider)

    raw_request = {
        "requested_url": "https://example.test/tile.pbf",
        "resolved_url": "https://example.test/tile.pbf",
        "kind": resource.ResourceKind.TILE.native_code,
        "loading_method": resource.ResourceLoadingMethod.NETWORK_ONLY,
        "priority": resource.ResourcePriority.LOW,
        "usage": resource.ResourceUsage.ONLINE,
        "storage_policy": resource.ResourceStoragePolicy.VOLATILE,
        "range": None,
        "prior_modified_unix_ms": None,
        "prior_expires_unix_ms": None,
        "prior_etag": None,
        "prior_data": None,
    }
    with warnings.catch_warnings(record=True) as captured:
        warnings.simplefilter("always", ResourceWarning)
        decision = adapted(raw_request, native)

    assert decision == resource.ResourceProviderDecision.PASS_THROUGH.native_code
    assert native.closed is True
    assert not captured


def test_resource_provider_adapter_closes_temporary_handle_on_exception() -> None:
    class FakeNativeRequest:
        def __init__(self) -> None:
            self.closed = False
            self.close_count = 0

        def complete(self, response: dict[str, object]) -> None:
            raise AssertionError(response)

        def is_cancelled(self) -> bool:
            return False

        def close(self) -> None:
            self.close_count += 1
            self.closed = True

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        assert request.requested_url == "https://example.test/tile.pbf"
        assert handle.closed is False
        msg = "contained provider failure"
        raise RuntimeError(msg)

    raw_request = {
        "requested_url": "https://example.test/tile.pbf",
        "resolved_url": "https://example.test/tile.pbf",
        "kind": resource.ResourceKind.TILE.native_code,
        "loading_method": resource.ResourceLoadingMethod.NETWORK_ONLY,
        "priority": resource.ResourcePriority.LOW,
        "usage": resource.ResourceUsage.ONLINE,
        "storage_policy": resource.ResourceStoragePolicy.VOLATILE,
        "range": None,
        "prior_modified_unix_ms": None,
        "prior_expires_unix_ms": None,
        "prior_etag": None,
        "prior_data": b"",
    }
    native = FakeNativeRequest()
    adapted = resource._adapt_resource_provider_callback(provider)

    with warnings.catch_warnings(record=True) as captured:
        warnings.simplefilter("always", ResourceWarning)
        with pytest.raises(RuntimeError, match="contained provider failure"):
            adapted(raw_request, native)

    assert native.closed is True
    assert native.close_count == 1
    assert not captured


def test_offline_download_state_setter_rejects_unknown_before_native_call() -> None:
    class FakeRuntimeNative:
        def offline_region_set_download_state_start(
            self,
            region_id: int,
            state: int,
        ) -> int:
            raise AssertionError((region_id, state))

    runtime = mln.RuntimeHandle.__new__(mln.RuntimeHandle)
    runtime._native = FakeRuntimeNative()

    with pytest.raises(mln.InvalidArgumentError, match="cannot be set"):
        runtime.set_offline_region_download_state(
            1,
            offline.OfflineRegionDownloadState(999_001),
        )


def test_resource_request_handle_close_context_and_completion_state() -> None:
    class FakeNativeRequest:
        def __init__(self) -> None:
            self.completed = None
            self.complete_count = 0
            self.closed = False
            self.close_count = 0
            self.validation_error: BaseException | None = None

        def validate_completion_response(self, response: dict[str, object]) -> None:
            if self.validation_error is not None:
                raise self.validation_error

        def complete(self, response: dict[str, object]) -> None:
            self.complete_count += 1
            self.completed = response

        def is_cancelled(self) -> bool:
            return False

        def close(self) -> None:
            self.close_count += 1
            self.closed = True

    native = FakeNativeRequest()
    with pytest.raises(TypeError, match="created by resource providers"):
        resource.ResourceRequestHandle(native)

    handle = resource.ResourceRequestHandle._from_native(native)

    assert handle.closed is False
    assert handle.is_cancelled() is False
    handle.complete(
        resource.ResourceResponse(status=resource.ResourceResponseStatus.NO_CONTENT)
    )
    assert handle.closed is True
    assert native.completed["status"] == resource.ResourceResponseStatus.NO_CONTENT
    assert native.complete_count == 1
    with pytest.raises(mln.InvalidStateError, match="already closed"):
        handle.complete(
            resource.ResourceResponse(status=resource.ResourceResponseStatus.NO_CONTENT)
        )
    assert native.complete_count == 1
    with pytest.raises(mln.InvalidStateError, match="already closed"):
        handle.is_cancelled()

    closed_native = FakeNativeRequest()
    closed = resource.ResourceRequestHandle._from_native(closed_native)
    closed.close()
    with pytest.raises(mln.InvalidStateError, match="already closed"):
        closed.complete(
            resource.ResourceResponse(status=resource.ResourceResponseStatus.NO_CONTENT)
        )
    with pytest.raises(mln.InvalidStateError, match="already closed"):
        closed.is_cancelled()
    assert closed_native.complete_count == 0
    assert closed_native.close_count == 1

    retry_native = FakeNativeRequest()
    retry = resource.ResourceRequestHandle._from_native(retry_native)

    class InvalidResponse:
        def _to_native(self) -> dict[str, object]:
            msg = "cannot materialize response"
            raise ValueError(msg)

    with pytest.raises(ValueError, match="cannot materialize response"):
        retry.complete(InvalidResponse())  # type: ignore[arg-type]
    assert retry.closed is False
    assert retry_native.complete_count == 0
    assert retry_native.close_count == 0
    retry.close()

    pre_c_native = FakeNativeRequest()
    pre_c_native.validation_error = ValueError("native response validation failed")
    pre_c = resource.ResourceRequestHandle._from_native(pre_c_native)
    with pytest.raises(ValueError, match="native response validation failed"):
        pre_c.complete(
            resource.ResourceResponse(status=resource.ResourceResponseStatus.NO_CONTENT)
        )
    assert pre_c.closed is False
    assert pre_c_native.complete_count == 0
    assert pre_c_native.close_count == 0
    pre_c.close()

    second_native = FakeNativeRequest()
    with resource.ResourceRequestHandle._from_native(second_native) as second:
        assert second.closed is False
    assert second.closed is True
    assert second_native.closed is True

    leaked_native = FakeNativeRequest()
    leaked = resource.ResourceRequestHandle._from_native(leaked_native)
    with pytest.warns(ResourceWarning, match="ResourceRequestHandle was not closed"):
        leaked.__del__()
    assert leaked_native.closed is False


def test_resource_transform_registers_and_clears() -> None:
    seen: list[resource.ResourceTransformRequest] = []

    def transform(request: resource.ResourceTransformRequest) -> str | None:
        seen.append(request)
        return None

    with mln.RuntimeHandle() as runtime:
        runtime.set_resource_transform(transform, max_pending_callbacks=1)
        with runtime.create_map():
            runtime.set_resource_transform(transform, max_pending_callbacks=1)
            runtime.clear_resource_transform()


def test_resource_callback_registration_validates_bounds_and_lifecycle() -> None:
    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        handle.close()
        return resource.ResourceProviderDecision.PASS_THROUGH

    runtime = mln.RuntimeHandle()
    with pytest.raises(mln.InvalidArgumentError):
        runtime.set_resource_provider(provider, max_pending_callbacks=0)

    runtime.set_resource_provider(provider, max_pending_callbacks=1)
    runtime.close()

    with pytest.raises(mln.InvalidStateError) as replaced:
        runtime.set_resource_provider(provider, max_pending_callbacks=1)
    assert replaced.value.native_status_code is None
    assert replaced.value.diagnostic == "runtime handle is closed"

    with pytest.raises(mln.InvalidStateError) as cleared:
        runtime.clear_resource_provider()
    assert cleared.value.native_status_code is None
    assert cleared.value.diagnostic == "runtime handle is closed"


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
        with mln.RuntimeHandle() as runtime:
            runtime.set_resource_provider(provider, max_pending_callbacks=4)
            with runtime.create_map() as map_handle:
                map_handle.set_style_url(style_url)
                _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)

        assert served.is_set()
        assert any(request.kind == resource.ResourceKind.STYLE for request in calls)
        assert temporary_handles
        assert all(handle.closed for handle in temporary_handles)
        with pytest.raises(mln.InvalidStateError, match="already closed"):
            temporary_handles[0].complete(
                resource.ResourceResponse(
                    status=resource.ResourceResponseStatus.NO_CONTENT
                )
            )


def test_resource_transform_rewrites_copied_network_style_request() -> None:
    transform_requests: list[resource.ResourceTransformRequest] = []

    def transform(request: resource.ResourceTransformRequest) -> str | None:
        transform_requests.append(request)
        if request.url == "http://example.invalid/original-style.json":
            return rewritten_style_url
        return None

    with (
        _online_network(),
        _http_style_server() as (rewritten_style_url, served),
        mln.RuntimeHandle() as runtime,
        runtime.create_map() as map_handle,
    ):
        runtime.set_resource_transform(transform, max_pending_callbacks=4)
        map_handle.set_style_url("http://example.invalid/original-style.json")
        _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)

    assert served.is_set()
    assert transform_requests
    assert transform_requests[0].kind == resource.ResourceKind.STYLE
    assert transform_requests[0].url == "http://example.invalid/original-style.json"


def test_resource_transform_can_be_cleared_after_map_creation() -> None:
    calls: list[resource.ResourceTransformRequest] = []

    def transform(request: resource.ResourceTransformRequest) -> str:
        calls.append(request)
        return "unsupported://unexpected-rewrite.json"

    with (
        _online_network(),
        _http_style_server() as (style_url, served),
        mln.RuntimeHandle() as runtime,
        runtime.create_map() as map_handle,
    ):
        runtime.set_resource_transform(transform, max_pending_callbacks=1)
        runtime.clear_resource_transform()
        map_handle.set_style_url(style_url)
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
            runtime.pump()
            for event in runtime.drain_events().events:
                if (
                    event.event_type == mln.RuntimeEventType.MAP_LOADING_FAILED
                    and event.message is not None
                    and style_url in event.message
                    and '"jar"' in event.message
                ):
                    return
            time.sleep(0.001)
        raise AssertionError(f"style {style_url!r} did not report a loading failure")

    with mln.RuntimeHandle() as runtime:
        runtime.set_resource_provider(
            counting_provider(first_urls), max_pending_callbacks=4
        )
        with runtime.create_map() as map_handle:
            load_unservable_style(runtime, map_handle, "jar:file:/packaged/first.json")
            assert "jar:file:/packaged/first.json" in first_urls

            runtime.set_resource_provider(
                counting_provider(second_urls), max_pending_callbacks=4
            )
            first_urls_after_replace = list(first_urls)
            load_unservable_style(runtime, map_handle, "jar:file:/packaged/second.json")
            assert "jar:file:/packaged/second.json" in second_urls
            assert first_urls == first_urls_after_replace

            runtime.clear_resource_provider()
            second_urls_after_clear = list(second_urls)
            load_unservable_style(runtime, map_handle, "jar:file:/packaged/third.json")
            assert first_urls == first_urls_after_replace
            assert second_urls == second_urls_after_clear

            # Clearing an already cleared provider stays a successful no-op.
            runtime.clear_resource_provider()


def test_resource_provider_sees_scheme_alias_and_its_resolved_url() -> None:
    """BND-155: a configured URI-scheme alias arrives alongside its fetch URL."""
    resolved_urls: list[str] = []

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if request.requested_url != "maplibre://maps/style":
            return resource.ResourceProviderDecision.PASS_THROUGH
        resolved_urls.append(request.resolved_url)
        handle.complete(resource.ResourceResponse(bytes=_EMPTY_STYLE_BYTES))
        return resource.ResourceProviderDecision.HANDLE

    with mln.RuntimeHandle() as runtime:
        runtime.set_resource_provider(provider, max_pending_callbacks=4)
        with runtime.create_map() as map_handle:
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
        handle.complete(resource.ResourceResponse(bytes=_EMPTY_STYLE_BYTES))
        completions += 1
        return resource.ResourceProviderDecision.PASS_THROUGH

    with mln.RuntimeHandle() as runtime:
        runtime.set_resource_provider(provider, max_pending_callbacks=4)
        with runtime.create_map() as map_handle:
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

    with mln.RuntimeHandle() as runtime:
        runtime.set_resource_provider(provider, max_pending_callbacks=4)
        with runtime.create_map() as map_handle:
            map_handle.set_style_url("custom://deferred-style.json")
            handle = _wait_for_provider_handle(runtime, handles)

            assert handle.is_cancelled() is False
            with pytest.raises(mln.InvalidArgumentError):
                handle.complete(
                    resource.ResourceResponse(
                        status=resource.ResourceResponseStatus.ERROR,
                        error_reason=resource.ResourceErrorReason.OTHER,
                        error_message="bad\0message",
                    )
                )
            assert handle.closed is False

            handle.complete(resource.ResourceResponse(bytes=_EMPTY_STYLE_BYTES))
            with pytest.raises(mln.InvalidStateError, match="already closed"):
                handle.complete(resource.ResourceResponse(bytes=_EMPTY_STYLE_BYTES))
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

    with mln.RuntimeHandle() as runtime:
        runtime.set_resource_provider(provider, max_pending_callbacks=4)
        with runtime.create_map() as map_handle:
            map_handle.set_style_url("custom://cross-thread-style.json")
            handle = _wait_for_provider_handle(runtime, handles)

            completed = threading.Event()

            def complete() -> None:
                handle.complete(
                    resource.ResourceResponse(bytes=_EMPTY_STYLE_BYTES),
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
            )
        )
        return resource.ResourceProviderDecision.HANDLE

    with mln.RuntimeHandle() as runtime:
        runtime.set_resource_provider(provider, max_pending_callbacks=4)
        with runtime.create_map() as map_handle:
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
            )
        )
        return resource.ResourceProviderDecision.HANDLE

    definition = offline.OfflineTilePyramidRegionDefinition(
        style_url="custom://offline-error-style.json",
        bounds=geo.LatLngBounds(
            southwest=geo.LatLng(1.0, 2.0),
            northeast=geo.LatLng(3.0, 4.0),
        ),
        min_zoom=5.0,
        max_zoom=6.0,
        pixel_ratio=1.0,
    )

    with mln.RuntimeHandle(
        mln.RuntimeOptions(cache_path=str(tmp_path / "offline-cache.db"))
    ) as runtime:
        runtime.set_resource_provider(provider, max_pending_callbacks=4)
        create = runtime.create_offline_region(definition, b"metadata")
        _wait_for_offline_operation(runtime, create)
        region = create.take_region()

        observe = runtime.set_offline_region_observed(region.id, True)
        _wait_for_offline_operation(runtime, observe)
        observe.close()

        activate = runtime.set_offline_region_download_state(
            region.id,
            offline.OfflineRegionDownloadState.ACTIVE,
        )
        try:
            event = _wait_for_runtime_event(
                runtime,
                mln.RuntimeEventType.OFFLINE_REGION_RESPONSE_ERROR,
            )
        finally:
            activate.close()

        deactivate = runtime.set_offline_region_download_state(
            region.id,
            offline.OfflineRegionDownloadState.INACTIVE,
        )
        _wait_for_offline_operation(runtime, deactivate)
        deactivate.close()

        unobserve = runtime.set_offline_region_observed(region.id, False)
        _wait_for_offline_operation(runtime, unobserve)
        unobserve.close()

    assert isinstance(event.payload, offline.OfflineRegionResponseError)
    assert event.payload.region_id == region.id
    assert event.payload.reason == resource.ResourceErrorReason.NOT_FOUND
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

    with mln.RuntimeHandle() as runtime:
        runtime.set_resource_provider(provider, max_pending_callbacks=4)
        map_handle = runtime.create_map()
        map_handle.set_style_url("custom://cancelled-style.json")
        handle = _wait_for_provider_handle(runtime, handles)

        map_handle.close()
        for _ in range(5000):
            runtime.pump()
            if handle.is_cancelled():
                break
            time.sleep(0.001)
        else:
            raise AssertionError("resource request was not cancelled")

        with pytest.raises(mln.InvalidStateError) as native_error:
            handle.complete(resource.ResourceResponse(bytes=_EMPTY_STYLE_BYTES))
        assert native_error.value.native_status_code == -2
        assert native_error.value.diagnostic

        with pytest.raises(mln.InvalidStateError) as terminal_error:
            handle.complete(resource.ResourceResponse(bytes=_EMPTY_STYLE_BYTES))
        assert terminal_error.value.native_status_code is None


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

    with mln.RuntimeHandle() as runtime:
        runtime.set_resource_provider(provider, max_pending_callbacks=4)
        with runtime.create_map() as map_handle:
            map_handle.set_style_url("custom://stale-style-1.json")
            stale_handle = _wait_for_provider_handle(runtime, handles)
            stale_handle.close()
            with pytest.raises(mln.InvalidStateError, match="already closed"):
                stale_handle.is_cancelled()

            map_handle.set_style_url("custom://stale-style-2.json")
            live_handle = _wait_for_provider_handle(runtime, handles)
            with pytest.raises(mln.InvalidStateError, match="already closed"):
                stale_handle.complete(
                    resource.ResourceResponse(bytes=_EMPTY_STYLE_BYTES)
                )
            assert live_handle.is_cancelled() is False
            live_handle.complete(resource.ResourceResponse(bytes=_EMPTY_STYLE_BYTES))
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

    with mln.RuntimeHandle() as runtime:
        runtime.set_resource_provider(provider, max_pending_callbacks=4)
        with runtime.create_map() as map_handle:
            map_handle.set_style_url("custom://release-race-style.json")
            handle = _wait_for_provider_handle(runtime, handles)

            started = threading.Event()
            stop = threading.Event()
            saw_closed = threading.Event()

            def probe_cancelled() -> None:
                while not stop.is_set():
                    try:
                        handle.is_cancelled()
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
            with pytest.raises(mln.InvalidStateError, match="already closed"):
                handle.is_cancelled()


def test_custom_geometry_source_scaffolding_queues_copied_events() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        source = map_handle.add_custom_geometry_source(
            "custom",
            style.CustomGeometrySourceOptions(
                has_cancel_tile=True,
                max_queued_events=1,
            ),
        )

        source._native.push_fetch_for_test(1, 2, 3)
        source._native.push_cancel_for_test(4, 5, 6)

        event = source.poll_event()
        assert event == style.CustomGeometrySourceEvent(
            style.CustomGeometrySourceEventType.FETCH_TILE,
            style.CanonicalTileId(1, 2, 3),
        )
        assert source.poll_event() is None
        assert source.dropped_event_count == 1

        tile = style.CanonicalTileId(0, 0, 0)
        data = _json_object(
            {
                "type": "FeatureCollection",
                "features": [
                    {
                        "type": "Feature",
                        "geometry": {"type": "Point", "coordinates": [0.0, 0.0]},
                        "properties": {},
                    }
                ],
            }
        )
        bounds = geo.LatLngBounds(
            southwest=geo.LatLng(-1.0, -1.0),
            northeast=geo.LatLng(1.0, 1.0),
        )
        map_handle.set_custom_geometry_source_tile_data("custom", tile, data)
        map_handle.invalidate_custom_geometry_source_tile("custom", tile)
        map_handle.invalidate_custom_geometry_source_region("custom", bounds)
        source.close()
        assert source.closed

        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        assert source.closed


def test_style_url_load_releases_custom_geometry_state_when_the_style_lands() -> None:
    handles: list[resource.ResourceRequestHandle] = []

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if request.requested_url != "custom://geometry-style.json":
            return resource.ResourceProviderDecision.PASS_THROUGH
        handles.append(handle)
        return resource.ResourceProviderDecision.HANDLE

    with mln.RuntimeHandle() as runtime:
        runtime.set_resource_provider(provider, max_pending_callbacks=4)
        with runtime.create_map() as map_handle:
            map_handle.set_style_json(_EMPTY_STYLE_BYTES)
            source = map_handle.add_custom_geometry_source(
                "custom",
                style.CustomGeometrySourceOptions(max_queued_events=1),
            )

            # The inline load above queued a style-loaded event, so drain it to
            # keep the wait below tied to the URL load.
            _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)

            map_handle.set_style_url("custom://geometry-style.json")
            handle = _wait_for_provider_handle(runtime, handles)
            # Requesting the new style does not drop the source, so the callback
            # state stays live until the replacement style actually lands.
            assert source.closed is False

            handle.complete(resource.ResourceResponse(bytes=_EMPTY_STYLE_BYTES))
            _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)

            assert source.closed
            source._native.push_fetch_for_test(1, 2, 3)
            assert source.poll_event() is None


def _load_style_and_collect_types(
    runtime: mln.RuntimeHandle,
    map_handle: mln.MapHandle,
    until: mln.RuntimeEventType | None,
) -> set[mln.RuntimeEventType]:
    """Load the empty style, collecting event types until `until` arrives."""
    map_handle.set_style_json(_EMPTY_STYLE_BYTES)
    types: set[mln.RuntimeEventType] = set()
    for _ in range(500):
        runtime.pump()
        types.update(event.event_type for event in runtime.drain_events().events)
        if until is not None and until in types:
            break
        time.sleep(0.001)
    return types


def test_style_json_load_releases_custom_geometry_state_with_style_loaded_cleared() -> (
    None
):
    style_loaded = mln.RuntimeEventType.MAP_STYLE_LOADED

    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        # The same drive reports a style-loaded event while the type is
        # selected, so the cleared phase below asserts a real negative.
        assert style_loaded in _load_style_and_collect_types(
            runtime, map_handle, style_loaded
        )
        source = map_handle.add_custom_geometry_source(
            "custom-masked",
            style.CustomGeometrySourceOptions(max_queued_events=1),
        )
        assert source.closed is False

        map_handle.set_event_mask(
            mln.RuntimeEventMask.ALL & ~mln.RuntimeEventMask.MAP_STYLE_LOADED
        )
        types = _load_style_and_collect_types(runtime, map_handle, None)

        # The C API releases the dropped source itself, so the release does not
        # depend on a style-loaded event reaching the batch.
        assert source.closed
        assert style_loaded not in types


def test_remove_style_source_releases_custom_geometry_handle() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        source = map_handle.add_custom_geometry_source(
            "custom-remove",
            style.CustomGeometrySourceOptions(max_queued_events=1),
        )

        assert map_handle.remove_style_source("custom-remove") is True
        assert source.closed
        source._native.push_fetch_for_test(1, 2, 3)
        assert source.poll_event() is None


def test_map_close_releases_custom_geometry_handle() -> None:
    with mln.RuntimeHandle() as runtime:
        map_handle = runtime.create_map()
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        source = map_handle.add_custom_geometry_source(
            "custom-close",
            style.CustomGeometrySourceOptions(max_queued_events=1),
        )

        map_handle.close()

        assert source.closed
        source._native.push_fetch_for_test(1, 2, 3)
        assert source.poll_event() is None
        map_handle.close()


def test_custom_geometry_source_rejects_empty_queue_capacity() -> None:
    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        map_handle.set_style_json(_EMPTY_STYLE_BYTES)
        with pytest.raises(mln.InvalidArgumentError):
            map_handle.add_custom_geometry_source(
                "custom",
                style.CustomGeometrySourceOptions(max_queued_events=0),
            )


def test_set_bounds_rejects_unsupported_constraint() -> None:
    # Annotations do not bind at runtime, so a stale LatLngBounds would
    # otherwise be treated as absent and silently leave the constraint alone.
    world = geo.LatLngBounds(
        southwest=geo.LatLng(-90.0, -180.0),
        northeast=geo.LatLng(90.0, 180.0),
    )

    with mln.RuntimeHandle() as runtime, runtime.create_map() as map_handle:
        with pytest.raises(mln.InvalidArgumentError):
            map_handle.set_bounds(
                camera.BoundOptions(bounds=world)  # type: ignore[arg-type]
            )
        assert map_handle.get_bounds().bounds == camera.Unbounded()


@pytest.mark.parametrize("transition_id", [-1, 2**64])
def test_transition_id_out_of_range_raises_binding_error(transition_id: int) -> None:
    # PyO3 extracts this as Option<u64>, so without a range check the caller
    # would see a bare OverflowError instead of the binding's error shape.
    with (
        mln.RuntimeHandle() as runtime,
        runtime.create_map() as map_handle,
        pytest.raises(mln.InvalidArgumentError),
    ):
        map_handle.ease_to(
            camera.CameraOptions(zoom=2.0),
            camera.AnimationOptions(duration_ms=0.0, transition_id=transition_id),
        )


def test_released_map_id_replayed_after_a_new_map_reports_it_stale() -> None:
    """BND-045."""
    with mln.RuntimeHandle() as runtime:
        first = runtime.create_map()
        released = first._native.id()
        first.close()

        # The released slot is the one the next map takes, so the replayed id
        # names a retired generation of a slot that is live again.
        second = runtime.create_map()

        with pytest.raises(mln.InvalidArgumentError) as excinfo:
            _native.map_size_by_id_for_test(released)
        assert "stale" in str(excinfo.value)

        # The live map is unaffected by the replay.
        assert _native.map_size_by_id_for_test(second._native.id())[0] > 0
        second.close()


def test_map_id_passed_to_a_runtime_operation_reports_invalid_argument() -> None:
    """BND-047."""
    with mln.RuntimeHandle() as runtime:
        map_handle = runtime.create_map()

        with pytest.raises(mln.InvalidArgumentError) as excinfo:
            _native.pump_runtime_with_map_id_for_test(map_handle._native.id())
        message = str(excinfo.value)
        assert "map" in message
        assert "runtime" in message

        map_handle.close()


def test_live_map_id_called_from_another_thread_reports_wrong_thread() -> None:
    """BND-049."""
    with mln.RuntimeHandle() as runtime:
        map_handle = runtime.create_map()
        live = map_handle._native.id()

        failures: list[BaseException] = []

        def call_from_other_thread() -> None:
            try:
                _native.map_size_by_id_for_test(live)
            except BaseException as error:  # noqa: BLE001 - recorded for the assert
                failures.append(error)

        thread = threading.Thread(target=call_from_other_thread)
        thread.start()
        thread.join()

        # The id is live, so the owner-thread rule decides rather than identity.
        assert len(failures) == 1
        assert isinstance(failures[0], mln.WrongThreadError)
        assert "stale" not in str(failures[0])

        map_handle.close()

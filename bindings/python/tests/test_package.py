import contextlib
import http.server
import math
from pathlib import Path
import subprocess
import sys
import textwrap
import threading
import time
import typing
import warnings

import pytest

import maplibre_native as mln
from maplibre_native import _native
from maplibre_native import (
    camera,
    geo,
    json,
    log,
    map as map_module,
    offline,
    query,
    render,
    resource,
    style,
)

_EMPTY_STYLE_JSON = '{"version":8,"sources":{},"layers":[]}'
_EMPTY_STYLE_BYTES = _EMPTY_STYLE_JSON.encode()


def _json_object(value: object) -> json.JsonObject:
    converted = json.from_python(value)
    assert isinstance(converted, json.JsonObject)
    return converted


def _json_value(value: object) -> json.JsonValue:
    return json.from_python(value)


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
        def do_GET(self) -> None:  # noqa: N802
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
        runtime.run_once()
        while event := runtime.poll_event():
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
        runtime.run_once()
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
    operation_id = operation._operation_id  # noqa: SLF001
    for _ in range(iterations):
        runtime.run_once()
        while event := runtime.poll_event():
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
        maximum_cache_size: int | None,
    ) -> object:
        return _native.create_runtime_with_abi_version_for_test(
            actual_abi_version,
            asset_path,
            cache_path,
            maximum_cache_size,
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
    assert "maplibre_native.json.JsonObject" in repr(map_hints["source_json"])

    layer_hints = typing.get_type_hints(map_module.MapHandle.set_layer_property)
    assert layer_hints["value"] != typing.Any
    assert "maplibre_native.json.JsonUInt" in repr(layer_hints["value"])

    style_hints = typing.get_type_hints(map_module.MapHandle.get_style_layer_json)
    assert style_hints["return"] != typing.Any
    assert "maplibre_native.json.JsonObject" in repr(style_hints["return"])

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
    assert extension_hints["feature"] is geo.Feature
    assert extension_hints["return"] is query.FeatureExtensionResult
    assert extension_hints["arguments"] != typing.Any
    assert "maplibre_native.json.JsonObject" in repr(extension_hints["arguments"])

    rendered_hints = typing.get_type_hints(
        render.RenderSessionHandle.query_rendered_features
    )
    assert rendered_hints["geometry"] is query.RenderedQueryGeometry
    assert rendered_hints["options"] != typing.Any

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
        runtime.run_once()

    assert runtime.closed
    runtime.close()
    assert runtime.closed


def test_closed_handle_finalizers_are_quiet_at_interpreter_shutdown() -> None:
    script = textwrap.dedent(
        """
        import maplibre_native as mln
        from maplibre_native import style

        runtime = mln.RuntimeHandle()
        map_handle = runtime.create_map()
        map_handle.set_style_json('{"version":8,"sources":{},"layers":[]}')
        source = map_handle.add_custom_geometry_source(
            "custom",
            style.CustomGeometrySourceOptions(max_queued_events=1),
        )
        source.close()
        map_handle.close()
        runtime.close()
        """
    )

    completed = subprocess.run(  # noqa: S603
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
        except BaseException as error:
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
        for call in (runtime.run_once, runtime.poll_event, map_handle.request_repaint):
            try:
                call()
            except BaseException as error:
                raised_errors.append(error)

    thread = threading.Thread(target=call_owner_thread_methods)
    thread.start()
    thread.join()

    try:
        assert len(raised_errors) == 3
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
            except BaseException as error:
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
    feature = geo.Feature(geometry=geo.Point(geo.LatLng(1.0, 2.0)))
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
                json.JsonObject((json.JsonMember("limit", json.JsonInt(10)),)),
            )
        ),
        "set_feature_state": (
            lambda: session.set_feature_state(
                selector,
                json.JsonObject((json.JsonMember("hover", True),)),
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
            except BaseException as error:
                raised_errors[name] = error

    thread = threading.Thread(target=call_render_session_methods)
    thread.start()
    thread.join()

    assert set(raised_errors) == set(calls)
    for name, error in raised_errors.items():
        assert_wrong_thread_error(error, diagnostics[name])


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
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map(mln.MapOptions(mode=mln.MapMode.STATIC)) as static_map:
            static_map.request_still_image()

    with mln.RuntimeHandle() as runtime:
        with runtime.create_map(
            mln.MapOptions(mode=mln.MapMode.CONTINUOUS)
        ) as map_handle:
            with pytest.raises(mln.InvalidStateError) as raised:
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
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
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
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            stale = _native_invalid_network_status_error().diagnostic

            with pytest.raises(mln.InvalidArgumentError) as raised:
                map_handle.set_style_url("bad\0url")

    assert raised.value.status == mln.MaplibreStatus.INVALID_ARGUMENT
    assert raised.value.native_status_code is None
    assert "embedded NUL" in raised.value.diagnostic
    assert raised.value.diagnostic != stale


def test_style_source_url_metadata_and_removal_public_api() -> None:
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            map_handle.set_style_json('{"version":8,"sources":{},"layers":[]}')
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
                "points", "https://example.test/points.geojson"
            )
            inline_points = geo.FeatureCollection(
                (
                    geo.Feature(
                        geometry=geo.Point(geo.LatLng(1.0, 2.0)),
                        properties=(json.JsonMember("name", "one"),),
                        identifier=geo.FeatureIdentifierString("point-1"),
                    ),
                )
            )
            map_handle.add_geojson_source_data("inline-points", inline_points)
            map_handle.set_geojson_source_url(
                "inline-points",
                "https://example.test/inline-points.geojson",
            )
            map_handle.set_geojson_source_data("inline-points", inline_points)
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
                ("https://example.test/vector/{z}/{x}/{y}.pbf",),
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
                map_handle.get_style_source_type("points")
                == style.StyleSourceType.GEOJSON
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
            assert map_handle.get_style_source_info("missing") is None

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

    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            map_handle.set_style_json('{"version":8,"sources":{},"layers":[]}')
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
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            map_handle.set_style_json('{"version":8,"sources":{},"layers":[]}')
            map_handle.add_geojson_source_url(
                "points",
                "https://example.test/points.geojson",
            )
            with pytest.raises(TypeError, match="unsupported JSON value: dict"):
                map_handle.add_style_layer_json(
                    typing.cast(
                        typing.Any,
                        {"id": "raw-dict", "type": "background"},
                    )
                )
            with pytest.raises(TypeError, match="unsupported JSON value: int"):
                map_handle.set_style_light_property(
                    "intensity",
                    typing.cast(typing.Any, 1),
                )
            map_handle.add_style_layer_json(background)
            map_handle.add_style_layer_json(circle)
            map_handle.set_layer_property(
                "json-background",
                "background-color",
                "#ff0000",
            )
            map_handle.set_layer_filter("json-circle", filter_value)
            map_handle.set_style_light_json(_json_object({"anchor": "viewport"}))
            map_handle.set_style_light_property("intensity", json.JsonDouble(0.5))

            layer_json = map_handle.get_style_layer_json("json-background")
            assert layer_json is not None
            assert ("id", "json-background") in json.to_python(layer_json)
            assert map_handle.get_style_layer_json("missing") is None
            background_color = map_handle.get_layer_property(
                "json-background",
                "background-color",
            )
            assert isinstance(background_color, json.JsonArray)
            assert background_color.values[0] == "rgba"
            assert (
                json.to_python(map_handle.get_layer_filter("json-circle")) == raw_filter
            )
            assert map_handle.get_style_light_property("anchor") == "viewport"
            assert map_handle.get_style_light_property("intensity") == json.JsonDouble(
                0.5
            )

            with pytest.raises(ValueError, match="finite"):
                map_handle.set_style_light_property(
                    "intensity", json.JsonDouble(math.inf)
                )

            map_handle.set_layer_filter("json-circle", None)
            assert map_handle.get_layer_filter("json-circle") is None


def test_style_image_metadata_copy_and_removal_public_api() -> None:
    image = render.PremultipliedRgba8Image(
        info=render.TextureImageInfo(width=1, height=1, stride=4, byte_length=4),
        data=bytes([255, 0, 0, 255]),
    )
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            map_handle.set_style_json('{"version":8,"sources":{},"layers":[]}')
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
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            map_handle.set_style_json('{"version":8,"sources":{},"layers":[]}')
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


def test_style_layer_metadata_move_and_removal_public_api() -> None:
    style_json = """
    {
      "version": 8,
      "sources": {},
      "layers": [
        {"id": "background-a", "type": "background"},
        {"id": "background-b", "type": "background"}
      ]
    }
    """
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
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

    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            map_handle.set_viewport_options(viewport)
            map_handle.set_tile_options(tile)

            assert map_handle.get_viewport_options() == viewport
            assert map_handle.get_tile_options() == tile


def test_camera_snapshot_and_jump_round_trip_public_values() -> None:
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
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


def test_free_camera_and_projection_mode_round_trip_public_values() -> None:
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
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

    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            map_handle.set_bounds(
                camera.BoundOptions(bounds=bounds, min_zoom=0.0, max_zoom=10.0)
            )
            constraints = map_handle.get_bounds()
            fit_bounds = map_handle.camera_for_lat_lng_bounds(bounds, fit)
            fit_coordinates = map_handle.camera_for_lat_lngs(
                (bounds.southwest, bounds.northeast),
                fit,
            )
            fit_geometry = map_handle.camera_for_geometry(
                geo.LineString((bounds.southwest, bounds.northeast)),
                fit,
            )
            visible_bounds = map_handle.lat_lng_bounds_for_camera(target)
            unwrapped_bounds = map_handle.lat_lng_bounds_for_camera(
                target,
                unwrapped=True,
            )

            assert constraints.bounds == bounds
            assert constraints.min_zoom == pytest.approx(0.0)
            assert constraints.max_zoom == pytest.approx(10.0)
            assert isinstance(fit_bounds, camera.CameraOptions)
            assert isinstance(fit_coordinates, camera.CameraOptions)
            assert isinstance(fit_geometry, camera.CameraOptions)
            assert isinstance(visible_bounds, geo.LatLngBounds)
            assert isinstance(unwrapped_bounds, geo.LatLngBounds)


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

    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
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


def test_poll_event_returns_none_when_queue_is_empty() -> None:
    with mln.RuntimeHandle() as runtime:
        assert runtime.poll_event() is None


def test_poll_event_returns_copied_map_event() -> None:
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            with pytest.raises((mln.InvalidArgumentError, mln.NativeError)):
                map_handle.set_style_json("{")

            loading_failed = None
            for _ in range(8):
                event = runtime.poll_event()
                if event is None:
                    break
                if event.event_type == mln.RuntimeEventType.MAP_LOADING_FAILED:
                    loading_failed = event
                    break

            assert loading_failed is not None
            copied_message = loading_failed.message
            runtime.poll_event()

            assert loading_failed.event_type == mln.RuntimeEventType.MAP_LOADING_FAILED
            assert loading_failed.source.source_type == mln.RuntimeEventSourceType.MAP
            assert loading_failed.source.map_handle is map_handle
            assert copied_message == loading_failed.message
            assert loading_failed.message


def test_run_once_and_poll_event_return_copied_style_loaded_event() -> None:
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            map_handle.set_style_json('{"version":8,"sources":{},"layers":[]}')

            style_loaded = None
            for _ in range(32):
                runtime.run_once()
                while event := runtime.poll_event():
                    if event.event_type == mln.RuntimeEventType.MAP_STYLE_LOADED:
                        style_loaded = event
                        break
                if style_loaded is not None:
                    break

            assert style_loaded is not None
            assert style_loaded.source.source_type == mln.RuntimeEventSourceType.MAP
            assert style_loaded.source.map_handle is map_handle
            runtime.run_once()
            assert runtime.poll_event() is None


def test_runtime_event_payload_wire_shapes_include_native_fields() -> None:
    events = _native.runtime_event_payload_wire_shapes_for_test()

    render_frame_event = mln.RuntimeEvent._from_native(events["render_frame"])
    assert isinstance(render_frame_event.payload, mln.RenderFramePayload)
    assert render_frame_event.payload.mode == mln.RenderMode.FULL

    render_frame_payload = events["render_frame"]["payload"]
    render_frame = mln.RenderFramePayload._from_runtime_payload(render_frame_payload)
    assert render_frame.mode == mln.RenderMode.FULL
    assert render_frame.needs_repaint is True
    assert render_frame.placement_changed is False
    assert render_frame.stats.encoding_time == pytest.approx(1.25)
    assert render_frame.stats.rendering_time == pytest.approx(2.5)
    assert render_frame.stats.frame_count == 3
    assert render_frame.stats.draw_call_count == 4
    assert render_frame.stats.total_draw_call_count == 5

    tile_action_payload = events["tile_action"]["payload"]
    tile_action_event = mln.RuntimeEvent._from_native(events["tile_action"])
    assert isinstance(tile_action_event.payload, mln.TileActionPayload)
    tile_action = mln.TileActionPayload._from_runtime_payload(tile_action_payload)
    assert tile_action.operation == mln.TileOperation.LOAD_FROM_NETWORK
    assert tile_action.tile_id.overscaled_z == 6
    assert tile_action.tile_id.wrap == -1
    assert tile_action.tile_id.canonical_z == 5
    assert tile_action.tile_id.canonical_x == 12
    assert tile_action.tile_id.canonical_y == 34
    assert tile_action.source_id == "source-a"

    offline_status_payload = events["offline_region_status"]["payload"]
    status_changed = offline.OfflineRegionStatusChanged._from_runtime_payload(
        offline_status_payload
    )
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


def test_render_descriptors_are_public_python_values() -> None:
    extent = render.RenderTargetExtent(width=320, height=240, scale_factor=2.0)
    pointer = render.NativePointer(0x1234)
    metal = render.MetalOwnedTextureDescriptor(
        extent=extent,
        context=render.MetalContextDescriptor(device=pointer),
    )
    vulkan = render.VulkanBorrowedTextureDescriptor(
        extent=extent,
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
        context=render.WglContextDescriptor(
            device_context=pointer,
            share_context=render.NativePointer(0x8888),
            get_proc_address=render.NativePointer(0x9999),
        ),
        texture=5,
        target=0x0DE1,
    )

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


def test_render_session_query_public_api_uses_query_and_geojson_wire_values() -> None:
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
            return [queried_feature_native()]

        def query_source_features(
            self,
            source_id: str,
            source_layer_ids: tuple[str, ...] | None,
            filter_: object,
        ) -> list[dict[str, object]]:
            self.source_call = (source_id, source_layer_ids, filter_)
            return [queried_feature_native()]

        def query_feature_extensions(
            self,
            source_id: str,
            feature: object,
            extension: str,
            extension_field: str,
            arguments: object,
        ) -> dict[str, object]:
            self.extension_call = (
                source_id,
                feature,
                extension,
                extension_field,
                arguments,
            )
            return {"type": 1, "value": json.JsonUInt(7)}

    def queried_feature_native() -> dict[str, object]:
        return {
            "feature": geo.Feature(
                geometry=geo.Point(geo.LatLng(1.0, 2.0)),
                properties=(json.JsonMember("name", "one"),),
                identifier=geo.FeatureIdentifierString("feature-1"),
            ),
            "source_id": "points",
            "source_layer_id": None,
            "state": json.JsonObject((json.JsonMember("hover", True),)),
        }

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
    feature = geo.Feature(
        geometry=geo.Point(geo.LatLng(1.0, 2.0)),
        properties=(json.JsonMember("name", "one"),),
        identifier=geo.FeatureIdentifierString("feature-1"),
    )

    rendered = session.query_rendered_features(geometry, rendered_options)
    source = session.query_source_features("points", source_options)
    extension = session.query_feature_extensions(
        "points",
        feature,
        "supercluster",
        "leaves",
        _json_object({"limit": 10}),
    )

    assert fake_native.rendered_call == (
        {"type": "point", "point": (1.0, 2.0)},
        ("circle",),
        _json_value(["==", ["get", "kind"], "park"]),
    )
    assert fake_native.source_call[0] == "points"
    assert fake_native.source_call[1] == ("landuse",)
    assert rendered[0].feature == feature
    assert source[0].state == json.JsonObject((json.JsonMember("hover", True),))
    assert fake_native.extension_call == (
        "points",
        feature,
        "supercluster",
        "leaves",
        _json_object({"limit": 10}),
    )
    assert extension == query.FeatureExtensionResult.value_result(json.JsonUInt(7))


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


def test_render_session_feature_state_public_api_uses_json_values() -> None:
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
        ) -> object:
            assert (source_id, source_layer_id, feature_id, state_key) == (
                "points",
                "symbols",
                "feature-1",
                "hover",
            )
            return json.JsonObject((json.JsonMember("hover", True),))

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
    state = json.JsonObject((json.JsonMember("hover", True),))

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
    assert json.to_python(returned) == [("hover", True)]
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
        ) -> object:
            assert (source_id, source_layer_id, feature_id, state_key) == (
                "points",
                None,
                "feature-1",
                None,
            )
            return json.JsonObject(())

    session = render.RenderSessionHandle._from_native(
        FakeNativeRenderSession(), object()
    )
    selector = query.FeatureStateSelector(source_id="points", feature_id="feature-1")

    assert json.to_python(session.get_feature_state(selector)) == []


def test_invalid_render_target_attach_reports_native_status() -> None:
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            with pytest.raises(
                (mln.InvalidArgumentError, mln.UnsupportedFeatureError)
            ) as raised:
                map_handle.attach_metal_owned_texture(
                    render.MetalOwnedTextureDescriptor()
                )

            assert raised.value.status in {
                mln.MaplibreStatus.INVALID_ARGUMENT,
                mln.MaplibreStatus.UNSUPPORTED,
            }


def test_invalid_opengl_render_target_attach_reports_native_status() -> None:
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
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
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
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

    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
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
                    geo.LineString((geo.LatLng(-1.0, -1.0), geo.LatLng(1.0, 1.0))),
                    camera.EdgeInsets(),
                )

                assert not projection.closed
                assert isinstance(projection.get_camera(), camera.CameraOptions)
                assert isinstance(point, camera.ScreenPoint)
                assert math.isfinite(projected.latitude)
                assert math.isfinite(projected.longitude)

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
    assert runtime._native.status_takes == 1  # noqa: SLF001

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

    assert runtime._native.discarded == list(range(20, 26))  # noqa: SLF001
    assert runtime._native.status_takes == 1  # noqa: SLF001
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
        filter=json.JsonArray(("==", "class", "park")),
    )
    source_options = query.SourceFeatureQueryOptions(source_layer_ids=("landuse",))
    selector = query.FeatureStateSelector(
        source_id="source",
        source_layer_id="layer",
        feature_id="feature-1",
        state_key="hover",
    )
    feature = geo.Feature(geo.Point(geo.LatLng(0.0, 0.0)))
    queried = query.QueriedFeature(
        feature=feature,
        source_id="source",
        source_layer_id="layer",
        state=json.JsonObject((json.JsonMember("hover", True),)),
    )
    extension = query.FeatureExtensionResult.feature_collection_result((feature,))

    assert geometry.point == point
    assert box_geometry.box is not None
    assert line_geometry.line_string == (
        camera.ScreenPoint(0.0, 0.0),
        camera.ScreenPoint(5.0, 5.0),
    )
    assert rendered_options.layer_ids == ("landuse",)
    assert source_options.source_layer_ids == ("landuse",)
    assert selector.state_key == "hover"
    assert queried.source_id == "source"
    assert extension.feature_collection == (feature,)


def test_feature_extension_result_preserves_unknown_native_type() -> None:
    extension = query.FeatureExtensionResult._from_native({"type": 999_001})

    assert extension.type is query.FeatureExtensionResultType.UNKNOWN
    assert extension.raw_type == 999_001


def test_query_selector_rejects_state_key_without_feature_id() -> None:
    with pytest.raises(ValueError, match="state_key requires feature_id"):
        query.FeatureStateSelector(source_id="source", state_key="hover")


def test_process_global_logging_receiver_copies_native_records() -> None:
    receiver = log.set_log_callback(max_queued_records=8, consume=True)
    try:
        log.set_async_log_severity_mask(log.LogSeverityMask.DEFAULT)
        with mln.RuntimeHandle() as runtime:
            with runtime.create_map() as map_handle:
                with pytest.raises((mln.InvalidArgumentError, mln.NativeError)):
                    map_handle.set_style_json("{")
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
        with mln.RuntimeHandle() as runtime:
            with runtime.create_map() as map_handle:
                with pytest.raises((mln.InvalidArgumentError, mln.NativeError)):
                    map_handle.set_style_json("{")
                map_handle.dump_debug_logs()

        assert receiver.poll_record() is not None
        assert receiver.dropped_record_count >= 0
    finally:
        log.clear_log_callback()


def test_json_values_preserve_order_duplicates_and_numeric_shape() -> None:
    value = json.JsonObject(
        (
            json.JsonMember("same", json.JsonUInt(1)),
            json.JsonMember("same", json.JsonInt(-1)),
            json.JsonMember("nested", json.JsonArray((True, json.JsonDouble(1.5)))),
        )
    )

    assert json.to_python(value) == [
        ("same", 1),
        ("same", -1),
        ("nested", [True, 1.5]),
    ]
    assert value.members[0].value == json.JsonUInt(1)
    assert value.members[1].value == json.JsonInt(-1)


def test_geojson_values_preserve_geometry_and_properties() -> None:
    feature = geo.Feature(
        geometry=geo.LineString((geo.LatLng(1.0, 2.0), geo.LatLng(3.0, 4.0))),
        properties=(
            json.JsonMember("name", "road"),
            json.JsonMember("name", "duplicate"),
        ),
        identifier=geo.FeatureIdentifierString("feature-1"),
    )
    collection = geo.FeatureCollection((feature,))

    assert collection.features[0].geometry == geo.LineString(
        (geo.LatLng(1.0, 2.0), geo.LatLng(3.0, 4.0))
    )
    assert [member.key for member in collection.features[0].properties] == [
        "name",
        "name",
    ]
    assert collection.features[0].identifier == geo.FeatureIdentifierString("feature-1")


def test_resource_values_preserve_native_shape() -> None:
    request = resource.ResourceRequest._from_native(
        {
            "url": "https://example.test/tile.pbf",
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
        assert request.url == "https://example.test/tile.pbf"
        assert handle.closed is False
        return resource.ResourceProviderDecision.PASS_THROUGH

    native = FakeNativeRequest()
    adapted = resource._adapt_resource_provider_callback(provider)  # noqa: SLF001

    raw_request = {
        "url": "https://example.test/tile.pbf",
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
        assert request.url == "https://example.test/tile.pbf"
        assert handle.closed is False
        msg = "contained provider failure"
        raise RuntimeError(msg)

    raw_request = {
        "url": "https://example.test/tile.pbf",
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
    adapted = resource._adapt_resource_provider_callback(provider)  # noqa: SLF001

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

    with mln.RuntimeHandle() as runtime:
        with pytest.raises(mln.InvalidArgumentError):
            runtime.set_resource_provider(provider, max_pending_callbacks=0)

        runtime.set_resource_provider(provider, max_pending_callbacks=1)
        with runtime.create_map():
            with pytest.raises(mln.InvalidStateError):
                runtime.set_resource_provider(provider, max_pending_callbacks=1)


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

    with _online_network(), _http_style_server() as (rewritten_style_url, served):
        with mln.RuntimeHandle() as runtime:
            runtime.set_resource_transform(transform, max_pending_callbacks=4)
            with runtime.create_map() as map_handle:
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

    with _online_network(), _http_style_server() as (style_url, served):
        with mln.RuntimeHandle() as runtime:
            runtime.set_resource_transform(transform, max_pending_callbacks=1)
            with runtime.create_map() as map_handle:
                runtime.clear_resource_transform()
                map_handle.set_style_url(style_url)
                _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)

    assert served.is_set()
    assert calls == []


def test_resource_provider_inline_completion_overrides_pass_through_return() -> None:
    completions = 0

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        nonlocal completions
        if request.url != "custom://inline-style.json":
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
        if not request.url.startswith("custom://deferred-style"):
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
    thread_errors: list[BaseException] = []

    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if request.url != "custom://cross-thread-style.json":
            return resource.ResourceProviderDecision.PASS_THROUGH
        handles.append(handle)
        return resource.ResourceProviderDecision.HANDLE

    def complete_on_thread(handle: resource.ResourceRequestHandle) -> None:
        try:
            handle.complete(resource.ResourceResponse(bytes=_EMPTY_STYLE_BYTES))
        except BaseException as error:  # pragma: no cover - re-raised below
            thread_errors.append(error)

    with mln.RuntimeHandle() as runtime:
        runtime.set_resource_provider(provider, max_pending_callbacks=4)
        with runtime.create_map() as map_handle:
            map_handle.set_style_url("custom://cross-thread-style.json")
            handle = _wait_for_provider_handle(runtime, handles)

            thread = threading.Thread(target=complete_on_thread, args=(handle,))
            thread.start()
            thread.join(timeout=2)

            assert not thread.is_alive()
            assert thread_errors == []
            _wait_for_runtime_event(runtime, mln.RuntimeEventType.MAP_STYLE_LOADED)


def test_resource_provider_error_response_reports_loading_failure_event() -> None:
    def provider(
        request: resource.ResourceRequest,
        handle: resource.ResourceRequestHandle,
    ) -> resource.ResourceProviderDecision:
        if request.url != "custom://error-style.json":
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
        if request.url != "custom://offline-error-style.json":
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
        if request.url != "custom://cancelled-style.json":
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
            runtime.run_once()
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
        if not request.url.startswith("custom://stale-style"):
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
        if request.url != "custom://release-race-style.json":
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
            unexpected_errors: list[BaseException] = []

            def probe_cancelled() -> None:
                while not stop.is_set():
                    try:
                        handle.is_cancelled()
                        started.set()
                    except mln.InvalidArgumentError, mln.InvalidStateError:
                        saw_closed.set()
                        stop.set()
                    except BaseException as error:  # pragma: no cover - re-raised below
                        unexpected_errors.append(error)
                        stop.set()

            thread = threading.Thread(target=probe_cancelled)
            thread.start()
            assert started.wait(timeout=2)
            handle.close()
            assert saw_closed.wait(timeout=2)
            stop.set()
            thread.join(timeout=2)

            assert not thread.is_alive()
            assert unexpected_errors == []
            with pytest.raises(mln.InvalidStateError, match="already closed"):
                handle.is_cancelled()


def test_custom_geometry_source_scaffolding_queues_copied_events() -> None:
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            map_handle.set_style_json('{"version":8,"sources":{},"layers":[]}')
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
            data = geo.FeatureCollection(
                (geo.Feature(geometry=geo.Point(geo.LatLng(0.0, 0.0))),)
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

            map_handle.set_style_json('{"version":8,"sources":{},"layers":[]}')
            assert source.closed


def test_set_style_url_retires_custom_geometry_callback_state() -> None:
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            map_handle.set_style_json('{"version":8,"sources":{},"layers":[]}')
            source = map_handle.add_custom_geometry_source(
                "custom",
                style.CustomGeometrySourceOptions(max_queued_events=1),
            )

            map_handle.set_style_url("https://example.test/style.json")

            assert source.closed
            source._native.push_fetch_for_test(1, 2, 3)
            assert source.poll_event() is None


def test_remove_style_source_releases_custom_geometry_handle() -> None:
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            map_handle.set_style_json('{"version":8,"sources":{},"layers":[]}')
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
        map_handle.set_style_json('{"version":8,"sources":{},"layers":[]}')
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
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            map_handle.set_style_json('{"version":8,"sources":{},"layers":[]}')
            with pytest.raises(mln.InvalidArgumentError):
                map_handle.add_custom_geometry_source(
                    "custom",
                    style.CustomGeometrySourceOptions(max_queued_events=0),
                )

from __future__ import annotations

import json
import time
from collections.abc import Callable
from concurrent.futures import Future

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import camera, geo, query, render, style

_DEFAULT_GPU_SYNC = render.GpuSync()


EMPTY_STYLE_JSON = '{"version":8,"sources":{},"layers":[]}'

RED_BACKGROUND_STYLE_JSON = (
    '{"version":8,"sources":{},"layers":['
    '{"id":"background","type":"background",'
    '"paint":{"background-color":"#ff0000"}}]}'
)
"""A style that paints every pixel, for tests that read a target back."""

RED_PIXEL = b"\xff\x00\x00\xff"

CLUSTER_POINTS = json.dumps(
    {
        "type": "FeatureCollection",
        "features": [
            {
                "type": "Feature",
                "geometry": {"type": "Point", "coordinates": [offset, offset]},
                "properties": {"name": name, "weight": weight},
            }
            for offset, name, weight in (
                (0.0, "one", 1),
                (0.001, "two", 2),
                (0.002, "three", 3),
            )
        ],
    },
    separators=(",", ":"),
).encode()

CLUSTER_STYLE_JSON = (
    '{"version":8,"name":"python-cluster-query-test",'
    '"sources":{"cluster-source":{"type":"geojson","cluster":true,'
    '"data":{"type":"FeatureCollection","features":['
    '{"type":"Feature","geometry":{"type":"Point","coordinates":[0.0,0.0]},'
    '"properties":{"name":"one"}},'
    '{"type":"Feature","geometry":{"type":"Point","coordinates":[0.001,0.001]},'
    '"properties":{"name":"two"}},'
    '{"type":"Feature","geometry":{"type":"Point","coordinates":[0.002,0.002]},'
    '"properties":{"name":"three"}}]}}},'
    '"layers":[{"id":"background","type":"background",'
    '"paint":{"background-color":"#ffffff"}},'
    '{"id":"cluster-circle","type":"circle","source":"cluster-source",'
    '"filter":["has","point_count"],'
    '"paint":{"circle-color":"#2563eb","circle-radius":20}}]}'
)


def is_configured_render_backend(
    backend: str,
    *,
    context_provider: str | None = None,
) -> bool:
    backend_flag = {
        "metal": mln.RenderBackend.METAL,
        "opengl": mln.RenderBackend.OPENGL,
        "vulkan": mln.RenderBackend.VULKAN,
    }[backend]
    if not mln.supported_render_backends() & backend_flag:
        return False
    if context_provider is None:
        return True
    context_provider_flag = {
        "egl": mln.OpenGLContextProvider.EGL,
        "wgl": mln.OpenGLContextProvider.WGL,
    }[context_provider]
    return bool(mln.supported_opengl_context_providers() & context_provider_flag)


def skip_or_fail_fixture_setup(
    reason: str,
    backend: str,
    *,
    context_provider: str | None = None,
    allow_module_level: bool = False,
) -> None:
    if is_configured_render_backend(backend, context_provider=context_provider):
        pytest.fail(reason)
    pytest.skip(reason, allow_module_level=allow_module_level)


def wait_for_runtime_event(
    runtime: mln.RuntimeHandle,
    event_type: mln.RuntimeEventType,
    *,
    iterations: int = 5000,
) -> mln.RuntimeEvent:
    for _ in range(iterations):
        for event in runtime.drain_events().events:
            if event.event_type == event_type:
                return event
        time.sleep(0.001)
    raise AssertionError(f"runtime event {event_type!r} was not observed")


def drain_frame_results(
    session: render.RenderSessionHandle,
) -> list[render.RenderFrameResult]:
    """Drain terminal frame results, reading an empty queue as no results."""
    try:
        return session.drain_frame_results()
    except mln.NotReadyError:
        # An empty queue reports NOT_READY, which is a poll result and not a
        # failure.
        return []


def request_and_finish_frame(
    session: render.RenderSessionHandle,
    *,
    token: int = 1,
    flags: render.FrameDemandFlag = render.FrameDemandFlag.IF_NEEDED,
    iterations: int = 5000,
) -> render.RenderFrameResult:
    """Request one frame and return the terminal result for its own token.

    Clearing ``IF_NEEDED`` renders even when nothing newer is pending, which
    is how a test gets a fresh frame out of a settled style.
    """
    session.request_frame(render.FrameDemand(flags=flags, token=token))
    for _ in range(iterations):
        if session.snapshot().driver == render.RenderDriver.CALLER_GRAPHICS_THREAD:
            session.service_driver_work(16)
        for result in drain_frame_results(session):
            if result.token == token:
                return result
        time.sleep(0.001)
    raise AssertionError(f"frame demand {token} did not produce a terminal result")


def finish_render_operation[T](
    session: render.RenderSessionHandle,
    operation: Future[T],
    *,
    iterations: int = 5000,
) -> T:
    """Complete renderer-affine work through either driver."""
    for _ in range(iterations):
        if session.snapshot().driver == render.RenderDriver.CALLER_GRAPHICS_THREAD:
            session.service_driver_work(16)
        if operation.done():
            return operation.result(timeout=0)
        time.sleep(0.001)
    raise AssertionError("render operation did not complete")


def assert_attached_session_shape(session: render.RenderSessionHandle) -> None:
    """Assert the public shape an attached session reports to a host."""
    assert isinstance(session, render.RenderSessionHandle)
    assert session.closed is False
    assert session.snapshot().state == render.RenderSessionState.ATTACHED
    assert isinstance(session.capabilities(), render.RenderSessionCapabilities)


def map_extent(map_handle: mln.MapHandle) -> tuple[int, int, float]:
    """Return the published logical extent of a map."""
    snapshot = map_handle.snapshot()
    return snapshot.width, snapshot.height, snapshot.scale_factor


def assert_invalid_state(call: Callable[[], object]) -> None:
    """Assert that a call reports the invalid-state category."""
    with pytest.raises(mln.InvalidStateError) as raised:
        call()
    assert raised.value.status == mln.MaplibreStatus.INVALID_STATE


def read_texture_info(
    runtime: mln.RuntimeHandle,
    map_handle: mln.MapHandle,
    session: render.RenderSessionHandle,
) -> render.TextureImageInfo:
    """Render one update and return the readback metadata for that frame."""
    map_handle.set_style_json(EMPTY_STYLE_JSON.encode())
    render_until_update(runtime, session)
    image = finish_render_operation(session, session.read_premultiplied_rgba8())
    return image.info


def assert_frame_demands_report_their_own_tokens(
    session: render.RenderSessionHandle,
) -> None:
    """Two outstanding demands report one terminal result each, in order."""
    first, second = 4001, 4002
    session.request_frame(
        render.FrameDemand(flags=render.FrameDemandFlag(0), token=first)
    )
    session.request_frame(
        render.FrameDemand(flags=render.FrameDemandFlag(0), token=second)
    )

    tokens: list[int] = []
    for _ in range(5000):
        if session.snapshot().driver == render.RenderDriver.CALLER_GRAPHICS_THREAD:
            session.service_driver_work(16)
        tokens.extend(result.token for result in drain_frame_results(session))
        if len(tokens) >= 2:
            break
        time.sleep(0.001)
    assert tokens == [first, second]


def assert_texture_ring_exhaustion_reports_not_ready(
    session: render.RenderSessionHandle,
    acquire: Callable[[], object],
) -> None:
    """Acquiring every ring slot leaves the next acquire with nothing to lease."""
    depth = session.capabilities().texture_ring_depth
    assert depth >= 1
    leases: list[object] = []
    try:
        for slot in range(depth):
            for _ in range(5000):
                request_and_finish_frame(
                    session, token=5000 + slot, flags=render.FrameDemandFlag(0)
                )
                try:
                    leases.append(acquire())
                    break
                except mln.NotReadyError:
                    time.sleep(0.001)
            else:
                raise AssertionError(f"ring slot {slot} never held a frame")
        assert session.snapshot().acquired_frame_count == depth

        # Every slot is leased out, so the next acquire reports NOT_READY
        # instead of waiting for one to come back.
        with pytest.raises(mln.NotReadyError) as raised:
            acquire()
        assert raised.value.status == mln.MaplibreStatus.NOT_READY

        # Held leases stay readable while the ring is exhausted.
        for lease in leases:
            assert lease.result.disposition == render.RenderResult.RENDERED
    finally:
        for lease in leases:
            lease.release()
    assert session.snapshot().acquired_frame_count == 0


def assert_session_maintenance_commands_round_trip(
    session: render.RenderSessionHandle,
) -> None:
    """Renderer-affine maintenance commands each reach their completion."""
    for operation in (
        session.reduce_memory_use(),
        session.clear_data(),
        session.dump_debug_logs(),
        session.barrier(),
    ):
        assert finish_render_operation(session, operation) is None

    while drain_frame_results(session):
        pass

    # A barrier reports earlier accepted work and produces no frame result, so
    # the drained queue stays empty and reports NOT_READY.
    finish_render_operation(session, session.barrier())
    with pytest.raises(mln.NotReadyError) as raised:
        session.drain_frame_results()
    assert raised.value.status == mln.MaplibreStatus.NOT_READY


def assert_abandon_retires_the_session(
    session: render.RenderSessionHandle,
    map_handle: mln.MapHandle,
) -> None:
    """Abandonment retires a lost target without any graphics call."""
    result = session.abandon()

    assert isinstance(result, render.RenderAbandonResult)
    assert result.disposition in {
        render.RenderAbandonDisposition.CLEAN,
        render.RenderAbandonDisposition.QUARANTINED,
    }
    assert result.quarantined_resource_count >= 0
    assert session.snapshot().state == render.RenderSessionState.ABANDONED

    # An abandoned session renders nothing more, and reports that state rather
    # than reaching a target it no longer has.
    assert_invalid_state(lambda: session.request_frame(render.FrameDemand()))
    assert_invalid_state(lambda: session.resize(render.RenderTargetExtent(8, 8, 1.0)))

    # The session still owns CPU-side state until it is destroyed, and the map
    # retires once it is.
    session.close()
    assert session.closed
    map_handle.close().result(timeout=10)
    assert map_handle.closed


def close_session(session: render.RenderSessionHandle) -> None:
    """Detach graphics resources, then destroy CPU-side session state."""
    if session.closed:
        return
    finish_render_operation(session, session.detach())
    session.close()


def release_frame(
    frame: object,
    sync: render.GpuSync = _DEFAULT_GPU_SYNC,
) -> None:
    """Release an acquired texture slot."""
    frame.release(sync)


def render_until_update(
    runtime: mln.RuntimeHandle,
    session: render.RenderSessionHandle,
    *,
    iterations: int = 5000,
) -> None:
    event = wait_for_runtime_event(
        runtime,
        mln.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE,
        iterations=iterations,
    )
    assert event.event_type == mln.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE
    assert request_and_finish_frame(session).disposition == render.RenderResult.RENDERED


def render_until(
    runtime: mln.RuntimeHandle,
    session: render.RenderSessionHandle,
    condition: Callable[[], bool],
    description: str,
    *,
    iterations: int = 5000,
) -> None:
    """Render until `condition` holds, failing with `description`."""
    for _ in range(iterations):
        runtime.drain_events()
        request_and_finish_frame(session)
        if condition():
            return
        time.sleep(0.001)
    raise AssertionError(description)


def wait_for_rendered_layer_feature(
    runtime: mln.RuntimeHandle,
    map_handle: mln.MapHandle,
    session: render.RenderSessionHandle,
    layer_id: str,
    *,
    iterations: int = 5000,
) -> query.QueriedFeature:
    """Render until one feature of `layer_id` covers the map center."""
    query_point = map_handle.pixel_for_lat_lng(geo.LatLng(0.0, 0.0)).result(timeout=5)
    geometry = query.RenderedQueryGeometry.box_geometry(
        query.ScreenBox(
            camera.ScreenPoint(query_point.x - 30.0, query_point.y - 30.0),
            camera.ScreenPoint(query_point.x + 30.0, query_point.y + 30.0),
        )
    )
    options = query.RenderedFeatureQueryOptions(layer_ids=(layer_id,))
    for _ in range(iterations):
        runtime.drain_events()
        try:
            result = request_and_finish_frame(session)
            if result.disposition != render.RenderResult.RENDERED:
                continue
            features = finish_render_operation(
                session, session.query_rendered_features(geometry, options)
            )
        except mln.InvalidStateError:
            features = []
        if features:
            return features[0]
        time.sleep(0.001)
    raise AssertionError(f"rendered feature query for {layer_id} returned no features")


def wait_for_rendered_cluster(
    runtime: mln.RuntimeHandle,
    map_handle: mln.MapHandle,
    session: render.RenderSessionHandle,
    *,
    iterations: int = 5000,
) -> query.QueriedFeature:
    """Load the cluster style and return the first rendered cluster feature."""
    map_handle.jump_to(
        camera.CameraOptions(center=geo.LatLng(0.0, 0.0), zoom=0.0)
    ).result(timeout=5)
    map_handle.set_style_json(CLUSTER_STYLE_JSON.encode()).result(timeout=5)
    return wait_for_rendered_layer_feature(
        runtime,
        map_handle,
        session,
        "cluster-circle",
        iterations=iterations,
    )


def feature_member(feature: dict[str, object], key: str) -> object:
    return feature["properties"][key]  # type: ignore[index]


def single_cluster_leaf(
    session: render.RenderSessionHandle,
    feature: bytes,
    *,
    offset: int,
) -> dict[str, object]:
    """Return the one leaf at `offset` through a bounded supercluster query."""
    leaves = finish_render_operation(
        session,
        session.query_feature_extensions(
            "cluster-source",
            feature,
            "supercluster",
            "leaves",
            json.dumps({"limit": 1, "offset": offset}, separators=(",", ":")).encode(),
        ),
    )
    collection = json.loads(leaves)
    assert len(collection["features"]) == 1
    return collection["features"][0]


def assert_geojson_cluster_source(
    runtime: mln.RuntimeHandle,
    map_handle: mln.MapHandle,
    session: render.RenderSessionHandle,
) -> None:
    """Cluster nearby points added through the GeoJSON source data API."""
    map_handle.jump_to(
        camera.CameraOptions(center=geo.LatLng(0.0, 0.0), zoom=0.0)
    ).result(timeout=5)
    map_handle.set_style_json(EMPTY_STYLE_JSON.encode()).result(timeout=5)
    with style.GeoJsonSourceDataHandle(
        CLUSTER_POINTS,
        style.GeoJsonSourceOptions(
            cluster=True,
            cluster_radius=60,
            cluster_min_points=2,
            cluster_max_zoom=20.0,
            # ["+", <map expression>] accumulates the mapped value per cluster.
            cluster_properties=b'{"weight_sum":["+",["get","weight"]]}',
        ),
    ) as cluster_data:
        map_handle.add_geojson_source_data("typed-cluster-source", cluster_data).result(
            timeout=5
        )
    map_handle.add_style_layer_json(
        b'{"id":"typed-cluster-circle","type":"circle",'
        b'"source":"typed-cluster-source","filter":["has","point_count"],'
        b'"paint":{"circle-color":"#2563eb","circle-radius":20}}'
    ).result(timeout=5)

    queried = wait_for_rendered_layer_feature(
        runtime, map_handle, session, "typed-cluster-circle"
    )
    # The three source points only collapse into one feature when the options
    # reach MapLibre Native, and weight_sum only appears with cluster
    # properties.
    feature = json.loads(queried.feature)
    assert isinstance(feature, dict)
    assert feature_member(feature, "cluster") is True
    assert feature_member(feature, "point_count") == 3
    assert feature_member(feature, "weight_sum") == 6


def assert_cluster_feature_extensions(
    runtime: mln.RuntimeHandle,
    map_handle: mln.MapHandle,
    session: render.RenderSessionHandle,
) -> None:
    """Round-trip a rendered cluster feature through supercluster queries."""
    cluster = wait_for_rendered_cluster(runtime, map_handle, session)
    # Native matches cluster_id by exact JSON value type, so the copied feature
    # must keep the unsigned alternative to resolve on the way back in.
    feature = json.loads(cluster.feature)
    assert isinstance(feature, dict)
    assert isinstance(feature_member(feature, "cluster_id"), int)

    children = finish_render_operation(
        session,
        session.query_feature_extensions(
            "cluster-source", cluster.feature, "supercluster", "children", None
        ),
    )
    assert json.loads(children)["features"]

    expansion_zoom = finish_render_operation(
        session,
        session.query_feature_extensions(
            "cluster-source", cluster.feature, "supercluster", "expansion-zoom", None
        ),
    )
    assert isinstance(json.loads(expansion_zoom), int)

    # Native ignores limit and offset arguments of another type and falls back
    # to ten leaves at offset zero, so both must move the observed result.
    first = single_cluster_leaf(session, cluster.feature, offset=0)
    second = single_cluster_leaf(session, cluster.feature, offset=1)
    assert feature_member(first, "name") != feature_member(second, "name")

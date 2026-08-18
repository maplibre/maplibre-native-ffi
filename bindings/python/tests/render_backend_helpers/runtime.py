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
        time.sleep(0.001)
        for event in runtime.drain_events().events:
            if event.event_type == event_type:
                return event
        time.sleep(0.001)
    raise AssertionError(f"runtime event {event_type!r} was not observed")


def finish_attach(
    session: render.RenderSessionHandle,
    operation: Future[None],
    *,
    iterations: int = 5000,
) -> None:
    """Complete attachment through its selected native driver."""
    if session.snapshot.driver == render.RenderDriver.CALLER_GRAPHICS_THREAD:
        for _ in range(iterations):
            session.service_driver_work(16)
            if operation.done():
                break
            time.sleep(0.001)
    else:
        operation.result(timeout=5)
        return
    operation.result(timeout=0)


def request_and_finish_frame(
    session: render.RenderSessionHandle,
    *,
    token: int = 1,
    iterations: int = 5000,
) -> render.RenderFrameResult:
    """Request one frame and return its owned terminal result."""
    session.request_frame(render.FrameDemand(token=token))
    for _ in range(iterations):
        if session.snapshot.driver == render.RenderDriver.CALLER_GRAPHICS_THREAD:
            session.service_driver_work(16)
        try:
            results = session.drain_frame_results()
        except mln.NotReadyError:
            results = []
        if results:
            return results[-1]
        time.sleep(0.001)
    raise AssertionError("frame demand did not produce a terminal result")


def finish_render_operation(
    session: render.RenderSessionHandle,
    operation: Future[object],
    *,
    return_result: bool = False,
    iterations: int = 5000,
) -> object:
    """Complete renderer-affine work through either driver."""
    for _ in range(iterations):
        if session.snapshot.driver == render.RenderDriver.CALLER_GRAPHICS_THREAD:
            session.service_driver_work(16)
        if operation.done():
            result = operation.result(timeout=0)
            return result if return_result else None
        time.sleep(0.001)
    raise AssertionError("render operation did not complete")


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
        time.sleep(0.001)
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
    query_point = map_handle.pixel_for_lat_lng(geo.LatLng(0.0, 0.0))
    geometry = query.RenderedQueryGeometry.box_geometry(
        query.ScreenBox(
            camera.ScreenPoint(query_point.x - 30.0, query_point.y - 30.0),
            camera.ScreenPoint(query_point.x + 30.0, query_point.y + 30.0),
        )
    )
    options = query.RenderedFeatureQueryOptions(layer_ids=(layer_id,))
    operation = map_handle.request_still_image()
    for _ in range(iterations):
        # Native execution advances independently; rendering remains host-driven.
        time.sleep(0.001)
        runtime.drain_events()
        try:
            result = request_and_finish_frame(session)
            if result.disposition != render.RenderResult.RENDERED:
                continue
            features = finish_render_operation(
                session,
                session.query_rendered_features(geometry, options),
                return_result=True,
            )
        except mln.InvalidStateError:
            features = []
        assert isinstance(features, list)
        if features:
            operation.close()
            first = features[0]
            assert isinstance(first, query.QueriedFeature)
            return first
        time.sleep(0.001)
    operation.close()
    raise AssertionError(f"rendered feature query for {layer_id} returned no features")


def wait_for_rendered_cluster(
    runtime: mln.RuntimeHandle,
    map_handle: mln.MapHandle,
    session: render.RenderSessionHandle,
    *,
    iterations: int = 5000,
) -> query.QueriedFeature:
    """Load the cluster style and return the first rendered cluster feature."""
    map_handle.jump_to(camera.CameraOptions(center=geo.LatLng(0.0, 0.0), zoom=0.0))
    map_handle.set_style_json(CLUSTER_STYLE_JSON.encode())
    runtime.barrier().result(timeout=5)
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
        return_result=True,
    )
    assert isinstance(leaves, bytes)
    collection = json.loads(leaves)
    assert len(collection["features"]) == 1
    return collection["features"][0]


def assert_geojson_cluster_source(
    runtime: mln.RuntimeHandle,
    map_handle: mln.MapHandle,
    session: render.RenderSessionHandle,
) -> None:
    """Cluster nearby points added through the GeoJSON source data API."""
    map_handle.jump_to(camera.CameraOptions(center=geo.LatLng(0.0, 0.0), zoom=0.0))
    map_handle.set_style_json(EMPTY_STYLE_JSON.encode())
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
        map_handle.add_geojson_source_data("typed-cluster-source", cluster_data)
    map_handle.add_style_layer_json(
        b'{"id":"typed-cluster-circle","type":"circle",'
        b'"source":"typed-cluster-source","filter":["has","point_count"],'
        b'"paint":{"circle-color":"#2563eb","circle-radius":20}}'
    )
    runtime.barrier().result(timeout=5)

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
        return_result=True,
    )
    assert isinstance(children, bytes)
    assert json.loads(children)["features"]

    expansion_zoom = finish_render_operation(
        session,
        session.query_feature_extensions(
            "cluster-source", cluster.feature, "supercluster", "expansion-zoom", None
        ),
        return_result=True,
    )
    assert isinstance(expansion_zoom, bytes)
    assert isinstance(json.loads(expansion_zoom), int)

    # Native ignores limit and offset arguments of another type and falls back
    # to ten leaves at offset zero, so both must move the observed result.
    first = single_cluster_leaf(session, cluster.feature, offset=0)
    second = single_cluster_leaf(session, cluster.feature, offset=1)
    assert feature_member(first, "name") != feature_member(second, "name")

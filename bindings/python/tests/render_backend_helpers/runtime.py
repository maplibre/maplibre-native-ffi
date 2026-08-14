from __future__ import annotations

import json
import time
from collections.abc import Callable

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import camera, geo, query, render, style

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
        runtime.pump()
        for event in runtime.drain_events().events:
            if event.event_type == event_type:
                return event
        time.sleep(0.001)
    raise AssertionError(f"runtime event {event_type!r} was not observed")


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
    assert session.render_update().result == render.RenderResult.RENDERED


def render_until(
    runtime: mln.RuntimeHandle,
    session: render.RenderSessionHandle,
    condition: Callable[[], bool],
    description: str,
    *,
    iterations: int = 5000,
) -> None:
    """Pump and render until `condition` holds, failing with `description`."""
    for _ in range(iterations):
        runtime.pump()
        for event in runtime.drain_events().events:
            if event.event_type == mln.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE:
                session.render_update()
        if condition():
            return
        time.sleep(0.001)
    raise AssertionError(description)


def request_still_image_if_needed(map_handle: mln.MapHandle) -> None:
    try:
        map_handle.request_still_image()
    except mln.InvalidStateError as error:
        if "pending still-image request" not in error.diagnostic:
            raise


def wait_for_rendered_layer_feature(
    runtime: mln.RuntimeHandle,
    map_handle: mln.MapHandle,
    session: render.RenderSessionHandle,
    layer_id: str,
    *,
    iterations: int = 5000,
) -> dict[str, object]:
    """Render until one feature of `layer_id` covers the map center."""
    query_point = map_handle.pixel_for_lat_lng(geo.LatLng(0.0, 0.0))
    geometry = query.RenderedQueryGeometry.box_geometry(
        query.ScreenBox(
            camera.ScreenPoint(query_point.x - 30.0, query_point.y - 30.0),
            camera.ScreenPoint(query_point.x + 30.0, query_point.y + 30.0),
        )
    )
    options = query.RenderedFeatureQueryOptions(layer_ids=(layer_id,))
    for _ in range(iterations):
        request_still_image_if_needed(map_handle)
        runtime.pump()
        for event in runtime.drain_events().events:
            if event.event_type == mln.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE:
                try:
                    session.render_update()
                except mln.InvalidStateError:
                    pass
        try:
            features = json.loads(session.query_rendered_features(geometry, options))
        except mln.InvalidStateError:
            features = ()
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
) -> dict[str, object]:
    """Load the cluster style and return the first rendered cluster feature."""
    map_handle.jump_to(camera.CameraOptions(center=geo.LatLng(0.0, 0.0), zoom=0.0))
    map_handle.set_style_json(CLUSTER_STYLE_JSON.encode())
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
    feature: dict[str, object],
    *,
    offset: int,
) -> dict[str, object]:
    """Return the one leaf at `offset` through a bounded supercluster query."""
    leaves = session.query_feature_extensions(
        "cluster-source",
        json.dumps(feature, separators=(",", ":")).encode(),
        "supercluster",
        "leaves",
        json.dumps({"limit": 1, "offset": offset}, separators=(",", ":")).encode(),
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

    queried = wait_for_rendered_layer_feature(
        runtime, map_handle, session, "typed-cluster-circle"
    )
    # The three source points only collapse into one feature when the options
    # reach MapLibre Native, and weight_sum only appears with cluster
    # properties.
    feature = queried["feature"]
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
    feature = cluster["feature"]
    assert isinstance(feature, dict)
    assert isinstance(feature_member(feature, "cluster_id"), int)
    feature_bytes = json.dumps(feature, separators=(",", ":")).encode()

    children = session.query_feature_extensions(
        "cluster-source", feature_bytes, "supercluster", "children", None
    )
    assert json.loads(children)["features"]

    expansion_zoom = session.query_feature_extensions(
        "cluster-source", feature_bytes, "supercluster", "expansion-zoom", None
    )
    assert isinstance(json.loads(expansion_zoom), int)

    # Native ignores limit and offset arguments of another type and falls back
    # to ten leaves at offset zero, so both must move the observed result.
    first = single_cluster_leaf(session, feature, offset=0)
    second = single_cluster_leaf(session, feature, offset=1)
    assert feature_member(first, "name") != feature_member(second, "name")

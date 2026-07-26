from __future__ import annotations

import time

import pytest

import maplibre_native as mln
from maplibre_native import camera, geo, json, query, render

EMPTY_STYLE_JSON = '{"version":8,"sources":{},"layers":[]}'

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
        runtime.run_once()
        while event := runtime.poll_event():
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
    assert session.render_update()


def request_still_image_if_needed(map_handle: mln.MapHandle) -> None:
    try:
        map_handle.request_still_image()
    except mln.InvalidStateError as error:
        if "pending still-image request" not in error.diagnostic:
            raise


def wait_for_rendered_cluster(
    runtime: mln.RuntimeHandle,
    map_handle: mln.MapHandle,
    session: render.RenderSessionHandle,
    *,
    iterations: int = 5000,
) -> query.QueriedFeature:
    """Load the cluster style and return the first rendered cluster feature."""
    map_handle.jump_to(camera.CameraOptions(center=geo.LatLng(0.0, 0.0), zoom=0.0))
    map_handle.set_style_json(CLUSTER_STYLE_JSON)
    query_point = map_handle.pixel_for_lat_lng(geo.LatLng(0.0, 0.0))
    geometry = query.RenderedQueryGeometry.box_geometry(
        query.ScreenBox(
            camera.ScreenPoint(query_point.x - 30.0, query_point.y - 30.0),
            camera.ScreenPoint(query_point.x + 30.0, query_point.y + 30.0),
        )
    )
    options = query.RenderedFeatureQueryOptions(layer_ids=("cluster-circle",))
    for _ in range(iterations):
        request_still_image_if_needed(map_handle)
        runtime.run_once()
        while event := runtime.poll_event():
            if event.event_type == mln.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE:
                try:
                    session.render_update()
                except mln.InvalidStateError:
                    pass
        try:
            features = session.query_rendered_features(geometry, options)
        except mln.InvalidStateError:
            features = ()
        if features:
            return features[0]
        time.sleep(0.001)
    raise AssertionError("cluster feature query returned no features")


def assert_cluster_feature_extensions(
    runtime: mln.RuntimeHandle,
    map_handle: mln.MapHandle,
    session: render.RenderSessionHandle,
) -> None:
    """Round-trip a rendered cluster feature through supercluster queries."""
    cluster = wait_for_rendered_cluster(runtime, map_handle, session)
    # Native matches cluster_id by exact JSON value type, so the copied feature
    # must keep the unsigned alternative to resolve on the way back in.
    cluster_id = next(
        member.value
        for member in cluster.feature.properties
        if member.key == "cluster_id"
    )
    assert isinstance(cluster_id, json.JsonUInt)

    children = session.query_feature_extensions(
        "cluster-source", cluster.feature, "supercluster", "children", None
    )
    assert children.type == query.FeatureExtensionResultType.FEATURE_COLLECTION
    assert children.feature_collection

    expansion_zoom = session.query_feature_extensions(
        "cluster-source", cluster.feature, "supercluster", "expansion-zoom", None
    )
    assert expansion_zoom.type == query.FeatureExtensionResultType.VALUE
    assert isinstance(expansion_zoom.value, json.JsonUInt)

    leaves = session.query_feature_extensions(
        "cluster-source",
        cluster.feature,
        "supercluster",
        "leaves",
        json.JsonObject(
            (
                json.JsonMember("limit", json.JsonUInt(1)),
                json.JsonMember("offset", json.JsonUInt(0)),
            )
        ),
    )
    assert leaves.type == query.FeatureExtensionResultType.FEATURE_COLLECTION
    assert leaves.feature_collection is not None
    assert len(leaves.feature_collection) == 1

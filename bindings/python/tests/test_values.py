"""The generated value shapes, one representative each against native."""

import json
import typing
from dataclasses import replace

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import api
from support import EMPTY_STYLE, Harness, ok_response, result


def _jump(map_handle: mln.MapHandle, camera: mln.CameraOptions) -> None:
    update = replace(
        mln.CameraUpdate.default(), mode=mln.CameraUpdateMode.JUMP, camera=camera
    )
    result(map_handle.update_camera(update))


def test_host_strings_use_views_and_native_lent_strings_decode(
    harness: Harness,
    map_handle: mln.MapHandle,
) -> None:
    seen: list[str | None] = []

    def serve(
        request: mln.ResourceRequestInfo, handle: mln.ResourceRequestHandle
    ) -> None:
        seen.append(request.resolved_url)
        handle.complete(ok_response())

    harness.routes["custom://native-lent.json"] = serve
    map_handle.set_style_url("custom://native-lent.json")
    harness.wait_event(mln.RuntimeEventType.MAP_STYLE_LOADED)
    assert seen == ["custom://native-lent.json"]
    result(map_handle.set_style_json(EMPTY_STYLE))
    layer_id = "café-层"
    layer = json.dumps({"id": layer_id, "type": "background"}, ensure_ascii=False)

    # The layer JSON crosses as sized UTF-8 bytes, and the ID comes back
    # through a lookup that also takes an explicit-length view.
    result(map_handle.add_style_layer_json(layer.encode()))
    assert [entry.id for entry in result(map_handle.list_style_layers())] == [layer_id]
    stored = result(map_handle.get_style_layer_json(layer_id))
    assert stored is not None
    assert json.loads(stored)["id"] == layer_id

    with pytest.raises(mln.MaplibreError) as raised:
        map_handle.set_style_url("custom://bad\0url")
    assert raised.value.status == mln.Status.INVALID_ARGUMENT
    assert "embedded NUL" in raised.value.diagnostic
    with pytest.raises(TypeError, match="instance of 'bytes'"):
        map_handle.add_style_layer_json(typing.cast(typing.Any, layer))


def test_absent_fields_stay_distinct_from_present_values(
    map_handle: mln.MapHandle,
) -> None:
    _jump(map_handle, mln.CameraOptions(center=mln.LatLng(10.0, 20.0), zoom=3.0))
    # Fields left as None are absent, so this update changes the zoom alone.
    _jump(map_handle, mln.CameraOptions(zoom=5.0, bearing=0.0))

    camera = result(map_handle.get_camera()).camera
    assert camera.center is not None
    assert camera.center.latitude == pytest.approx(10.0)
    assert camera.center.longitude == pytest.approx(20.0)
    assert camera.zoom == pytest.approx(5.0)
    assert camera.bearing == 0.0

    result(
        map_handle.set_style_json(
            b'{"version":8,"sources":{"points":{"type":"geojson","data":'
            b'{"type":"FeatureCollection","features":[]}}},"layers":'
            b'[{"id":"circles","type":"circle","source":"points"}]}'
        )
    )
    # An absent optional output comes back as None.
    assert result(map_handle.get_style_layer_property("circles", "filter")) is None


def test_a_strided_batch_decodes_and_an_unknown_union_arm_reaches_native(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    # Loading an inline style queues its events, with the generation that the
    # command reports, before the command completes.
    loaded = result(map_handle.set_style_json(EMPTY_STYLE))

    # The copied records outlive the native callback that borrowed them.
    events = harness._pending
    types = [event.type for event in events]
    assert mln.RuntimeEventType.MAP_STYLE_LOADED in types
    assert len(types) >= 2
    assert {event.source for event in events} == {map_handle.id}
    assert events[-1].generation == loaded.generation

    unknown = mln.OfflineRegionDefinition(mln.UnknownVariant(999))
    with pytest.raises(mln.MaplibreError) as raised:
        harness.runtime.create_offline_region(unknown, b"")
    # Native saw the arm's tag and refused it; the binding passed it through.
    assert raised.value.status == mln.Status.INVALID_ARGUMENT
    assert "definition type is invalid" in raised.value.diagnostic


def test_integers_are_range_checked_enums_stay_open_and_64_bit_values_round_trip(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    for out_of_range in (-1, 2**64):
        with pytest.raises(OverflowError):
            harness.runtime.set_maximum_ambient_cache_size(out_of_range)

    unknown = mln.RenderResult(777)
    assert unknown.is_unknown
    assert int(unknown) == 777

    # Cancelling the largest identity ends the ease that carries it, so the
    # identity crossed whole both times.
    largest = 2**64 - 1
    ends: list[mln.CameraTransitionEnd] = []
    ease = replace(
        mln.CameraUpdate.default(),
        mode=mln.CameraUpdateMode.EASE,
        camera=mln.CameraOptions(zoom=2.0),
        animation=mln.AnimationOptions(
            duration_ms=60000.0,
            transition_id=largest,
            end_handler=mln.CameraTransitionHandler(ends.append),
        ),
    )
    result(map_handle.update_camera(ease))
    result(map_handle.cancel_camera_transition(largest))
    assert [end.outcome for end in ends] == [mln.CameraTransitionOutcome.CANCELLED]


def test_public_type_hints_resolve() -> None:
    for name in api.__all__:
        target = getattr(api, name)
        if isinstance(target, type):
            typing.get_type_hints(target)
            for member in vars(target).values():
                if callable(member) and hasattr(member, "__annotations__"):
                    typing.get_type_hints(member)
        elif callable(target):
            typing.get_type_hints(target)


def test_synchronous_calls_fill_caller_sized_outputs_in_input_order(
    map_handle: mln.MapHandle,
) -> None:
    projection = mln.map_projection_create(map_handle.get_snapshot().view)
    try:
        coordinates = (mln.LatLng(0, 0), mln.LatLng(10, 20), mln.LatLng(-12, -30))
        points = projection.pixels_for_lat_lngs(coordinates)
        assert points == tuple(
            projection.pixel_for_lat_lng(value) for value in coordinates
        )
        assert projection.lat_lngs_for_pixels(points) == tuple(
            projection.lat_lng_for_pixel(point) for point in points
        )
        assert projection.lat_lngs_for_pixels_unwrapped(points) == tuple(
            projection.lat_lng_for_pixel_unwrapped(point) for point in points
        )
        assert projection.pixels_for_lat_lngs(()) == ()
        assert projection.lat_lngs_for_pixels(()) == ()
        assert projection.lat_lngs_for_pixels_unwrapped(()) == ()
    finally:
        projection.close()


def test_caller_sized_text_returns_results_longer_than_its_first_storage() -> None:
    zoom = mln.style_expression_create(
        b'["interpolate", ["linear"], ["zoom"], 0, 0, 10, 100]'
    )
    try:
        assert zoom.evaluate(mln.StyleExpressionInputs(zoom=5.0)) == b"50.0"
    finally:
        zoom.close()

    # The result outgrows the runtime's first storage, so the call runs again.
    text = b'"' + b"a" * 1000 + b'"'
    long = mln.style_expression_create(text)
    try:
        assert long.evaluate() == text
    finally:
        long.close()

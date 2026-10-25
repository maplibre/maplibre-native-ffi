"""The generated value shapes, one representative each against native."""

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
    seen: list[str] = []

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

    # The ID comes back through a lookup that also takes an explicit-length
    # view.
    result(map_handle.add_style_layer_json({"id": layer_id, "type": "background"}, ""))
    assert [entry.id for entry in result(map_handle.list_style_layers())] == [layer_id]

    with pytest.raises(mln.MaplibreError) as raised:
        map_handle.set_style_url("custom://bad\0url")
    assert raised.value.status == mln.Status.INVALID_ARGUMENT
    assert "embedded NUL" in raised.value.diagnostic


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

    collection = {"type": "FeatureCollection", "features": []}
    result(
        map_handle.set_style_json(
            {
                "version": 8,
                "sources": {"points": {"type": "geojson", "data": collection}},
                "layers": [{"id": "circles", "type": "circle", "source": "points"}],
            }
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


def test_json_crosses_as_json_dumps_and_json_loads_values(
    map_handle: mln.MapHandle,
) -> None:
    result(
        map_handle.set_style_json(
            {
                "version": 8,
                "sources": {},
                "layers": [{"id": "fill", "type": "background"}],
            }
        )
    )
    # A value goes in as json.dumps writes it and comes back as json.loads
    # reads it, so a style-spec string needs no inner quotes.
    result(map_handle.set_global_state_property("mode", "dark"))
    assert result(map_handle.get_global_state()) == {"mode": "dark"}
    result(map_handle.set_style_layer_property("fill", "background-opacity", 0.5))
    opacity = result(map_handle.get_style_layer_property("fill", "background-opacity"))
    assert opacity == 0.5
    # A parameter that is None is JSON null, which restores the default, and a
    # property without a value comes back absent as None.
    result(map_handle.set_style_layer_property("fill", "background-opacity", None))
    assert (
        result(map_handle.get_style_layer_property("fill", "background-opacity"))
        is None
    )

    # A record member that is None is absent, an empty view.
    expression = mln.style_expression_create(["get", "name"])
    try:
        feature = {
            "type": "Feature",
            "properties": {"name": "a"},
            "geometry": {"type": "Point", "coordinates": [0, 0]},
        }
        inputs = mln.StyleExpressionInputs(feature=feature, feature_state=None)
        assert expression.evaluate(inputs) == "a"
    finally:
        expression.close()

    # A value that json.dumps rejects raises its error before native runs.
    with pytest.raises(TypeError, match="not JSON serializable"):
        mln.style_expression_create(object())


def test_caller_sized_text_returns_results_longer_than_its_first_storage() -> None:
    zoom = mln.style_expression_create(
        ["interpolate", ["linear"], ["zoom"], 0, 0, 10, 100]
    )
    try:
        assert zoom.evaluate(mln.StyleExpressionInputs(zoom=5.0)) == 50.0
    finally:
        zoom.close()

    # The result outgrows the runtime's first storage, so the call runs again.
    text = "a" * 1000
    long = mln.style_expression_create(text)
    try:
        assert long.evaluate() == text
    finally:
        long.close()


def test_inputs_take_any_sequence_or_byte_buffer_and_require_their_subject(
    map_handle: mln.MapHandle,
) -> None:
    # An input record keeps a tuple and bytes, so it stays hashable.
    options = mln.RenderedFeatureQueryOptions(layer_ids=["roads"])
    assert options.layer_ids == ("roads",)
    hash(options)
    image = mln.PremultipliedRgba8Image(
        width=1, height=1, stride=4, pixels=bytearray(b"\xff\x00\x00\xff")
    )
    assert image.pixels == b"\xff\x00\x00\xff"
    hash(image)
    with pytest.raises(TypeError, match="not str"):
        mln.RenderedFeatureQueryOptions(layer_ids=typing.cast(typing.Any, "roads"))

    result(map_handle.set_style_json(EMPTY_STYLE))
    result(
        map_handle.add_custom_mvt_vector_source(
            "custom", mln.CustomMvtVectorSourceOptions(fetch_tile=lambda *_: None)
        )
    )
    tile = mln.CanonicalTileId(z=0, x=0, y=0)
    # A parameter copies a buffer that is not bytes before native reads it.
    result(
        map_handle.set_custom_mvt_vector_source_tile_data(
            "custom", tile, memoryview(bytearray(b"\x1a\x00"))
        )
    )
    with pytest.raises(TypeError, match="not str"):
        map_handle.camera_for_lat_lngs(typing.cast(typing.Any, "coordinates"))
    # The extent is the subject of a resize, which has no default.
    with pytest.raises(TypeError):
        typing.cast(typing.Any, map_handle).resize()

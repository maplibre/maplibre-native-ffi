"""The generated value shapes, one representative each against native."""

import json
import typing
from dataclasses import replace

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import api
from support import EMPTY_STYLE, Harness, result


def _jump(map_handle: mln.MapHandle, camera: mln.CameraOptions) -> None:
    update = replace(
        mln.CameraUpdate.default(), mode=mln.CameraUpdateMode.JUMP, camera=camera
    )
    assert result(map_handle.update_camera(update)).disposition == (
        mln.CommandDisposition.COMMITTED
    )


def test_strings_cross_as_c_strings_and_as_sized_views(
    map_handle: mln.MapHandle,
) -> None:
    result(map_handle.set_style_json(EMPTY_STYLE))
    layer_id = "café-层"
    layer = json.dumps({"id": layer_id, "type": "background"}, ensure_ascii=False)

    # The layer JSON crosses as sized UTF-8 bytes, and the ID comes back
    # through a lookup that takes it as a NUL-terminated string.
    result(map_handle.add_style_layer_json(layer.encode()))
    assert [entry.id for entry in result(map_handle.list_style_layers())] == [layer_id]
    stored = result(map_handle.get_style_layer_json(layer_id))
    assert stored is not None
    assert json.loads(stored)["id"] == layer_id

    with pytest.raises(mln.InvalidArgumentError) as raised:
        map_handle.set_style_url("custom://bad\0url")
    assert raised.value.native_status_code is None
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
    assert result(map_handle.get_style_layer_filter("circles")) is None
    result(
        map_handle.set_style_layer_filter("circles", b'["==",["get","kind"],"park"]')
    )
    assert json.loads(result(map_handle.get_style_layer_filter("circles"))) == [
        "==",
        ["get", "kind"],
        "park",
    ]
    result(map_handle.set_style_layer_filter("circles", None))
    assert result(map_handle.get_style_layer_filter("circles")) is None


def test_a_strided_batch_decodes_and_an_unknown_union_arm_reaches_native(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    # Loading an inline style queues its events, with the generation that the
    # command reports, before the command completes.
    loaded = result(map_handle.set_style_json(EMPTY_STYLE))

    batch = harness.runtime.drain_events()
    assert batch is not None
    with batch:
        view = batch.get()
    # The copied records outlive the batch that held them.
    types = [event.type for event in view.events]
    assert mln.RuntimeEventType.MAP_STYLE_LOADED in types
    assert len(types) >= 2
    assert {event.source for event in view.events} == {map_handle.id}
    assert view.events[-1].generation == loaded.generation

    unknown = mln.OfflineRegionDefinition(mln.UnknownVariant(999))
    with pytest.raises(mln.InvalidArgumentError) as raised:
        harness.runtime.create_offline_region(unknown, b"")
    # Native saw the arm's tag and refused it; the binding passed it through.
    assert raised.value.native_status_code == mln.Status.INVALID_ARGUMENT.native_code
    assert "definition type is invalid" in raised.value.diagnostic


def test_integers_are_range_checked_enums_stay_open_and_64_bit_values_round_trip(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    for out_of_range in (-1, 2**64):
        with pytest.raises(OverflowError):
            harness.runtime.set_maximum_ambient_cache_size(out_of_range)

    unknown = mln.RenderResult(777)
    assert unknown.is_unknown
    assert unknown.native_code == 777

    largest = 2**64 - 1
    ease = replace(
        mln.CameraUpdate.default(),
        mode=mln.CameraUpdateMode.EASE,
        camera=mln.CameraOptions(zoom=2.0),
        animation=mln.AnimationOptions(duration_ms=0.0, transition_id=largest),
    )
    result(map_handle.update_camera(ease))
    finished = harness.wait_event(mln.RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED)
    assert isinstance(finished.payload, mln.RuntimeEventCameraTransitionFinishedVariant)
    assert finished.payload.value.transition_id == largest


def test_an_array_input_is_copied_when_it_is_submitted(
    map_handle: mln.MapHandle,
) -> None:
    result(map_handle.set_style_json(EMPTY_STYLE))
    tiles = ["custom://tiles/{z}/{x}/{y}.pbf"]

    added = map_handle.add_vector_source_tiles("vector", typing.cast(typing.Any, tiles))
    tiles[0] = "custom://replaced/{z}/{x}/{y}.pbf"
    tiles.append("custom://appended/{z}/{x}/{y}.pbf")

    assert result(added).disposition == mln.CommandDisposition.COMMITTED
    source = result(map_handle.get_style_source("vector"))
    assert source is not None
    assert source.tilejson is not None
    assert source.tilejson.tile_urls == ("custom://tiles/{z}/{x}/{y}.pbf",)


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

    assert typing.get_type_hints(mln.MapHandle.set_style_json)["json"] is bytes
    assert typing.get_args(
        typing.get_type_hints(mln.RuntimeHandle.create_map)["return"]
    ) == (mln.MapHandle,)

# Snippets for the docs site's Style page. They are type-checked with the
# tests, which never run them.

import maplibre_native_ffi as mln


def draw_image(image_id: str) -> mln.PremultipliedRgba8Image:
    return mln.PremultipliedRgba8Image(
        width=1, height=1, stride=4, pixels=bytes([255, 0, 0, 255])
    )


# pixels holds 16 x 16 premultiplied RGBA pixels.
def style(map_handle: mln.MapHandle, pixels: bytes) -> None:
    # #region load
    map_handle.discarding.set_style_url("https://demotiles.maplibre.org/style.json")
    # #endregion load

    # #region add
    map_handle.discarding.add_style_source_json(
        "satellite",
        {
            "type": "raster",
            "tileSize": 256,
            "tiles": ["https://tiles.example.com/{z}/{x}/{y}.png"],
        },
    )

    # Draw the layer under the "countries-label" layer.
    map_handle.discarding.add_style_layer_json(
        {"id": "satellite", "type": "raster", "source": "satellite"},
        "countries-label",
    )
    # #endregion add

    # #region geojson
    # The source keeps its own reference to the data, so the block can close it.
    with mln.GeoJsonSourceDataHandle.create(
        {"type": "Point", "coordinates": [-122.4, 37.8]}
    ) as data:
        map_handle.discarding.add_geo_json_source_data("points", data)

    # Later, prepare new data with the same options, and set it.
    with mln.GeoJsonSourceDataHandle.create(
        {"type": "Point", "coordinates": [2.35, 48.86]}
    ) as data:
        map_handle.discarding.set_geo_json_source_data("points", data)
    # #endregion geojson

    # #region set-property
    # Show both changes in the same frame.
    map_handle.discarding.begin_command_group()
    map_handle.discarding.set_style_layer_property("satellite", "visibility", "none")
    map_handle.discarding.set_style_layer_property(
        "countries-boundary",
        "line-color",
        ["interpolate", ["linear"], ["zoom"], 5, "#9cf", 12, "#06c"],
    )
    map_handle.discarding.end_command_group()
    # #endregion set-property

    # #region image
    map_handle.discarding.set_style_image(
        "marker",
        mln.PremultipliedRgba8Image(width=16, height=16, stride=16 * 4, pixels=pixels),
    )

    # An empty layer ID draws the layer on top.
    map_handle.discarding.add_style_layer_json(
        {
            "id": "markers",
            "type": "symbol",
            "source": "points",
            "layout": {"icon-image": "marker"},
        },
        "",
    )
    # #endregion image


def style_events(runtime: mln.RuntimeHandle, map_handle: mln.MapHandle) -> None:
    # #region missing-image
    # Runs on the binding's dispatcher thread.
    def on_events(events: tuple[mln.RuntimeEvent, ...]) -> None:
        for event in events:
            if (
                event.source != map_handle.id
                or event.type != mln.RuntimeEventType.MAP_STYLE_IMAGE_MISSING
            ):
                continue
            # The message holds the image's ID.
            map_handle.discarding.set_style_image(
                event.message, draw_image(event.message)
            )

    runtime.discarding.set_event_handler(mln.RuntimeEventHandler(on_events))
    # #endregion missing-image

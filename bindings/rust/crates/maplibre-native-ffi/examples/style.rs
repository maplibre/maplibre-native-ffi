//! Snippets for the docs site's Style page. They compile with the tests, which
//! never run them.

#![allow(dead_code)]

use std::sync::Arc;

use maplibre_native_ffi::*;

fn draw_image(_image_id: &str) -> PremultipliedRgba8Image<'static> {
    PremultipliedRgba8Image::new(1, 1, 4, &[255, 0, 0, 255])
}

// pixels holds 16 x 16 premultiplied RGBA pixels.
async fn style(map: &MapHandle, pixels: &[u8]) -> Result<()> {
    // #region load
    map.set_style_url("https://demotiles.maplibre.org/style.json")?
        .await?;
    // #endregion load

    // #region add
    map.discarding().add_style_source_json(
        "satellite",
        r#"{
            "type": "raster",
            "tileSize": 256,
            "tiles": ["https://tiles.example.com/{z}/{x}/{y}.png"]
        }"#,
    )?;

    // Draw the layer under the "countries-label" layer.
    map.discarding().add_style_layer_json(
        r#"{"id":"satellite","type":"raster","source":"satellite"}"#,
        Some("countries-label"),
    )?;
    // #endregion add

    // #region geojson
    map.discarding().add_geo_json_source("points", None, None)?;
    map.discarding().set_geo_json_source_data(
        "points",
        r#"{"type":"Point","coordinates":[-122.4,37.8]}"#,
    )?;
    // #endregion geojson

    // #region set-property
    // Show both changes in the same frame.
    map.discarding().begin_command_group()?;
    map.discarding().set_style_layer_property(
        "satellite",
        "visibility",
        r#""none""#,
    )?;
    map.discarding().set_style_layer_property(
        "countries-boundary",
        "line-color",
        r##"["interpolate",["linear"],["zoom"],5,"#9cf",12,"#06c"]"##,
    )?;
    map.discarding().end_command_group()?;
    // #endregion set-property

    // #region image
    let image = PremultipliedRgba8Image::new(16, 16, 16 * 4, pixels);
    map.discarding().set_style_image("marker", &image, None)?;

    // An empty layer ID draws the layer on top.
    map.discarding().add_style_layer_json(
        r#"{
            "id": "markers",
            "type": "symbol",
            "source": "points",
            "layout": {"icon-image": "marker"}
        }"#,
        None,
    )?;
    // #endregion image

    Ok(())
}

fn style_events(
    runtime: &RuntimeHandle,
    map: &Arc<MapHandle>,
) -> Result<()> {
    // #region missing-image
    // A strong reference would keep the map from dropping.
    let map = Arc::downgrade(map);

    // The handler runs on a library thread.
    runtime.discarding().set_event_handler(
        RuntimeEventHandler::new(move |events| {
            let Some(map) = map.upgrade() else {
                return;
            };
            for event in events {
                if event.source != map.id() {
                    continue;
                }
                let RuntimeEventPayload::StyleImage(missing) =
                    &event.payload
                else {
                    continue;
                };
                let image = draw_image(&missing.image_id);
                let _ = map.discarding().set_style_image(
                    &missing.image_id,
                    &image,
                    None,
                );
            }
        }),
    )?;
    // #endregion missing-image

    Ok(())
}

fn main() {}

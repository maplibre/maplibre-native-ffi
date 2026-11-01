//! Snippets for the docs site's Query page. They compile with the tests, which
//! never run them.

#![allow(dead_code)]

use maplibre_native_ffi::*;

fn use_feature(_json: &str) {}

async fn query(session: &RenderSessionHandle, point: ScreenPoint) -> Result<()> {
    // #region rendered
    let area = RenderedQueryGeometry::Box(ScreenBox {
        min: ScreenPoint::new(point.x - 5.0, point.y - 5.0),
        max: ScreenPoint::new(point.x + 5.0, point.y + 5.0),
    });
    let options = RenderedFeatureQueryOptions {
        layer_ids: Some(&["poi"]),
        ..Default::default()
    };

    let hits = session
        .query_rendered_features(&area, Some(&options))?
        .await?;
    for hit in hits {
        // hit.feature is the GeoJSON feature, as JSON.
        use_feature(&hit.feature);
    }
    // #endregion rendered

    // #region source
    let options = SourceFeatureQueryOptions {
        source_layer_ids: Some(&["transportation"]),
        // A filter from the MapLibre style spec, as JSON.
        filter: r#"["==", ["get", "class"], "motorway"]"#,
    };

    let features = session
        .query_source_features("openmaptiles", Some(&options))?
        .await?;
    for feature in features {
        use_feature(&feature.feature);
    }
    // #endregion source

    Ok(())
}

fn main() {}

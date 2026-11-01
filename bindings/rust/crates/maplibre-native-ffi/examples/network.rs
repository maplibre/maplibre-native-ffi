//! Snippets for the docs site's Network page. They compile with the tests,
//! which never run them.

#![allow(dead_code)]

use std::sync::Arc;

use maplibre_native_ffi::*;

fn with_api_key(url: &str) -> String {
    let separator = if url.contains('?') { '&' } else { '?' };
    format!("{url}{separator}key=my-api-key")
}

fn find_tile(_url: &str) -> Option<Vec<u8>> {
    None
}

fn geojson_for_tile(_tile: CanonicalTileId) -> String {
    r#"{"type":"FeatureCollection","features":[]}"#.to_owned()
}

fn network(runtime: &RuntimeHandle, map: &Arc<MapHandle>) -> Result<()> {
    // #region headers
    let rule = HttpHeaderRule {
        flags: UrlMatchFlag::GLOB,
        url: "https://tiles.example.com/**".to_owned(),
        headers: vec![HttpHeader {
            name: "Authorization".to_owned(),
            value: "Bearer my-token".to_owned(),
        }],
        ..Default::default()
    };

    runtime
        .discarding()
        .set_http_header_transform(HttpHeaderTransform {
            rules: vec![rule],
            ..Default::default()
        })?;
    // #endregion headers

    // #region rewrite
    runtime
        .discarding()
        .set_resource_transform(ResourceTransform::new(|_kind, url, request| {
            // Dropping the request without an answer keeps the old URL.
            let _ = request.complete(&with_api_key(url));
        }))?;
    // #endregion rewrite

    // #region provider
    let provider = ResourceProvider {
        routes: vec![ResourceRoute {
            kinds: ResourceKindFlag::TILE,
            flags: UrlMatchFlag::GLOB,
            url: "app://tiles/**".to_owned(),
        }],
        ..ResourceProvider::new(|request, handle| {
            let tile = find_tile(&request.requested_url);
            let response = match &tile {
                Some(bytes) => ResourceResponse {
                    status: ResourceResponseStatus::Ok,
                    bytes,
                    ..Default::default()
                },
                None => ResourceResponse {
                    status: ResourceResponseStatus::NoContent,
                    ..Default::default()
                },
            };
            // Dropping the request without an answer fails it.
            let _ = handle.complete(&response);
        })
    };

    runtime.discarding().set_resource_provider(provider)?;
    // #endregion provider

    // #region custom-source
    // A strong reference would keep the map from dropping.
    let source_map = Arc::downgrade(map);
    let options = CustomGeometrySourceOptions::new(move |tile| {
        if let Some(map) = source_map.upgrade() {
            let data = geojson_for_tile(tile);
            let _ = map
                .discarding()
                .set_custom_geometry_source_tile_data("app-data", tile, &data);
        }
    });

    map.discarding()
        .add_custom_geometry_source("app-data", options)?;
    // #endregion custom-source

    // #region status
    set_network_status(NetworkStatus::Offline)?;

    // Later, when the connection returns:
    set_network_status(NetworkStatus::Online)?;
    // #endregion status

    Ok(())
}

fn main() {}

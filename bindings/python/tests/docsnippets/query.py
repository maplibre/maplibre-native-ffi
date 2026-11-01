# Snippets for the docs site's Query page. They are type-checked with the
# tests, which never run them.

from typing import Any

import maplibre_native_ffi as mln


def use_feature(feature: Any) -> None:
    pass


def query(session: mln.RenderSessionHandle, point: mln.ScreenPoint) -> None:
    # #region rendered
    hits = session.query_rendered_features(
        mln.RenderedQueryGeometryBoxVariant(
            mln.ScreenBox(
                min=mln.ScreenPoint(x=point.x - 5, y=point.y - 5),
                max=mln.ScreenPoint(x=point.x + 5, y=point.y + 5),
            )
        ),
        mln.RenderedFeatureQueryOptions(layer_ids=["poi"]),
    ).result()
    for hit in hits:
        # hit.feature is the parsed GeoJSON feature.
        use_feature(hit.feature)
    # #endregion rendered

    # #region source
    features = session.query_source_features(
        "openmaptiles",
        mln.SourceFeatureQueryOptions(
            source_layer_ids=["transportation"],
            # A filter from the MapLibre style spec.
            filter=["==", ["get", "class"], "motorway"],
        ),
    ).result()
    for feature in features:
        use_feature(feature.feature)
    # #endregion source

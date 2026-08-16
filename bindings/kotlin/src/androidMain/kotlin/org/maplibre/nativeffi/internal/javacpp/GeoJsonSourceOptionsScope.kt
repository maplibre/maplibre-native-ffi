package org.maplibre.nativeffi.internal.javacpp

import org.maplibre.nativeffi.style.GeoJsonSourceOptions

/** Native mln_geojson_source_options storage for one C call's borrow window. */
internal class GeoJsonSourceOptionsScope(value: GeoJsonSourceOptions?) : AutoCloseable {
  private val clusterProperties: ByteArrayViewScope? =
    value?.clusterPropertiesTransit?.let(::ByteArrayViewScope)
  val options: MaplibreNativeC.mln_geojson_source_options =
    MaplibreNativeC.mln_geojson_source_options_default()

  init {
    var fields = 0
    value?.minZoom?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM
      options.min_zoom(it)
    }
    value?.maxZoom?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM
      options.max_zoom(it)
    }
    value?.tolerance?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_TOLERANCE
      options.tolerance(it)
    }
    value?.clusterMaxZoom?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM
      options.cluster_max_zoom(it)
    }
    clusterProperties?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES
      options.cluster_properties(it.view)
    }
    value?.tileSize?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE
      options.tile_size(it)
    }
    value?.buffer?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_BUFFER
      options.buffer(it)
    }
    value?.clusterRadius?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS
      options.cluster_radius(it)
    }
    value?.clusterMinPoints?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
      options.cluster_min_points(it)
    }
    value?.lineMetrics?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS
      options.line_metrics(it)
    }
    value?.cluster?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_CLUSTER
      options.cluster(it)
    }
    value?.synchronousTiling?.let {
      fields = fields or MaplibreNativeC.MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING
      options.synchronous_tiling(it)
    }
    options.fields(fields)
  }

  override fun close() {
    options.close()
    clusterProperties?.close()
  }
}

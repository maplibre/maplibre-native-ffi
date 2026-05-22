package org.maplibre.nativejni.internal.bridge;

/** JNI declarations for the QueryNative C API coverage group. */
public final class QueryNative {
  private QueryNative() {}

  public static native int mln_rendered_feature_query_options_default();

  public static native int mln_source_feature_query_options_default();

  public static native int mln_rendered_query_geometry_point();

  public static native int mln_rendered_query_geometry_box();

  public static native int mln_rendered_query_geometry_line_string();

  public static native int mln_render_session_query_rendered_features(
      long session,
      org.maplibre.nativejni.query.RenderedQueryGeometry geometry,
      org.maplibre.nativejni.query.RenderedFeatureQueryOptions options,
      Object[] outFeatures);

  public static native int mln_render_session_query_source_features(
      long session,
      String sourceId,
      org.maplibre.nativejni.query.SourceFeatureQueryOptions options,
      Object[] outFeatures);

  public static native int mln_render_session_query_feature_extensions(
      long session,
      String sourceId,
      org.maplibre.nativejni.geo.Feature feature,
      String extension,
      String extensionField,
      org.maplibre.nativejni.json.JsonValue arguments,
      Object[] outResult);

  public static native int mln_feature_query_result_count();

  public static native int mln_feature_query_result_get();

  public static native int mln_feature_query_result_destroy();

  public static native int mln_feature_extension_result_get();

  public static native int mln_feature_extension_result_destroy();
}

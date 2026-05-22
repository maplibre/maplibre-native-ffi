package org.maplibre.nativejni.internal.bridge;

/** JNI declarations for the OfflineNative C API coverage group. */
public final class OfflineNative {
  private OfflineNative() {}

  public static native int mln_runtime_offline_region_create_start();

  public static native int mln_runtime_offline_region_get_start();

  public static native int mln_runtime_offline_regions_list_start();

  public static native int mln_runtime_offline_regions_merge_database_start();

  public static native int mln_runtime_offline_region_update_metadata_start();

  public static native int mln_runtime_offline_region_get_status_start();

  public static native int mln_runtime_offline_region_set_observed_start();

  public static native int mln_runtime_offline_region_set_download_state_start();

  public static native int mln_runtime_offline_region_invalidate_start();

  public static native int mln_runtime_offline_region_delete_start();

  public static native int mln_runtime_offline_region_create_take_result();

  public static native int mln_runtime_offline_region_get_take_result();

  public static native int mln_runtime_offline_regions_list_take_result();

  public static native int mln_runtime_offline_regions_merge_database_take_result();

  public static native int mln_runtime_offline_region_update_metadata_take_result();

  public static native int mln_runtime_offline_region_get_status_take_result();

  public static native int mln_offline_region_snapshot_get();

  public static native int mln_offline_region_snapshot_destroy();

  public static native int mln_offline_region_list_count();

  public static native int mln_offline_region_list_get();

  public static native int mln_offline_region_list_destroy();
}

extern fn mln_zig_test_fail_next_owned_texture_frame_wrapper_allocation() void;
extern fn mln_zig_test_use_counting_feature_query_result_destroy() void;
extern fn mln_zig_test_restore_feature_query_result_destroy() void;
extern fn mln_zig_test_feature_query_result_destroy_count() usize;

pub fn failNextOwnedTextureFrameWrapperAllocation() void {
    mln_zig_test_fail_next_owned_texture_frame_wrapper_allocation();
}

pub fn useCountingFeatureQueryResultDestroy() void {
    mln_zig_test_use_counting_feature_query_result_destroy();
}

pub fn restoreFeatureQueryResultDestroy() void {
    mln_zig_test_restore_feature_query_result_destroy();
}

pub fn featureQueryResultDestroyCount() usize {
    return mln_zig_test_feature_query_result_destroy_count();
}

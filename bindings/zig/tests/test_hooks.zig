extern fn mln_zig_test_use_counting_buffer_destroy() void;
extern fn mln_zig_test_restore_buffer_destroy() void;
extern fn mln_zig_test_buffer_destroy_count() usize;

pub fn useCountingBufferDestroy() void {
    mln_zig_test_use_counting_buffer_destroy();
}

pub fn restoreBufferDestroy() void {
    mln_zig_test_restore_buffer_destroy();
}

pub fn bufferDestroyCount() usize {
    return mln_zig_test_buffer_destroy_count();
}

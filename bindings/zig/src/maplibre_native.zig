const c = @import("c.zig").raw;

/// Returns the C ABI contract version reported by the linked native library.
pub fn cAbiVersion() u32 {
    return c.mln_c_version();
}

comptime {
    _ = c.MLN_STATUS_OK;
}

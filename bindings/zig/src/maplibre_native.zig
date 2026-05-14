const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const status = @import("status.zig");

pub const Diagnostic = diagnostics.Diagnostic;
pub const DiagnosticStore = diagnostics.DiagnosticStore;
pub const NativeStatusError = status.NativeStatusError;
pub const BindingError = status.BindingError;
pub const Error = status.Error;

/// Returns the C ABI contract version reported by the linked native library.
pub fn cAbiVersion() u32 {
    return c.mln_c_version();
}

/// Validates that the linked native library exposes the C ABI version supported
/// by this Zig package.
pub fn validateAbiVersion(diagnostic_store: ?*DiagnosticStore) Error!void {
    return status.validateAbiVersion(diagnostic_store);
}

comptime {
    _ = c.MLN_STATUS_OK;
}

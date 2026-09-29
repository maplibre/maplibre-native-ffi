const std = @import("std");

const c = @import("c.zig").raw;

/// The failure details of one call.
///
/// Every function whose native call reports a diagnostic takes an optional
/// `*Diagnostic` as its last argument, and `Future.wait` takes one for the
/// completion. When the call returns an error, the diagnostic holds the native
/// status and message, or a message from the binding for a failure that the
/// binding detected. A null diagnostic discards these details.
///
/// The caller owns the diagnostic, and it needs no allocator or deinit. The
/// message lives in a fixed buffer of the native capacity, so a longer
/// completion message is truncated. Read the diagnostic only after a call
/// returns an error, because each call resets it.
pub const Diagnostic = struct {
    /// The native status of the failure, or null for a failure that the
    /// binding detected.
    raw_status: ?i32 = null,
    /// The message storage that native calls and the binding write. Read it
    /// through `message`.
    native: c.mln_diagnostic = .{ .size = @sizeOf(c.mln_diagnostic), .message = @splat(0) },
    /// Whether the current call has recorded its failure. Binding use only.
    recorded: bool = false,

    /// Returns the failure message, borrowed from this diagnostic.
    pub fn message(self: *const Diagnostic) []const u8 {
        return std.mem.sliceTo(&self.native.message, 0);
    }
};

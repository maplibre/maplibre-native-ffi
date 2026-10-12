//! The root of the hand-written runtime modules' inline tests. One root
//! reaches every module, so each module's tests run once.

test {
    _ = @import("call.zig");
    _ = @import("callback.zig");
    _ = @import("completion.zig");
    _ = @import("owner.zig");
    _ = @import("status.zig");
    _ = @import("sync.zig");
}

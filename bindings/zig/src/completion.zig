const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const status = @import("status.zig");
const sync = @import("sync.zig");

pub const CommandDisposition = union(enum) {
    committed,
    superseded,
    failed,
    cancelled,
    unknown: u32,

    fn fromRaw(raw: u32) CommandDisposition {
        return switch (raw) {
            c.MLN_COMMAND_DISPOSITION_COMMITTED => .committed,
            c.MLN_COMMAND_DISPOSITION_SUPERSEDED => .superseded,
            c.MLN_COMMAND_DISPOSITION_FAILED => .failed,
            c.MLN_COMMAND_DISPOSITION_CANCELLED => .cancelled,
            else => .{ .unknown = raw },
        };
    }
};

pub const CommandCompletion = struct {
    disposition: CommandDisposition,
    generation: u64,
    raw_status: i32,

    /// Returns the native error carried by a failed or cancelled command.
    pub fn statusError(self: CommandCompletion) status.NativeStatusError!void {
        return status.errorFromRawStatus(self.raw_status);
    }
};

/// Releases a terminal value the future copied but never handed out.
fn disposeOwned(comptime Value: type, owned: *Value) void {
    switch (@typeInfo(Value)) {
        .optional => |optional| {
            if (owned.*) |*inner| disposeOwned(optional.child, inner);
        },
        .@"struct" => if (@hasDecl(Value, "deinit")) owned.deinit(),
        else => {},
    }
}

pub fn Future(comptime T: type) type {
    return struct {
        const Self = @This();
        const Copy = *const fn (*const c.mln_completion_result, ?*anyopaque) status.Error!T;

        const State = struct {
            refs: std.atomic.Value(usize) = .init(2),
            ready: sync.Latch = .{},
            completed: std.atomic.Value(u32) = .init(0),
            consumed: std.atomic.Value(bool) = .init(false),
            raw_status: i32 = c.MLN_STATUS_OK,
            diagnostic: []u8 = &.{},
            value: ?T = null,
            conversion_error: ?status.Error = null,
            copy: Copy,
            copy_context: ?*anyopaque = null,
            release_copy_context: ?*const fn (?*anyopaque) void = null,

            fn release(state: *State) void {
                if (state.refs.fetchSub(1, .acq_rel) != 1) return;
                if (!state.consumed.load(.acquire)) {
                    if (state.value) |*owned| disposeOwned(T, owned);
                }
                if (state.diagnostic.len != 0) std.heap.smp_allocator.free(state.diagnostic);
                if (state.release_copy_context) |release_context| release_context(state.copy_context);
                std.heap.smp_allocator.destroy(state);
            }

            fn finish(state: *State) void {
                state.completed.store(1, .release);
                state.ready.set();
            }
        };

        state: ?*State,

        pub fn poll(self: *const Self) status.BindingError!bool {
            const state = self.state orelse return error.ClosedHandle;
            return state.completed.load(.acquire) != 0;
        }

        /// Returns the copied terminal diagnostic until this future is
        /// deinitialized, and an empty slice before the completion arrives.
        pub fn diagnostic(self: *const Self) status.BindingError![]const u8 {
            const state = self.state orelse return error.ClosedHandle;
            if (state.completed.load(.acquire) == 0) return &.{};
            return state.diagnostic;
        }

        /// Blocks until the completion arrives and hands out its terminal value.
        ///
        /// A future whose value owns a native handle, such as the one
        /// `MapHandle.create` returns, hands that handle out only here, so a
        /// host waits on those futures rather than deinitializing them: a
        /// discarded creation leaves a live map that keeps its runtime open.
        /// Every other value the future copied is released by `deinit`.
        pub fn wait(self: *Self, diagnostic_store: ?*diagnostics.DiagnosticStore) status.Error!T {
            const state = self.state orelse return error.ClosedHandle;
            state.ready.wait();
            if (state.consumed.swap(true, .acq_rel)) return error.AlreadyCompleted;
            if (state.diagnostic.len != 0) {
                if (diagnostic_store) |store| try store.set(state.raw_status, state.diagnostic);
            }
            if (state.conversion_error) |err| return err;
            if (T != CommandCompletion) try status.errorFromRawStatus(state.raw_status);
            return state.value orelse unreachable;
        }

        pub fn deinit(self: *Self) void {
            const state = self.state orelse return;
            self.state = null;
            state.release();
        }

        fn callback(user_data: ?*anyopaque, raw: [*c]const c.mln_completion_result) callconv(.c) void {
            const state: *State = @ptrCast(@alignCast(user_data orelse return));
            const result: *const c.mln_completion_result = @ptrCast(raw orelse return);
            state.raw_status = result.status;
            if (result.diagnostic.data != null and result.diagnostic.size != 0) {
                const bytes = @as([*]const u8, @ptrCast(result.diagnostic.data))[0..result.diagnostic.size];
                state.diagnostic = std.heap.smp_allocator.dupe(u8, bytes) catch {
                    state.conversion_error = error.OutOfMemory;
                    state.finish();
                    return;
                };
            }
            if (result.status == c.MLN_STATUS_OK or T == CommandCompletion) {
                state.value = state.copy(result, state.copy_context) catch |err| {
                    state.conversion_error = err;
                    state.finish();
                    return;
                };
            }
            state.finish();
        }

        fn releaseUserData(user_data: ?*anyopaque) callconv(.c) void {
            const state: *State = @ptrCast(@alignCast(user_data orelse return));
            state.release();
        }

        fn descriptor(state: *State) c.mln_completion {
            return .{
                .size = @sizeOf(c.mln_completion),
                .callback = callback,
                .user_data = state,
                .release_user_data = releaseUserData,
            };
        }
    };
}

/// Returns a future that already carries `value`, for work the binding
/// satisfied without a native submission. The caller deinitializes it like any
/// other future.
pub fn completed(comptime T: type, terminal_value: T) std.mem.Allocator.Error!Future(T) {
    const FutureType = Future(T);
    const state = try std.heap.smp_allocator.create(FutureType.State);
    state.* = .{
        .refs = .init(1),
        .completed = .init(1),
        .value = terminal_value,
        .copy = struct {
            fn copyResult(_: *const c.mln_completion_result, _: ?*anyopaque) status.Error!T {
                unreachable;
            }
        }.copyResult,
    };
    state.ready.set();
    return .{ .state = state };
}

pub fn submit(
    comptime T: type,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    comptime copy: *const fn (*const c.mln_completion_result) status.Error!T,
    context: anytype,
    comptime start: anytype,
) status.Error!Future(T) {
    const FutureType = Future(T);
    const state = try std.heap.smp_allocator.create(FutureType.State);
    state.* = .{
        .copy = struct {
            fn copyResult(result: *const c.mln_completion_result, _: ?*anyopaque) status.Error!T {
                return copy(result);
            }
        }.copyResult,
    };
    return finishSubmission(T, state, diagnostic_store, context, start);
}

pub fn submitWithCopyContext(
    comptime T: type,
    comptime CopyContext: type,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    comptime copy: *const fn (*const c.mln_completion_result, *CopyContext) status.Error!T,
    copy_context: CopyContext,
    context: anytype,
    comptime start: anytype,
) status.Error!Future(T) {
    const FutureType = Future(T);
    const owned_copy_context = try std.heap.smp_allocator.create(CopyContext);
    owned_copy_context.* = copy_context;
    const state = std.heap.smp_allocator.create(FutureType.State) catch |err| {
        std.heap.smp_allocator.destroy(owned_copy_context);
        return err;
    };
    state.* = .{
        .copy = struct {
            fn copyResult(result: *const c.mln_completion_result, erased: ?*anyopaque) status.Error!T {
                const typed: *CopyContext = @ptrCast(@alignCast(erased orelse return error.NativeError));
                return copy(result, typed);
            }
        }.copyResult,
        .copy_context = @ptrCast(owned_copy_context),
        .release_copy_context = struct {
            fn release(erased: ?*anyopaque) void {
                const typed: *CopyContext = @ptrCast(@alignCast(erased orelse return));
                std.heap.smp_allocator.destroy(typed);
            }
        }.release,
    };
    return finishSubmission(T, state, diagnostic_store, context, start);
}

fn finishSubmission(
    comptime T: type,
    state: *Future(T).State,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    context: anytype,
    comptime start: anytype,
) status.Error!Future(T) {
    const FutureType = Future(T);
    var future = FutureType{ .state = state };
    const completion_descriptor = FutureType.descriptor(state);
    status.checkStatus(start(context, &completion_descriptor), diagnostic_store) catch |err| {
        state.release();
        future.deinit();
        return err;
    };
    return future;
}

pub fn unit(result: *const c.mln_completion_result) status.Error!void {
    if (result.value != null or result.value_count != 0) return error.NativeError;
}

pub fn command(result: *const c.mln_completion_result) status.Error!CommandCompletion {
    if (result.value != null or result.value_count != 0) return error.NativeError;
    return .{
        .disposition = CommandDisposition.fromRaw(result.disposition),
        .generation = result.generation,
        .raw_status = result.status,
    };
}

pub fn value(comptime T: type) *const fn (*const c.mln_completion_result) status.Error!T {
    return struct {
        fn copy(result: *const c.mln_completion_result) status.Error!T {
            if (result.value == null or result.value_count != 1) return error.NativeError;
            const pointer: *align(1) const T = @ptrCast(result.value.?);
            return pointer.*;
        }
    }.copy;
}

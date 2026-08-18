const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const status = @import("status.zig");

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

pub fn Future(comptime T: type) type {
    return struct {
        const Self = @This();
        const Copy = *const fn (*const c.mln_completion_result, ?*anyopaque) status.Error!T;

        const State = struct {
            refs: std.atomic.Value(usize) = .init(2),
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
                if (state.diagnostic.len != 0) std.heap.smp_allocator.free(state.diagnostic);
                if (state.release_copy_context) |release_context| release_context(state.copy_context);
                std.heap.smp_allocator.destroy(state);
            }
        };

        state: ?*State,

        pub fn poll(self: *const Self) status.BindingError!bool {
            const state = self.state orelse return error.ClosedHandle;
            return state.completed.load(.acquire) != 0;
        }

        /// Returns the copied terminal diagnostic until this future is deinitialized.
        pub fn diagnostic(self: *const Self) status.BindingError![]const u8 {
            const state = self.state orelse return error.ClosedHandle;
            return state.diagnostic;
        }

        pub fn wait(self: *Self, diagnostic_store: ?*diagnostics.DiagnosticStore) status.Error!T {
            const state = self.state orelse return error.ClosedHandle;
            while (state.completed.load(.acquire) == 0) std.Thread.yield() catch {};
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
                    state.completed.store(1, .release);
                    return;
                };
            }
            if (result.status == c.MLN_STATUS_OK or T == CommandCompletion) {
                state.value = state.copy(result, state.copy_context) catch |err| {
                    state.conversion_error = err;
                    state.completed.store(1, .release);
                    return;
                };
            }
            state.completed.store(1, .release);
        }

        fn releaseUserData(user_data: ?*anyopaque) callconv(.c) void {
            const state: *State = @ptrCast(@alignCast(user_data orelse return));
            state.release();
        }
    };
}

pub fn submit(
    comptime T: type,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    comptime copy: *const fn (*const c.mln_completion_result) status.Error!T,
    context: anytype,
    comptime start: anytype,
) status.Error!Future(T) {
    return submitWithCopyContext(T, void, diagnostic_store, struct {
        fn copyResult(result: *const c.mln_completion_result, _: *void) status.Error!T {
            return copy(result);
        }
    }.copyResult, {}, context, start);
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
    var future = FutureType{ .state = state };
    const descriptor = c.mln_completion{
        .size = @sizeOf(c.mln_completion),
        .callback = FutureType.callback,
        .user_data = state,
        .release_user_data = FutureType.releaseUserData,
    };
    status.checkStatus(start(context, &descriptor), diagnostic_store) catch |err| {
        state.release();
        future.deinit();
        return err;
    };
    return future;
}

pub fn Submission(comptime T: type, comptime Output: type) type {
    return struct { future: Future(T), output: Output };
}

pub fn submitWithOutput(
    comptime T: type,
    comptime Output: type,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    comptime copy: *const fn (*const c.mln_completion_result) status.Error!T,
    initial_output: Output,
    context: anytype,
    comptime start: anytype,
) status.Error!Submission(T, Output) {
    const FutureType = Future(T);
    const state = try std.heap.smp_allocator.create(FutureType.State);
    state.* = .{
        .copy = struct {
            fn copyResult(result: *const c.mln_completion_result, _: ?*anyopaque) status.Error!T {
                return copy(result);
            }
        }.copyResult,
    };
    var future = FutureType{ .state = state };
    const descriptor = c.mln_completion{
        .size = @sizeOf(c.mln_completion),
        .callback = FutureType.callback,
        .user_data = state,
        .release_user_data = FutureType.releaseUserData,
    };
    var output = initial_output;
    status.checkStatus(start(context, &output, &descriptor), diagnostic_store) catch |err| {
        state.release();
        future.deinit();
        return err;
    };
    return .{ .future = future, .output = output };
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

//! The steps that every generated operation shares.
//!
//! A generated operation names its native function, says how it reaches its
//! receiver, and lists the binding value for each remaining native parameter.
//! The functions here do the rest, in one place: callback admission, the
//! receiver's lease, close, or completion, an arena for encoded inputs, the
//! callback roots that inputs retain, the native call and its status, and the
//! decoding of outputs and completion results while the receiver is still held.
//!
//! Each argument is encoded by its binding type and the native parameter type,
//! both known at compile time, so a mismatch fails to compile:
//!
//! - a value of the parameter's type passes unchanged;
//! - an integer narrows to the parameter's integer type, or fails the call with
//!   `error.InvalidArgument` when it does not fit;
//! - bytes become a buffer view, or a pointer to their first byte;
//! - a generated value converts itself with `toNative`;
//! - a value passed to a const pointer parameter is stored in the arena, and a
//!   slice is converted item by item into the arena unless its items already
//!   have the native type;
//! - null becomes a null pointer or an empty buffer view;
//! - a handle is leased for the call;
//! - `cString`, `out`, `sizedOut`, and `adopt` mark the parameters that need a
//!   NUL-terminated copy or receive an output.

const std = @import("std");

const c = @import("c.zig").raw;
const callback = @import("callback.zig");
const completion = @import("completion.zig");
const diagnostics = @import("diagnostics.zig");
const marshal = @import("marshal.zig");
const owner = @import("owner.zig");
const status = @import("status.zig");

const Diagnostic = diagnostics.Diagnostic;
const Error = status.Error;

/// How an operation reaches its receiver, the first native parameter.
pub const Access = enum {
    /// The operation has no receiver.
    none,
    /// The call leases the receiver.
    lease,
    /// The call leases the receiver and holds off its close, because its
    /// outputs borrow from it until they are copied.
    borrow,
    /// The call passes the receiver's native id without a lease.
    issued,
    /// The receiver is a response that is valid only inside its callback.
    scoped,
    /// The call closes the receiver. A receiver that is already closed makes
    /// the call a no-op.
    close,
    /// The call completes the receiver's pending decision.
    complete,
};

/// Which owner an adopted handle retains: none, the receiver, or the handle
/// passed as the argument at an index.
pub const Parent = union(enum) { none, receiver, argument: usize };

const CString = struct { bytes: []const u8 };

/// Passes `bytes` as a NUL-terminated copy.
pub fn cString(bytes: []const u8) CString {
    return .{ .bytes = bytes };
}

fn Output(comptime T: type, comptime sized: bool, comptime parent: Parent) type {
    return struct {
        const Value = T;
        const sets_size = sized;
        const retains = parent;
    };
}

/// Receives an output parameter, decoded to `T`.
pub fn out(comptime T: type) Output(T, false, .none) {
    return .{};
}

/// Receives an output record whose `size` field native reads first.
pub fn sizedOut(comptime T: type) Output(T, true, .none) {
    return .{};
}

/// Receives a native handle and adopts it as `T`, retaining `parent`.
pub fn adopt(comptime T: type, comptime parent: Parent) Output(T, false, parent) {
    return .{};
}

/// The number of items in a slice, or zero for null.
pub fn len(items: anytype) usize {
    return switch (@typeInfo(@TypeOf(items))) {
        .optional => if (items) |present| present.len else 0,
        else => items.len,
    };
}

fn isOutput(comptime T: type) bool {
    return @typeInfo(T) == .@"struct" and @hasDecl(T, "retains");
}

fn isHandle(comptime T: type) bool {
    return @typeInfo(T) == .@"struct" and @hasDecl(T, "native_name");
}

fn Param(comptime function: anytype, comptime index: usize) type {
    return @typeInfo(@TypeOf(function)).@"fn".params[index].type.?;
}

fn argumentFields(comptime Args: type) []const std.builtin.Type.StructField {
    return @typeInfo(Args).@"struct".fields;
}

/// The value an operation returns for its outputs: nothing, the one output, or
/// a tuple of them in parameter order.
fn Outputs(comptime Args: type) type {
    var types: []const type = &.{};
    for (argumentFields(Args)) |field| {
        if (isOutput(field.type)) types = types ++ .{field.type.Value};
    }
    return switch (types.len) {
        0 => void,
        1 => types[0],
        else => std.meta.Tuple(types),
    };
}

/// Per argument: the lease of a handle argument, or the storage of an output.
fn Held(comptime function: anytype, comptime offset: usize, comptime Args: type) type {
    const fields = argumentFields(Args);
    var types: [fields.len]type = undefined;
    for (fields, 0..) |field, index| {
        types[index] = if (isHandle(field.type))
            field.type.Lease
        else if (isOutput(field.type))
            @typeInfo(Param(function, offset + index)).pointer.child
        else
            void;
    }
    return std.meta.Tuple(&types);
}

fn Native(comptime function: anytype, comptime count: usize) type {
    var types: [count]type = undefined;
    for (0..count) |index| types[index] = Param(function, index);
    return std.meta.Tuple(&types);
}

/// The receiver as the call holds it.
fn Receiver(comptime access: Access, comptime T: type) type {
    return struct {
        const Self = @This();
        const State = switch (access) {
            .none => void,
            .lease, .borrow, .complete => T.Lease,
            .issued => u64,
            .scoped => T,
            .close => T.Close,
        };
        state: State,

        /// The owner that callback admission checks the call against.
        fn admit(comptime name: []const u8, receiver: T) Error!void {
            switch (access) {
                .none => try callback.check(name, 0),
                .scoped => try callback.checkScoped(name, @intFromPtr(receiver.native)),
                else => try callback.check(name, receiver.raw),
            }
        }

        /// The owner that a registration the call retains calls back into.
        fn ownerId(receiver: T) u64 {
            return switch (access) {
                .none => 0,
                .scoped => @intFromPtr(receiver.native),
                else => receiver.raw,
            };
        }

        /// Takes hold of the receiver, or returns null for a closed one.
        fn acquire(receiver: T, diagnostic: ?*Diagnostic) Error!?Self {
            return .{ .state = switch (access) {
                .none => {},
                .lease => try receiver.lease(diagnostic),
                .borrow => try receiver.borrow(diagnostic),
                .complete => try receiver.beginComplete(diagnostic),
                .issued => receiver.raw,
                .scoped => receiver,
                .close => try receiver.beginClose(diagnostic) orelse return null,
            } };
        }

        fn native(self: *Self) switch (access) {
            .none => void,
            .scoped => @FieldType(T, "native"),
            else => u64,
        } {
            return switch (access) {
                .none => {},
                .issued => self.state,
                .scoped => self.state.native,
                else => self.state.native,
            };
        }

        /// Whether the native call runs. A close that native must not see yet
        /// is recorded and committed without one.
        fn calls(self: Self) bool {
            return if (access == .close) !self.state.deferred else true;
        }

        fn anchor(self: Self) ?owner.Anchor {
            return switch (access) {
                .lease, .borrow, .complete => self.state.anchor(),
                else => null,
            };
        }

        /// Settles a successful call: a decision is complete before the roots
        /// are accepted, and a close commits after them.
        fn succeed(self: Self, roots: *callback.Roots) void {
            if (access == .complete) self.state.finishComplete(true);
            roots.accept();
            if (access == .close) self.state.commit();
        }

        fn fail(self: Self) void {
            switch (access) {
                .close => self.state.rollback(),
                .complete => self.state.finishComplete(false),
                else => {},
            }
        }

        fn release(self: Self) void {
            switch (access) {
                .lease, .borrow => self.state.release(),
                else => {},
            }
        }
    };
}

/// One operation in flight: its receiver, input arena, roots, and the leases
/// and output storage of its arguments.
fn Operation(comptime name: []const u8, comptime access: Access, comptime ReceiverType: type, comptime Allocator: type, comptime Args: type) type {
    const function = @field(c, name);
    const offset: usize = if (access == .none) 0 else 1;
    const fields = argumentFields(Args);
    return struct {
        const Self = @This();
        const Hold = Receiver(access, ReceiverType);
        const NativeArgs = Native(function, offset + fields.len);

        receiver: Hold,
        /// Where a lifecycle error of the receiver or an input handle is
        /// recorded.
        diagnostic: ?*Diagnostic,
        arena: if (Allocator == std.mem.Allocator) std.heap.ArenaAllocator else void,
        roots: callback.Roots = .{},
        held: Held(function, offset, Args) = undefined,
        leased: usize = 0,
        settled: bool = false,
        /// The receiver id that a closing call passes by pointer. Native may
        /// clear it without touching the id the close commits.
        closing: u64 = 0,

        /// Admits the call and takes hold of the receiver, or returns null for
        /// a receiver that is already closed.
        fn begin(receiver: ReceiverType, allocator: Allocator, diagnostic: ?*Diagnostic) Error!?Self {
            try Hold.admit(name, receiver);
            const hold = try Hold.acquire(receiver, diagnostic) orelse return null;
            return .{
                .receiver = hold,
                .roots = .{ .owner = Hold.ownerId(receiver) },
                .diagnostic = diagnostic,
                .arena = if (Allocator == std.mem.Allocator) std.heap.ArenaAllocator.init(allocator) else {},
            };
        }

        /// Releases what the call holds. A call that failed before it settled
        /// rolls back its receiver and releases its roots unaccepted.
        fn end(self: *Self) void {
            inline for (fields, 0..) |field, index| {
                if (comptime isHandle(field.type)) {
                    if (index < self.leased) self.held[index].release();
                }
            }
            if (!self.settled) self.receiver.fail();
            self.receiver.release();
            self.roots.deinit();
            if (Allocator == std.mem.Allocator) self.arena.deinit();
        }

        fn inputAllocator(self: *Self) std.mem.Allocator {
            if (Allocator != std.mem.Allocator) @compileError(name ++ ": an input needs an allocator parameter");
            return self.arena.allocator();
        }

        /// Encodes the receiver and every argument into native arguments.
        fn arguments(self: *Self, args: Args) Error!NativeArgs {
            var native: NativeArgs = undefined;
            if (access == .close and @typeInfo(Param(function, 0)) == .pointer) {
                self.closing = self.receiver.state.native;
                native[0] = &self.closing;
            } else if (offset == 1) {
                native[0] = self.receiver.native();
            }
            inline for (fields, 0..) |field, index| {
                const Parameter = Param(function, offset + index);
                const input = @field(args, field.name);
                if (comptime isHandle(field.type)) {
                    self.held[index] = try input.lease(self.diagnostic);
                    self.leased = index + 1;
                    native[offset + index] = self.held[index].native;
                } else if (comptime isOutput(field.type)) {
                    self.held[index] = std.mem.zeroes(@TypeOf(self.held[index]));
                    if (field.type.sets_size) self.held[index].size = @sizeOf(@TypeOf(self.held[index]));
                    native[offset + index] = &self.held[index];
                } else {
                    native[offset + index] = try self.encode(Parameter, input);
                }
            }
            return native;
        }

        fn encode(self: *Self, comptime Target: type, input: anytype) Error!Target {
            const Input = @TypeOf(input);
            if (Input == Target) return input;
            if (Input == CString) return @ptrCast(try marshal.cString(self.inputAllocator(), input.bytes));
            if (@typeInfo(Input) == .optional) {
                if (input) |present| return self.encode(Target, present);
                if (Target == c.mln_buffer_view) return std.mem.zeroes(c.mln_buffer_view);
                return null;
            }
            if (Target == c.mln_buffer_view and Input == []const u8) return marshal.view(input);
            switch (@typeInfo(Target)) {
                .pointer => |pointer| return self.encodePointer(Target, pointer.child, pointer.is_const, input),
                .int => if (@typeInfo(Input) == .int) return std.math.cast(Target, input) orelse error.InvalidArgument,
                else => {},
            }
            switch (@typeInfo(Input)) {
                .@"struct", .@"enum", .@"union" => if (@hasDecl(Input, "toNative")) {
                    return switch (@typeInfo(@TypeOf(Input.toNative)).@"fn".params.len) {
                        1 => self.encode(Target, input.toNative()),
                        else => try input.toNative(self.inputAllocator(), &self.roots),
                    };
                },
                else => {},
            }
            return input;
        }

        fn encodePointer(self: *Self, comptime Target: type, comptime Item: type, comptime is_const: bool, input: anytype) Error!Target {
            const Input = @TypeOf(input);
            if (!is_const) @compileError(name ++ ": a mutable pointer parameter needs an output marker");
            // Bytes passed by pointer to a view are one value, not an array.
            const one_view = Item == c.mln_buffer_view and Input == []const u8;
            if (!one_view and @typeInfo(Input) == .pointer and @typeInfo(Input).pointer.size == .slice) {
                const Element = @typeInfo(Input).pointer.child;
                if (Element == Item or (Input == []const u8 and @sizeOf(Item) == 1)) return @ptrCast(input.ptr);
                const items = try self.inputAllocator().alloc(Item, input.len);
                for (input, items) |item, *target| target.* = try self.encode(Item, item);
                return items.ptr;
            }
            const stored = try self.inputAllocator().create(Item);
            stored.* = try self.encode(Item, input);
            return stored;
        }

        /// Decodes the outputs while the receiver and argument handles are held.
        fn outputs(self: *Self, allocator: Allocator) Error!Outputs(Args) {
            const indices = comptime outputIndices(Args);
            switch (indices.len) {
                0 => return,
                1 => return self.output(indices[0], allocator),
                else => {
                    var result: Outputs(Args) = undefined;
                    inline for (indices, 0..) |index, position| result[position] = try self.output(index, allocator);
                    return result;
                },
            }
        }

        /// The anchor of the owner that an adopted handle retains, borrowed
        /// from the lease the call holds on it.
        fn parent(self: *Self, comptime retains: Parent) ?owner.Anchor {
            return switch (retains) {
                .none => null,
                .receiver => self.receiver.anchor(),
                .argument => |index| self.held[index].anchor(),
            };
        }

        fn output(self: *Self, comptime index: usize, allocator: Allocator) Error!fields[index].type.Value {
            const Step = fields[index].type;
            if (comptime marshal.decodeAllocates(Step.Value) and Allocator != std.mem.Allocator) @compileError(name ++ ": an output needs an allocator parameter");
            return marshal.decode(Step.Value, self.held[index], .{
                .allocator = if (Allocator == std.mem.Allocator) allocator else null,
                .parent = self.parent(Step.retains),
            });
        }
    };
}

fn outputIndices(comptime Args: type) []const usize {
    var indices: []const usize = &.{};
    for (argumentFields(Args), 0..) |field, index| {
        if (isOutput(field.type)) indices = indices ++ .{index};
    }
    return indices;
}

/// The result of a call whose receiver was already closed: nothing to do.
fn closed(comptime T: type) Error!T {
    if (T == void) return;
    if (T == completion.Future(void)) return completion.completed(void, {});
    // Only a closing call finds its receiver closed, and a close returns no
    // value of its own.
    unreachable;
}

/// Calls a status-returning native function and returns its decoded outputs.
pub fn invoke(
    comptime name: []const u8,
    comptime access: Access,
    receiver: anytype,
    allocator: anytype,
    diagnostic: ?*Diagnostic,
    args: anytype,
) Error!Outputs(@TypeOf(args)) {
    status.begin(diagnostic);
    errdefer |err| status.fail(diagnostic, err);
    const Op = Operation(name, access, @TypeOf(receiver), @TypeOf(allocator), @TypeOf(args));
    var op = try Op.begin(receiver, allocator, diagnostic) orelse return closed(Outputs(@TypeOf(args)));
    defer op.end();
    const native = try op.arguments(args);
    if (op.receiver.calls()) try status.call(@field(c, name), native, diagnostic);
    op.receiver.succeed(&op.roots);
    op.settled = true;
    return op.outputs(allocator);
}

/// Calls a status-returning native function like `invoke`, except that the
/// `absent` status reports that native published no output, which returns null.
pub fn invokeUnless(
    comptime name: []const u8,
    comptime access: Access,
    receiver: anytype,
    allocator: anytype,
    diagnostic: ?*Diagnostic,
    comptime absent: i32,
    args: anytype,
) Error!?Outputs(@TypeOf(args)) {
    status.begin(diagnostic);
    errdefer |err| status.fail(diagnostic, err);
    const Op = Operation(name, access, @TypeOf(receiver), @TypeOf(allocator), @TypeOf(args));
    // Only a closing call finds its receiver closed, and a close has no output.
    var op = (try Op.begin(receiver, allocator, diagnostic)).?;
    defer op.end();
    const native = try op.arguments(args);
    const present = try status.callUnless(@field(c, name), native, diagnostic, absent);
    op.receiver.succeed(&op.roots);
    op.settled = true;
    if (!present) return null;
    return try op.outputs(allocator);
}

/// Calls a status-returning native function like `invoke` whose one output
/// reports, when true, that native kept nothing from the call's registration.
/// The roots of a declined registration release with the call, and the call
/// returns whether native declined it.
pub fn invokeDeclinable(
    comptime name: []const u8,
    comptime access: Access,
    receiver: anytype,
    allocator: anytype,
    diagnostic: ?*Diagnostic,
    args: anytype,
) Error!bool {
    status.begin(diagnostic);
    errdefer |err| status.fail(diagnostic, err);
    const Op = Operation(name, access, @TypeOf(receiver), @TypeOf(allocator), @TypeOf(args));
    if (Outputs(@TypeOf(args)) != bool) @compileError(name ++ ": a declinable call has one boolean output");
    // Only a closing call finds its receiver closed, and a close declines nothing.
    var op = (try Op.begin(receiver, allocator, diagnostic)).?;
    defer op.end();
    const native = try op.arguments(args);
    try status.call(@field(c, name), native, diagnostic);
    const declined = try op.outputs(allocator);
    var kept: callback.Roots = .{};
    op.receiver.succeed(if (declined) &kept else &op.roots);
    op.settled = true;
    return declined;
}

/// Calls a native function that returns its result rather than a status, and
/// decodes the result to `T`.
pub fn direct(
    comptime name: []const u8,
    comptime access: Access,
    receiver: anytype,
    comptime T: type,
    allocator: anytype,
    args: anytype,
) Error!T {
    const Op = Operation(name, access, @TypeOf(receiver), @TypeOf(allocator), @TypeOf(args));
    var op = try Op.begin(receiver, allocator, null) orelse return closed(T);
    defer op.end();
    const native = try op.arguments(args);
    if (T == void) {
        if (op.receiver.calls()) @call(.auto, @field(c, name), native);
        op.receiver.succeed(&op.roots);
        op.settled = true;
        return;
    }
    const raw = @call(.auto, @field(c, name), native);
    op.receiver.succeed(&op.roots);
    op.settled = true;
    if (comptime marshal.decodeAllocates(T) and @TypeOf(allocator) != std.mem.Allocator) @compileError(name ++ ": the result needs an allocator parameter");
    return marshal.decode(T, raw, .{ .allocator = if (@TypeOf(allocator) == std.mem.Allocator) allocator else null });
}

/// A future and the outputs that native wrote when it accepted the work.
pub fn Started(comptime Value: type, comptime Args: type) type {
    return struct { outputs: Outputs(Args), ready: completion.Future(Value) };
}

fn Submitted(comptime Copy: type, comptime Args: type) type {
    return if (Outputs(Args) == void) completion.Future(Copy.Value) else Started(Copy.Value, Args);
}

/// The context a completion copier receives: the caller's allocator and a
/// retained parent for an adopted handle.
const CopyContext = struct {
    decode: marshal.Decode,
    pub fn deinit(self: *CopyContext) void {
        if (self.decode.parent) |parent| parent.release();
    }
};

/// Starts native work that reports through a completion, and returns its
/// future along with any outputs. `Copy` is one of the result copiers below.
pub fn submit(
    comptime name: []const u8,
    comptime access: Access,
    receiver: anytype,
    comptime Copy: type,
    allocator: anytype,
    diagnostic: ?*Diagnostic,
    args: anytype,
) Error!Submitted(Copy, @TypeOf(args)) {
    status.begin(diagnostic);
    errdefer |err| status.fail(diagnostic, err);
    const Allocator = @TypeOf(allocator);
    const Op = Operation(name, access, @TypeOf(receiver), Allocator, @TypeOf(args));
    var op = try Op.begin(receiver, allocator, diagnostic) orelse return closed(Submitted(Copy, @TypeOf(args)));
    defer op.end();
    const native = try op.arguments(args);
    const function = @field(c, name);
    const allocates = comptime marshal.decodeAllocates(Copy.Value);
    if (allocates and Allocator != std.mem.Allocator) @compileError(name ++ ": the result needs an allocator parameter");
    const adopts = comptime marshal.decodeAdopts(Copy.Value);
    var ready = if (allocates or adopts) try completion.submitWithCopyContext(Copy.Value, CopyContext, diagnostic, struct {
        fn copy(result: *const c.mln_completion_result, context: *CopyContext) Error!Copy.Value {
            return Copy.copy(result, context.decode);
        }
    }.copy, .{ .decode = .{
        .allocator = if (Allocator == std.mem.Allocator) allocator else null,
        .parent = if (comptime @hasDecl(Copy, "retains")) if (op.parent(Copy.retains)) |anchor| anchor.retain() else null else null,
    } }, function, native) else try completion.submit(Copy.Value, diagnostic, struct {
        fn copy(result: *const c.mln_completion_result) Error!Copy.Value {
            return Copy.copy(result, .{});
        }
    }.copy, function, native);
    op.receiver.succeed(&op.roots);
    op.settled = true;
    if (Outputs(@TypeOf(args)) == void) return ready;
    errdefer ready.deinit();
    return .{ .outputs = try op.outputs(allocator), .ready = ready };
}

// Completion result copiers. Each names the future's value type and copies it
// out of the completion result before native reuses the result's storage.

/// A command's disposition, generation, and status.
pub const command = struct {
    pub const Value = completion.CommandCompletion;
    pub fn copy(result: *const c.mln_completion_result, _: marshal.Decode) Error!Value {
        return completion.command(result);
    }
};

/// Work that completes without a value.
pub const unit = struct {
    pub const Value = void;
    pub fn copy(result: *const c.mln_completion_result, _: marshal.Decode) Error!Value {
        return completion.unit(result);
    }
};

/// One native value of type `Raw`, decoded to `T`.
pub fn value(comptime T: type, comptime Raw: type) type {
    return struct {
        pub const Value = T;
        pub fn copy(result: *const c.mln_completion_result, context: marshal.Decode) Error!Value {
            return marshal.decode(T, try completion.value(Raw)(result), context);
        }
    };
}

/// A native handle of type `Raw`, adopted as `T` under `parent`.
pub fn handle(comptime T: type, comptime Raw: type, comptime parent: Parent) type {
    return struct {
        pub const Value = T;
        pub const retains = parent;
        pub fn copy(result: *const c.mln_completion_result, context: marshal.Decode) Error!Value {
            return T.adopt(try completion.value(Raw)(result), context.parent);
        }
    };
}

/// A copier's value, or null when native completes with a null payload.
pub fn orNull(comptime Copy: type) type {
    return struct {
        pub const Value = ?Copy.Value;
        pub fn copy(result: *const c.mln_completion_result, context: marshal.Decode) Error!Value {
            if (result.value == null) return null;
            return try Copy.copy(result, context);
        }
    };
}

/// A copier's value, or null when native completes with an empty payload: an
/// empty buffer, or no value.
pub fn orEmpty(comptime Copy: type, comptime Raw: type) type {
    return struct {
        pub const Value = ?Copy.Value;
        pub fn copy(result: *const c.mln_completion_result, context: marshal.Decode) Error!Value {
            if (Raw == c.mln_buffer_view) {
                if ((try completion.value(Raw)(result)).size == 0) return null;
            } else if (result.value_count == 0) return null;
            return try Copy.copy(result, context);
        }
    };
}

/// An array of native `Raw` items, copied as `Item`s into an arena of its own.
pub fn slice(comptime Item: type, comptime Raw: type) type {
    return struct {
        pub const Value = marshal.OwnedValue([]const Item);
        pub fn copy(result: *const c.mln_completion_result, context: marshal.Decode) Error!Value {
            const items = try marshal.nativeSlice(Raw, @ptrCast(@alignCast(result.value)), result.value_count);
            var arena = std.heap.ArenaAllocator.init(context.allocator.?);
            errdefer arena.deinit();
            const copied = try arena.allocator().alloc(Item, items.len);
            for (items, copied) |item, *target| target.* = try marshal.decode(Item, item, .{ .allocator = arena.allocator() });
            return .{ .arena = arena, .value = copied };
        }
    };
}

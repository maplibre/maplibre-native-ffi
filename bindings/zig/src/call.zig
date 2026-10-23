// Native leases each id while its call runs. Encoded inputs and callback roots
// remain live until the native call settles.

const std = @import("std");

const c = @import("c.zig").raw;
const callback = @import("callback.zig");
const completion = @import("completion.zig");
const diagnostics = @import("diagnostics.zig");
const marshal = @import("marshal.zig");
const status = @import("status.zig");

const Diagnostic = diagnostics.Diagnostic;
const Error = status.Error;

pub const Access = enum { none, handle, scoped };

const CString = struct { bytes: []const u8 };

pub fn cString(bytes: []const u8) CString {
    return .{ .bytes = bytes };
}

fn Output(comptime T: type, comptime sized: bool) type {
    return struct {
        pub const Value = T;
        pub const sets_size = sized;
        pub const output = true;
    };
}

pub fn out(comptime T: type) Output(T, false) {
    return .{};
}

/// Receives an output record whose `size` field native reads first.
pub fn sizedOut(comptime T: type) Output(T, true) {
    return .{};
}

/// Receives `count` output elements, one for each element of an input array.
pub fn outArray(comptime T: type, count: usize) OutputArray(T) {
    return .{ .count = count };
}

fn OutputArray(comptime T: type) type {
    return struct {
        pub const Value = marshal.OwnedValue([]const T);
        pub const array_element = T;
        pub const output = true;
        count: usize,
    };
}

/// Receives text that native writes into storage the call sizes. The call
/// passes the storage's capacity to the `textCapacity` argument and reads the
/// text's length, which native reports whether or not the text fit, from the
/// `textLength` argument. Native reports the same text for the same inputs, so
/// a call that did not fit is made again with enough storage.
pub fn text() Text {
    return .{};
}

const Text = struct {
    pub const Value = marshal.OwnedValue([]const u8);
    pub const output = true;
    pub const caller_sized_text = true;
};

pub fn textCapacity() TextCapacity {
    return .{};
}

const TextCapacity = struct {
    pub const text_capacity = true;
};

pub fn textLength() TextLength {
    return .{};
}

const TextLength = struct {
    pub const text_length = true;
};

fn marks(comptime T: type, comptime marker: []const u8) bool {
    return @typeInfo(T) == .@"struct" and @hasDecl(T, marker);
}

fn hasText(comptime Args: type) bool {
    for (argumentTypes(Args)) |Field| {
        if (marks(Field, "caller_sized_text")) return true;
    }
    return false;
}

/// Receives a native handle and adopts it as `T`.
pub fn adopt(comptime T: type) Output(T, false) {
    return .{};
}

pub fn len(items: anytype) usize {
    return switch (@typeInfo(@TypeOf(items))) {
        .optional => if (items) |present| present.len else 0,
        else => items.len,
    };
}

fn isOutput(comptime T: type) bool {
    return @typeInfo(T) == .@"struct" and @hasDecl(T, "output");
}

fn isHandle(comptime T: type) bool {
    return @typeInfo(T) == .@"struct" and @hasDecl(T, "native_name");
}

fn Param(comptime function: anytype, comptime index: usize) type {
    return @typeInfo(@TypeOf(function)).@"fn".param_types[index].?;
}

fn argumentTypes(comptime Args: type) []const type {
    return @typeInfo(Args).@"struct".field_types;
}

fn Outputs(comptime Args: type) type {
    var types: []const type = &.{};
    for (argumentTypes(Args)) |Field| {
        if (isOutput(Field)) types = types ++ .{Field.Value};
    }
    return switch (types.len) {
        0 => void,
        1 => types[0],
        else => @Tuple(types),
    };
}

fn Held(comptime function: anytype, comptime offset: usize, comptime Args: type) type {
    const fields = argumentTypes(Args);
    var types: [fields.len]type = undefined;
    for (fields, 0..) |Field, index| {
        types[index] = if (isOutput(Field))
            if (marks(Field, "caller_sized_text"))
                void
            else if (@hasDecl(Field, "array_element"))
                []@typeInfo(Param(function, offset + index)).pointer.child
            else
                @typeInfo(Param(function, offset + index)).pointer.child
        else
            void;
    }
    return @Tuple(&types);
}

fn Native(comptime function: anytype, comptime count: usize) type {
    var types: [count]type = undefined;
    for (0..count) |index| types[index] = Param(function, index);
    return @Tuple(&types);
}

fn Operation(comptime name: []const u8, comptime access: Access, comptime ReceiverType: type, comptime Allocator: type, comptime Args: type) type {
    const function = @field(c, name);
    const offset: usize = if (access == .none) 0 else 1;
    const fields = argumentTypes(Args);
    return struct {
        const Self = @This();
        const NativeArgs = Native(function, offset + fields.len);

        receiver: ReceiverType,
        arena: if (Allocator == std.mem.Allocator) std.heap.ArenaAllocator else void,
        roots: callback.Roots = .{},
        held: Held(function, offset, Args) = undefined,
        text_storage: []u8 = &.{},
        text_length: usize = 0,

        fn begin(receiver: ReceiverType, allocator: Allocator) Error!Self {
            if (access == .scoped) try callback.checkScoped(@intFromPtr(receiver.native));
            return .{
                .receiver = receiver,
                .arena = if (Allocator == std.mem.Allocator) std.heap.ArenaAllocator.init(allocator) else {},
            };
        }

        /// Releases encoded inputs and unaccepted callback roots.
        fn end(self: *Self) void {
            self.roots.deinit();
            if (Allocator == std.mem.Allocator) self.arena.deinit();
        }

        fn inputAllocator(self: *Self) std.mem.Allocator {
            if (Allocator != std.mem.Allocator) @compileError(name ++ ": an input needs an allocator parameter");
            return self.arena.allocator();
        }

        fn arguments(self: *Self, args: Args) Error!NativeArgs {
            var native: NativeArgs = undefined;
            if (offset == 1) {
                native[0] = if (access == .scoped) self.receiver.native else self.receiver.raw;
            }
            inline for (fields, 0..) |Field, index| {
                const Parameter = Param(function, offset + index);
                const input = args[index];
                if (comptime isHandle(Field)) {
                    native[offset + index] = input.raw;
                } else if (comptime marks(Field, "caller_sized_text") or marks(Field, "text_capacity")) {
                    // Set below, once the text has storage.
                } else if (comptime marks(Field, "text_length")) {
                    native[offset + index] = &self.text_length;
                } else if (comptime isOutput(Field)) {
                    if (comptime @hasDecl(Field, "array_element")) {
                        const Element = @typeInfo(Parameter).pointer.child;
                        self.held[index] = try self.inputAllocator().alloc(Element, input.count);
                        for (self.held[index]) |*item| {
                            item.* = std.mem.zeroes(Element);
                            if (comptime @typeInfo(Element) == .@"struct" and @hasField(Element, "size")) item.size = @sizeOf(Element);
                        }
                        native[offset + index] = self.held[index].ptr;
                    } else {
                        self.held[index] = std.mem.zeroes(@TypeOf(self.held[index]));
                        if (Field.sets_size) self.held[index].size = @sizeOf(@TypeOf(self.held[index]));
                        native[offset + index] = &self.held[index];
                    }
                } else {
                    native[offset + index] = try self.encode(Parameter, input);
                }
            }
            if (comptime hasText(Args)) {
                self.text_storage = try self.inputAllocator().alloc(u8, 256);
                self.passText(&native);
            }
            return native;
        }

        /// Passes the text's storage and its capacity to the native call.
        fn passText(self: *Self, native: *NativeArgs) void {
            inline for (fields, 0..) |Field, index| {
                if (comptime marks(Field, "caller_sized_text")) native[offset + index] = @ptrCast(self.text_storage.ptr);
                if (comptime marks(Field, "text_capacity")) native[offset + index] = self.text_storage.len;
            }
        }

        /// Makes the call again with enough storage while the text did not fit.
        fn fitText(self: *Self, native: *NativeArgs, diagnostic: ?*Diagnostic) Error!void {
            if (comptime !hasText(Args)) return;
            while (self.text_length >= self.text_storage.len) {
                self.text_storage = try self.inputAllocator().alloc(u8, self.text_length + 1);
                self.passText(native);
                try status.call(function, native.*, diagnostic);
            }
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
                .pointer => |pointer| return self.encodePointer(Target, pointer.child, pointer.attrs.@"const", input),
                .int => if (@typeInfo(Input) == .int) return std.math.cast(Target, input) orelse error.InvalidArgument,
                else => {},
            }
            switch (@typeInfo(Input)) {
                .@"struct", .@"enum", .@"union" => if (@hasDecl(Input, "toNative")) {
                    return switch (@typeInfo(@TypeOf(Input.toNative)).@"fn".param_types.len) {
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

        /// Copies borrowed outputs before the operation returns.
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

        fn output(self: *Self, comptime index: usize, allocator: Allocator) Error!fields[index].Value {
            const Step = fields[index];
            if (comptime marshal.decodeAllocates(Step.Value) and Allocator != std.mem.Allocator) @compileError(name ++ ": an output needs an allocator parameter");
            if (comptime marks(Step, "caller_sized_text")) {
                var arena = std.heap.ArenaAllocator.init(allocator);
                errdefer arena.deinit();
                const bytes = try arena.allocator().dupe(u8, self.text_storage[0..self.text_length]);
                return .{ .arena = arena, .value = bytes };
            }
            if (comptime @hasDecl(Step, "array_element")) {
                var arena = std.heap.ArenaAllocator.init(allocator);
                errdefer arena.deinit();
                const values = try arena.allocator().alloc(Step.array_element, self.held[index].len);
                for (values, self.held[index]) |*decoded, native_value| {
                    decoded.* = try marshal.decode(Step.array_element, native_value, .{ .allocator = arena.allocator() });
                }
                return .{ .arena = arena, .value = values };
            }
            return marshal.decode(Step.Value, self.held[index], .{
                .allocator = if (Allocator == std.mem.Allocator) allocator else null,
            });
        }
    };
}

fn outputIndices(comptime Args: type) []const usize {
    var indices: []const usize = &.{};
    for (argumentTypes(Args), 0..) |Field, index| {
        if (isOutput(Field)) indices = indices ++ .{index};
    }
    return indices;
}

pub fn invoke(
    comptime name: []const u8,
    comptime access: Access,
    receiver: anytype,
    allocator: anytype,
    diagnostic: ?*Diagnostic,
    args: anytype,
) Error!Outputs(@TypeOf(args)) {
    status.begin(diagnostic);
    return status.propagate(diagnostic, invokeInner(name, access, receiver, allocator, diagnostic, args));
}

fn invokeInner(
    comptime name: []const u8,
    comptime access: Access,
    receiver: anytype,
    allocator: anytype,
    diagnostic: ?*Diagnostic,
    args: anytype,
) Error!Outputs(@TypeOf(args)) {
    const Op = Operation(name, access, @TypeOf(receiver), @TypeOf(allocator), @TypeOf(args));
    var op = try Op.begin(receiver, allocator);
    defer op.end();
    var native = try op.arguments(args);
    try status.call(@field(c, name), native, diagnostic);
    try op.fitText(&native, diagnostic);
    op.roots.accept();
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
    return status.propagate(diagnostic, invokeUnlessInner(name, access, receiver, allocator, diagnostic, absent, args));
}

fn invokeUnlessInner(
    comptime name: []const u8,
    comptime access: Access,
    receiver: anytype,
    allocator: anytype,
    diagnostic: ?*Diagnostic,
    comptime absent: i32,
    args: anytype,
) Error!?Outputs(@TypeOf(args)) {
    const Op = Operation(name, access, @TypeOf(receiver), @TypeOf(allocator), @TypeOf(args));
    var op = try Op.begin(receiver, allocator);
    defer op.end();
    const native = try op.arguments(args);
    const present = try status.callUnless(@field(c, name), native, diagnostic, absent);
    op.roots.accept();
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
    return status.propagate(diagnostic, invokeDeclinableInner(name, access, receiver, allocator, diagnostic, args));
}

fn invokeDeclinableInner(
    comptime name: []const u8,
    comptime access: Access,
    receiver: anytype,
    allocator: anytype,
    diagnostic: ?*Diagnostic,
    args: anytype,
) Error!bool {
    const Op = Operation(name, access, @TypeOf(receiver), @TypeOf(allocator), @TypeOf(args));
    if (Outputs(@TypeOf(args)) != bool) @compileError(name ++ ": a declinable call has one boolean output");
    var op = try Op.begin(receiver, allocator);
    defer op.end();
    const native = try op.arguments(args);
    try status.call(@field(c, name), native, diagnostic);
    const declined = try op.outputs(allocator);
    if (!declined) op.roots.accept();
    return declined;
}

pub fn direct(
    comptime name: []const u8,
    comptime access: Access,
    receiver: anytype,
    comptime T: type,
    allocator: anytype,
    args: anytype,
) Error!T {
    const Op = Operation(name, access, @TypeOf(receiver), @TypeOf(allocator), @TypeOf(args));
    var op = try Op.begin(receiver, allocator);
    defer op.end();
    const native = try op.arguments(args);
    if (T == void) {
        @call(.auto, @field(c, name), native);
        op.roots.accept();
        return;
    }
    const raw = @call(.auto, @field(c, name), native);
    op.roots.accept();
    if (comptime marshal.decodeAllocates(T) and @TypeOf(allocator) != std.mem.Allocator) @compileError(name ++ ": the result needs an allocator parameter");
    return marshal.decode(T, raw, .{ .allocator = if (@TypeOf(allocator) == std.mem.Allocator) allocator else null });
}

pub fn Started(comptime Value: type, comptime Args: type) type {
    return struct { outputs: Outputs(Args), ready: completion.Future(Value) };
}

fn Submitted(comptime Copy: type, comptime Args: type) type {
    return if (Outputs(Args) == void) completion.Future(Copy.Value) else Started(Copy.Value, Args);
}

/// The allocator that a completion copier receives.
const CopyContext = struct {
    decode: marshal.Decode,
};

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
    return status.propagate(diagnostic, submitInner(name, access, receiver, Copy, allocator, diagnostic, args));
}

fn submitInner(
    comptime name: []const u8,
    comptime access: Access,
    receiver: anytype,
    comptime Copy: type,
    allocator: anytype,
    diagnostic: ?*Diagnostic,
    args: anytype,
) Error!Submitted(Copy, @TypeOf(args)) {
    const Allocator = @TypeOf(allocator);
    const Op = Operation(name, access, @TypeOf(receiver), Allocator, @TypeOf(args));
    var op = try Op.begin(receiver, allocator);
    defer op.end();
    const native = try op.arguments(args);
    const function = @field(c, name);
    const allocates = comptime marshal.decodeAllocates(Copy.Value);
    if (allocates and Allocator != std.mem.Allocator) @compileError(name ++ ": the result needs an allocator parameter");
    var ready = if (allocates) try completion.submitWithCopyContext(Copy.Value, CopyContext, diagnostic, struct {
        fn copy(result: *const c.mln_completion_result, context: *CopyContext) Error!Copy.Value {
            return Copy.copy(result, context.decode);
        }
    }.copy, .{ .decode = .{
        .allocator = if (Allocator == std.mem.Allocator) allocator else null,
    } }, function, native) else try completion.submit(Copy.Value, diagnostic, struct {
        fn copy(result: *const c.mln_completion_result) Error!Copy.Value {
            return Copy.copy(result, .{});
        }
    }.copy, function, native);
    op.roots.accept();
    if (Outputs(@TypeOf(args)) == void) return ready;
    errdefer ready.deinit();
    return .{ .outputs = try op.outputs(allocator), .ready = ready };
}

pub fn discard(
    comptime name: []const u8,
    comptime access: Access,
    receiver: anytype,
    allocator: anytype,
    diagnostic: ?*Diagnostic,
    args: anytype,
) Error!void {
    status.begin(diagnostic);
    return status.propagate(diagnostic, discardInner(name, access, receiver, allocator, diagnostic, args));
}

fn discardInner(
    comptime name: []const u8,
    comptime access: Access,
    receiver: anytype,
    allocator: anytype,
    diagnostic: ?*Diagnostic,
    args: anytype,
) Error!void {
    const Op = Operation(name, access, @TypeOf(receiver), @TypeOf(allocator), @TypeOf(args));
    var op = try Op.begin(receiver, allocator);
    defer op.end();
    const native = try op.arguments(args);
    try status.call(@field(c, name), native ++ .{@as(?*const c.mln_completion, null)}, diagnostic);
    op.roots.accept();
}

pub fn scopedView(
    comptime name: []const u8,
    comptime begin: []const u8,
    comptime end: []const u8,
    comptime Value: type,
    comptime Result: type,
    receiver: anytype,
    context: anytype,
    comptime use: *const fn (@TypeOf(context), Value) anyerror!Result,
    diagnostic: ?*Diagnostic,
) anyerror!Result {
    status.begin(diagnostic);
    return status.propagate(diagnostic, scopedViewInner(name, begin, end, Value, Result, receiver, context, use, diagnostic));
}

fn scopedViewInner(
    comptime name: []const u8,
    comptime begin: []const u8,
    comptime end: []const u8,
    comptime Value: type,
    comptime Result: type,
    receiver: anytype,
    context: anytype,
    comptime use: *const fn (@TypeOf(context), Value) anyerror!Result,
    diagnostic: ?*Diagnostic,
) anyerror!Result {
    var token: ?*anyopaque = null;
    try status.call(@field(c, begin), .{ receiver.raw, &token }, diagnostic);
    defer @field(c, end)(token);
    const Raw = @typeInfo(Param(@field(c, name), 1)).pointer.child;
    var raw: Raw = std.mem.zeroes(Raw);
    raw.size = @sizeOf(Raw);
    try status.call(@field(c, name), .{ receiver.raw, &raw }, diagnostic);
    return use(context, Value.fromNative(raw));
}

// Copy completion values before native reuses their storage.

pub const command = struct {
    pub const Value = completion.CommandCompletion;
    pub fn copy(result: *const c.mln_completion_result, _: marshal.Decode) Error!Value {
        return completion.command(result);
    }
};

pub const unit = struct {
    pub const Value = void;
    pub fn copy(result: *const c.mln_completion_result, _: marshal.Decode) Error!Value {
        return completion.unit(result);
    }
};

pub fn value(comptime T: type, comptime Raw: type) type {
    return struct {
        pub const Value = T;
        pub fn copy(result: *const c.mln_completion_result, context: marshal.Decode) Error!Value {
            return marshal.decode(T, try completion.value(Raw)(result), context);
        }
    };
}

pub fn handle(comptime T: type, comptime Raw: type) type {
    return struct {
        pub const Value = T;
        pub const output = true;
        pub fn copy(result: *const c.mln_completion_result, _: marshal.Decode) Error!Value {
            return T.adopt(try completion.value(Raw)(result));
        }
    };
}

pub fn orNull(comptime Copy: type) type {
    return struct {
        pub const Value = ?Copy.Value;
        pub fn copy(result: *const c.mln_completion_result, context: marshal.Decode) Error!Value {
            if (result.value == null) return null;
            return try Copy.copy(result, context);
        }
    };
}

pub fn slice(comptime Item: type, comptime Raw: type) type {
    return struct {
        pub const Value = marshal.OwnedValue([]const Item);
        pub fn copy(result: *const c.mln_completion_result, context: marshal.Decode) Error!Value {
            const items: ?[*]const Raw = @ptrCast(@alignCast(result.value));
            const count = result.value_count;
            if (count != 0) _ = try marshal.stridedAt(Raw, items, count, result.value_size, count - 1);
            var arena = std.heap.ArenaAllocator.init(context.allocator.?);
            errdefer arena.deinit();
            const copied = try arena.allocator().alloc(Item, count);
            for (copied, 0..) |*target, index| target.* = try marshal.decode(Item, try marshal.stridedAt(Raw, items, count, result.value_size, index), .{ .allocator = arena.allocator() });
            return .{ .arena = arena, .value = copied };
        }
    };
}

// A native build whose element grew reports a value_size wider than this
// binding's element, so the copy steps by it. A stride narrower than the
// element cannot hold one, so the copy fails.
test "an array completion is read at its value_size" {
    // Two coordinates, each followed by a member this binding does not know.
    const Wide = extern struct { point: c.mln_lat_lng, newer: f64 };
    const wide = [_]Wide{
        .{ .point = .{ .latitude = 1, .longitude = 2 }, .newer = -1 },
        .{ .point = .{ .latitude = 3, .longitude = 4 }, .newer = -1 },
    };
    var result = std.mem.zeroInit(c.mln_completion_result, .{
        .size = @sizeOf(c.mln_completion_result),
        .status = c.MLN_STATUS_OK,
        .value = @as(?*const anyopaque, &wide),
        .value_count = wide.len,
        .value_size = @sizeOf(Wide),
    });
    const Copy = slice(c.mln_lat_lng, c.mln_lat_lng);
    var points = try Copy.copy(&result, .{ .allocator = std.testing.allocator });
    defer points.deinit();
    try std.testing.expectEqual(@as(usize, 2), points.value.len);
    try std.testing.expectEqual(@as(f64, 3), points.value[1].latitude);
    try std.testing.expectEqual(@as(f64, 4), points.value[1].longitude);

    result.value_size = @sizeOf(c.mln_lat_lng) - 1;
    try std.testing.expectError(error.NativeError, Copy.copy(&result, .{ .allocator = std.testing.allocator }));
}

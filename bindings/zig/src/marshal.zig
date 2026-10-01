//! Conversions between binding values and the native values they mirror.
//!
//! Generated value types convert their own fields with these helpers, and the
//! operation runtime in call.zig decodes outputs and completion results with
//! `decode`.

const std = @import("std");

const c = @import("c.zig").raw;
const owner = @import("owner.zig");
const status = @import("status.zig");

/// A copied native value and the allocator that frees it. Move it rather than
/// copy it: deinit on two copies releases the value twice.
pub fn OwnedValue(comptime T: type) type {
    return struct {
        pub const Value = T;
        arena: std.heap.ArenaAllocator,
        value: T,
        pub fn deinit(self: *@This()) void {
            self.arena.deinit();
            self.* = undefined;
        }
    };
}

/// Whether `T` is an `OwnedValue`.
pub fn isOwned(comptime T: type) bool {
    return @typeInfo(T) == .@"struct" and @hasDecl(T, "Value") and T == OwnedValue(T.Value);
}

/// Returns the type of a callback field's function, for an optional C function
/// pointer type.
pub fn CallbackFunction(comptime T: type) type {
    return @typeInfo(@typeInfo(T).optional.child).pointer.child;
}

pub fn CallbackArg(comptime T: type, comptime index: usize) type {
    return @typeInfo(CallbackFunction(T)).@"fn".params[index].type.?;
}

pub fn CallbackResult(comptime T: type) type {
    return @typeInfo(CallbackFunction(T)).@"fn".return_type.?;
}

/// Copies `bytes` with a terminating NUL, rejecting an embedded one.
pub fn cString(allocator: std.mem.Allocator, bytes: []const u8) status.Error![*:0]const u8 {
    if (std.mem.indexOfScalar(u8, bytes, 0) != null) return error.InvalidString;
    return (try allocator.dupeZ(u8, bytes)).ptr;
}

/// Copies `value` into `allocator` and returns its address.
pub fn store(allocator: std.mem.Allocator, value: anytype) status.Error!*const @TypeOf(value) {
    const pointer = try allocator.create(@TypeOf(value));
    pointer.* = value;
    return pointer;
}

/// Borrows `bytes` as a native buffer view.
pub fn view(bytes: []const u8) c.mln_buffer_view {
    return .{ .data = bytes.ptr, .size = bytes.len };
}

/// Reads `count` native items, rejecting a null pointer or a count that does
/// not fit the address space.
pub fn nativeSlice(comptime T: type, pointer: ?[*]const T, count: usize) status.Error![]const T {
    if (count == 0) return &.{};
    if (count > std.math.maxInt(isize) / @max(1, @sizeOf(T))) return error.NativeError;
    return (pointer orelse return error.NativeError)[0..count];
}

/// Reads item `index` of a native array whose items are `stride` bytes apart.
pub fn stridedAt(comptime T: type, pointer: ?[*]const T, count: usize, stride: usize, index: usize) status.Error!T {
    if (stride < @sizeOf(T) or count > std.math.maxInt(isize) / stride or index >= count) return error.NativeError;
    const bytes: [*]const u8 = @ptrCast(pointer orelse return error.NativeError);
    return @as(*align(1) const T, @ptrCast(bytes + index * stride)).*;
}

/// Copies the text at `raw_offset` and `raw_length` inside a native arena of
/// `size` bytes, rejecting a range outside it.
pub fn copyArenaString(allocator: std.mem.Allocator, pointer: ?[*]const u8, size: usize, raw_offset: anytype, raw_length: anytype) status.Error![]const u8 {
    const offset = std.math.cast(usize, raw_offset) orelse return error.NativeError;
    const length = std.math.cast(usize, raw_length) orelse return error.NativeError;
    if (offset > size or length > size - offset) return error.NativeError;
    if (length == 0) return &.{};
    const bytes = pointer orelse return error.NativeError;
    return allocator.dupe(u8, bytes[offset..][0..length]);
}

/// Copies the bytes a native buffer view borrows.
pub fn copyView(allocator: std.mem.Allocator, raw: c.mln_buffer_view) status.Error![]const u8 {
    if (raw.size == 0) return &.{};
    if (raw.size > std.math.maxInt(isize)) return error.NativeError;
    return allocator.dupe(u8, @as([*]const u8, @ptrCast(raw.data orelse return error.NativeError))[0..raw.size]);
}

/// Converts a binding value that needs no allocator to the native value of
/// type `Target`: a generated value converts itself, and any other value
/// passes unchanged.
pub fn nativeOf(comptime Target: type, value: anytype) Target {
    const Value = @TypeOf(value);
    if (Value == Target) return value;
    switch (@typeInfo(Value)) {
        .@"struct", .@"enum", .@"union" => if (@hasDecl(Value, "toNative")) return value.toNative(),
        else => {},
    }
    return value;
}

/// Writes an optional value into a presence-masked native field. A present
/// value is converted into `field` and marks `presence`: a bool presence
/// becomes true, and a mask gains `bit`. A null value leaves both unchanged.
pub fn present(presence: anytype, comptime bit: anytype, field: anytype, value: anytype) void {
    const item = value orelse return;
    if (@TypeOf(presence.*) == bool) presence.* = true else presence.* |= bit;
    field.* = nativeOf(@TypeOf(field.*), item);
}

/// The native conversions of a generated enum, which keeps values that this
/// binding does not name.
pub fn EnumMethods(comptime T: type) type {
    const Raw = @typeInfo(T).@"enum".tag_type;
    return struct {
        pub fn fromNative(raw: Raw) T {
            return @enumFromInt(raw);
        }
        pub fn toNative(self: T) Raw {
            return @intFromEnum(self);
        }
    };
}

/// The native conversions and set operations of a generated flag set.
///
/// `T` declares one bool field per named flag, in the order of its
/// `native_bits`, and an `unknown_bits` field that keeps bits this binding does
/// not name.
pub fn FlagMethods(comptime T: type) type {
    const Raw = @FieldType(T, "unknown_bits");
    const flags = @typeInfo(T).@"struct".fields[0..T.native_bits.len];
    const known = blk: {
        var mask: Raw = 0;
        for (T.native_bits) |bit| mask |= bit;
        break :blk mask;
    };
    return struct {
        pub fn fromNative(raw: Raw) T {
            var result: T = .{ .unknown_bits = raw & ~known };
            inline for (flags, T.native_bits) |flag, bit| @field(result, flag.name) = raw & bit != 0;
            return result;
        }
        pub fn toNative(self: T) Raw {
            var raw = self.unknown_bits;
            inline for (flags, T.native_bits) |flag, bit| {
                if (@field(self, flag.name)) raw |= bit;
            }
            return raw;
        }
        pub fn contains(self: T, other: T) bool {
            return toNative(self) & toNative(other) == toNative(other);
        }
        pub fn isEmpty(self: T) bool {
            return toNative(self) == 0;
        }
        pub fn unionWith(self: T, other: T) T {
            return fromNative(toNative(self) | toNative(other));
        }
    };
}

/// What decoding a native value may need: an allocator for copied storage and
/// the owner that an adopted handle retains.
pub const Decode = struct {
    allocator: ?std.mem.Allocator = null,
    parent: ?owner.Anchor = null,

    fn alloc(self: Decode) std.mem.Allocator {
        return self.allocator orelse unreachable;
    }
};

/// Whether decoding to `T` copies into an allocator.
pub fn decodeAllocates(comptime T: type) bool {
    return switch (@typeInfo(T)) {
        .optional => |optional| decodeAllocates(optional.child),
        else => isOwned(T),
    };
}

/// Whether decoding to `T` adopts a native handle.
pub fn decodeAdopts(comptime T: type) bool {
    return switch (@typeInfo(T)) {
        .optional => |optional| decodeAdopts(optional.child),
        .@"struct" => @hasDecl(T, "native_name"),
        else => false,
    };
}

/// Converts a native value to the binding value `T`. An `OwnedValue` copies
/// into an arena of its own, a handle adopts the native handle under
/// `context.parent`, and a generated type converts itself.
pub fn decode(comptime T: type, raw: anytype, context: Decode) status.Error!T {
    const Raw = @TypeOf(raw);
    if (T == Raw) return raw;
    if (comptime isOwned(T)) {
        var arena = std.heap.ArenaAllocator.init(context.alloc());
        errdefer arena.deinit();
        const value = try decode(T.Value, raw, .{ .allocator = arena.allocator(), .parent = context.parent });
        return .{ .arena = arena, .value = value };
    }
    if (T == []const u8 and Raw == c.mln_buffer_view) return copyView(context.alloc(), raw);
    switch (@typeInfo(T)) {
        .@"struct", .@"enum", .@"union" => {
            if (@hasDecl(T, "native_name")) return T.adopt(raw, context.parent);
            if (@hasDecl(T, "fromNative")) {
                return switch (@typeInfo(@TypeOf(T.fromNative)).@"fn".params.len) {
                    1 => T.fromNative(raw),
                    else => try T.fromNative(context.alloc(), raw),
                };
            }
        },
        else => {},
    }
    return raw;
}

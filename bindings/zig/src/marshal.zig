// Conversions between binding values and the native values they mirror.

const std = @import("std");

const c = @import("c.zig").raw;
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

pub fn isOwned(comptime T: type) bool {
    return @typeInfo(T) == .@"struct" and @hasDecl(T, "Value") and T == OwnedValue(T.Value);
}

pub fn CallbackFunction(comptime T: type) type {
    return @typeInfo(@typeInfo(T).optional.child).pointer.child;
}

pub fn CallbackArg(comptime T: type, comptime index: usize) type {
    return @typeInfo(CallbackFunction(T)).@"fn".param_types[index].?;
}

pub fn CallbackResult(comptime T: type) type {
    return @typeInfo(CallbackFunction(T)).@"fn".return_type.?;
}

pub fn cString(allocator: std.mem.Allocator, bytes: []const u8) status.Error![*:0]const u8 {
    if (std.mem.indexOfScalar(u8, bytes, 0) != null) return error.InvalidString;
    return (try allocator.dupeSentinel(u8, bytes, 0)).ptr;
}

pub fn store(allocator: std.mem.Allocator, value: anytype) status.Error!*const @TypeOf(value) {
    const pointer = try allocator.create(@TypeOf(value));
    pointer.* = value;
    return pointer;
}

pub fn view(bytes: []const u8) c.mln_buffer_view {
    return .{ .data = bytes.ptr, .size = bytes.len };
}

pub fn nativeSlice(comptime T: type, pointer: ?[*]const T, count: usize) status.Error![]const T {
    if (count == 0) return &.{};
    if (count > std.math.maxInt(isize) / @max(1, @sizeOf(T))) return error.NativeError;
    return (pointer orelse return error.NativeError)[0..count];
}

pub fn stridedAt(comptime T: type, pointer: ?[*]const T, count: usize, stride: usize, index: usize) status.Error!T {
    if (stride < @sizeOf(T) or count > std.math.maxInt(isize) / stride or index >= count) return error.NativeError;
    const bytes: [*]const u8 = @ptrCast(pointer orelse return error.NativeError);
    return @as(*align(1) const T, @ptrCast(bytes + index * stride)).*;
}

pub fn copyView(allocator: std.mem.Allocator, raw: c.mln_buffer_view) status.Error![]const u8 {
    if (raw.size == 0) return &.{};
    if (raw.size > std.math.maxInt(isize)) return error.NativeError;
    return allocator.dupe(u8, @as([*]const u8, @ptrCast(raw.data orelse return error.NativeError))[0..raw.size]);
}

pub fn nativeOf(comptime Target: type, value: anytype) Target {
    const Value = @TypeOf(value);
    if (Value == Target) return value;
    switch (@typeInfo(Value)) {
        .@"struct", .@"enum", .@"union" => if (@hasDecl(Value, "toNative")) return value.toNative(),
        else => {},
    }
    return value;
}

pub fn present(presence: anytype, comptime bit: anytype, field: anytype, value: anytype) void {
    const item = value orelse return;
    presence.* |= bit;
    field.* = nativeOf(@TypeOf(field.*), item);
}

/// The native conversions of a generated enum, which keeps values that this
/// binding does not name.
pub fn EnumMethods(comptime T: type) type {
    const Raw = @typeInfo(T).@"enum".tag_type;
    return struct {
        pub fn fromNative(raw: Raw) T {
            return @fromBackingInt(raw);
        }
        pub fn toNative(self: T) Raw {
            return @backingInt(self);
        }
    };
}

/// The native conversions of a generated flag set.
///
/// `T` declares one bool field per named flag, in the order of its
/// `native_bits`, and an `unknown_bits` field that keeps bits this binding does
/// not name.
pub fn FlagMethods(comptime T: type) type {
    const Raw = @FieldType(T, "unknown_bits");
    const flags = @typeInfo(T).@"struct".field_names[0..T.native_bits.len];
    const known = blk: {
        var mask: Raw = 0;
        for (T.native_bits) |bit| mask |= bit;
        break :blk mask;
    };
    return struct {
        pub fn fromNative(raw: Raw) T {
            var result: T = .{ .unknown_bits = raw & ~known };
            inline for (flags, T.native_bits) |flag, bit| @field(result, flag) = raw & bit != 0;
            return result;
        }
        pub fn toNative(self: T) Raw {
            var raw = self.unknown_bits;
            inline for (flags, T.native_bits) |flag, bit| {
                if (@field(self, flag)) raw |= bit;
            }
            return raw;
        }
    };
}

pub const Decode = struct {
    allocator: ?std.mem.Allocator = null,

    fn alloc(self: Decode) std.mem.Allocator {
        return self.allocator orelse unreachable;
    }
};

pub fn decodeAllocates(comptime T: type) bool {
    return switch (@typeInfo(T)) {
        .optional => |optional| decodeAllocates(optional.child),
        else => isOwned(T),
    };
}

pub fn decode(comptime T: type, raw: anytype, context: Decode) status.Error!T {
    const Raw = @TypeOf(raw);
    if (T == Raw) return raw;
    if (comptime isOwned(T)) {
        var arena = std.heap.ArenaAllocator.init(context.alloc());
        errdefer arena.deinit();
        const value = try decode(T.Value, raw, .{ .allocator = arena.allocator() });
        return .{ .arena = arena, .value = value };
    }
    if (T == []const u8 and Raw == c.mln_buffer_view) return copyView(context.alloc(), raw);
    switch (@typeInfo(T)) {
        .@"struct", .@"enum", .@"union" => {
            if (@hasDecl(T, "native_name")) return T.adopt(raw);
            if (@hasDecl(T, "fromNative")) {
                return switch (@typeInfo(@TypeOf(T.fromNative)).@"fn".param_types.len) {
                    1 => T.fromNative(raw),
                    else => try T.fromNative(context.alloc(), raw),
                };
            }
        },
        else => {},
    }
    return raw;
}

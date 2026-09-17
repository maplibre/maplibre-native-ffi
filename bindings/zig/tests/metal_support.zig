const maplibre = @import("maplibre_native_ffi");

pub const AutoreleasePool = struct {
    token: *anyopaque,

    pub fn init() !AutoreleasePool {
        return .{ .token = mln_zig_test_autorelease_pool_push() orelse return error.AutoreleasePoolUnavailable };
    }

    pub fn deinit(self: AutoreleasePool) void {
        mln_zig_test_autorelease_pool_pop(self.token);
    }
};

pub const WindowLayer = extern struct {
    window: ?*anyopaque,
    layer: ?*anyopaque,

    pub fn deinit(self: *WindowLayer) void {
        mln_zig_test_destroy_window_metal_layer(self);
    }

    pub fn layerPointer(self: WindowLayer) !*anyopaque {
        return self.layer orelse error.MetalLayerUnavailable;
    }

    pub fn nextDrawableCount(self: WindowLayer) !u32 {
        return mln_zig_test_metal_layer_next_drawable_count(try self.layerPointer());
    }

    pub fn hasDevice(self: WindowLayer) !bool {
        return mln_zig_test_metal_layer_has_device(try self.layerPointer());
    }

    pub fn drawableSize(self: WindowLayer) !struct { width: u32, height: u32 } {
        var width: u32 = 0;
        var height: u32 = 0;
        if (!mln_zig_test_metal_layer_drawable_size(try self.layerPointer(), &width, &height)) {
            return error.MetalLayerUnavailable;
        }
        return .{ .width = width, .height = height };
    }
};

extern "c" fn mln_zig_test_autorelease_pool_push() ?*anyopaque;
extern "c" fn mln_zig_test_autorelease_pool_pop(pool: *anyopaque) void;
extern "c" fn mln_zig_test_create_counting_window_metal_layer(width: u32, height: u32, out_layer: *WindowLayer) bool;
extern "c" fn mln_zig_test_metal_layer_next_drawable_count(layer: *anyopaque) u32;
extern "c" fn mln_zig_test_metal_layer_has_device(layer: *anyopaque) bool;
extern "c" fn mln_zig_test_metal_layer_drawable_size(layer: *anyopaque, out_width: *u32, out_height: *u32) bool;
extern "c" fn mln_zig_test_destroy_window_metal_layer(window_layer: *WindowLayer) void;

pub fn createCountingWindowLayer(width: u32, height: u32) !WindowLayer {
    var window_layer = WindowLayer{ .window = null, .layer = null };
    if (!mln_zig_test_create_counting_window_metal_layer(width, height, &window_layer) or window_layer.layer == null) {
        return error.MetalLayerUnavailable;
    }
    return window_layer;
}

/// Caller-owned Metal texture for a borrowed-texture session.
pub const BorrowedTexture = extern struct {
    device: ?*anyopaque,
    texture: ?*anyopaque,
    width: u32,
    height: u32,

    pub fn deinit(self: *BorrowedTexture) void {
        mln_zig_test_destroy_metal_texture(self);
    }

    pub fn descriptor(self: BorrowedTexture, extent: maplibre.RenderTargetExtent) maplibre.MetalBorrowedTextureDescriptor {
        return .{
            .extent = extent,
            .physical_width = self.width,
            .physical_height = self.height,
            .texture = maplibre.NativePointer.fromPtr(self.texture.?),
        };
    }

    pub fn hasNonZeroPixel(self: BorrowedTexture) !bool {
        var found = false;
        if (!mln_zig_test_metal_texture_has_non_zero_pixel(&self, &found)) return error.MetalTextureUnavailable;
        return found;
    }
};

extern "c" fn mln_zig_test_create_metal_texture(width: u32, height: u32, out_texture: *BorrowedTexture) bool;
extern "c" fn mln_zig_test_metal_texture_has_non_zero_pixel(owned_texture: *const BorrowedTexture, out_has_non_zero: *bool) bool;
extern "c" fn mln_zig_test_destroy_metal_texture(owned_texture: *BorrowedTexture) void;

pub fn createBorrowedTexture(width: u32, height: u32) !BorrowedTexture {
    var texture = BorrowedTexture{ .device = null, .texture = null, .width = width, .height = height };
    if (!mln_zig_test_create_metal_texture(width, height, &texture) or texture.texture == null) {
        return error.MetalTextureUnavailable;
    }
    return texture;
}

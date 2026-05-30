const maplibre = @import("maplibre_native");
const diagnostics = @import("diagnostics.zig");
const types = @import("types.zig");

pub const Session = union(enum) {
    none,
    texture: maplibre.RenderSessionHandle,
    surface: maplibre.RenderSessionHandle,

    pub fn deinit(self: *Session) void {
        switch (self.*) {
            .none => {},
            .texture => |*texture| texture.close() catch {},
            .surface => |*surface| surface.close() catch {},
        }
        self.* = .none;
    }

    pub fn resize(
        self: *Session,
        viewport: types.Viewport,
        diagnostic_store: ?*const maplibre.DiagnosticStore,
    ) !void {
        switch (self.*) {
            .none => return types.AppError.TextureResizeFailed,
            .texture => |*texture| texture.resize(extent(viewport)) catch |err| {
                diagnostics.logError("texture resize failed", err, diagnostic_store);
                return types.AppError.TextureResizeFailed;
            },
            .surface => |*surface| surface.resize(extent(viewport)) catch |err| {
                diagnostics.logError("surface resize failed", err, diagnostic_store);
                return types.AppError.SurfaceResizeFailed;
            },
        }
    }

    pub fn renderUpdate(
        self: *Session,
        diagnostic_store: ?*const maplibre.DiagnosticStore,
    ) !bool {
        switch (self.*) {
            .none => return false,
            .texture => |*texture| {
                texture.renderUpdate() catch |err| switch (err) {
                    error.InvalidState => return false,
                    else => {
                        diagnostics.logError("texture render failed", err, diagnostic_store);
                        return types.AppError.TextureRenderFailed;
                    },
                };
                return true;
            },
            .surface => |*surface| {
                surface.renderUpdate() catch |err| switch (err) {
                    error.InvalidState => return false,
                    else => {
                        diagnostics.logError("surface render failed", err, diagnostic_store);
                        return types.AppError.SurfaceRenderFailed;
                    },
                };
                return true;
            },
        }
    }
};

pub fn extent(viewport: types.Viewport) maplibre.RenderTargetExtent {
    return .{
        .width = viewport.logical_width,
        .height = viewport.logical_height,
        .scale_factor = viewport.scale_factor,
    };
}

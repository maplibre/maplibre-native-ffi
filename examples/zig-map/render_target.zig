const maplibre = @import("maplibre_native_ffi");
const diagnostics = @import("diagnostics.zig");
const types = @import("types.zig");

pub const Session = union(enum) {
    none,
    texture: maplibre.RenderSessionHandle,
    surface: maplibre.RenderSessionHandle,

    pub fn deinit(self: *Session) void {
        switch (self.*) {
            .none => {},
            .texture => |*session| closeSession(session),
            .surface => |*session| closeSession(session),
        }
        self.* = .none;
    }

    pub fn resize(self: *Session, viewport: types.Viewport, diagnostic_store: ?*const maplibre.DiagnosticStore) !void {
        const session = switch (self.*) {
            .none => return types.AppError.TextureResizeFailed,
            .texture => |*value| value,
            .surface => |*value| value,
        };
        var operation = session.resizeStart(extent(viewport)) catch |err| {
            diagnostics.logError("render target resize failed", err, diagnostic_store);
            return types.AppError.TextureResizeFailed;
        };
        defer operation.release();
        try serviceUntilComplete(session, operation);
    }

    pub fn textureHandle(self: *Session) !*maplibre.RenderSessionHandle {
        return switch (self.*) {
            .texture => |*texture| texture,
            .none, .surface => types.AppError.TextureResizeFailed,
        };
    }

    pub fn surfaceHandle(self: *Session) !*maplibre.RenderSessionHandle {
        return switch (self.*) {
            .surface => |*surface| surface,
            .none, .texture => types.AppError.SurfaceAttachFailed,
        };
    }

    pub fn renderUpdate(self: *Session, diagnostic_store: ?*const maplibre.DiagnosticStore) !bool {
        const Target = struct { session: *maplibre.RenderSessionHandle, present: bool };
        const target: Target = switch (self.*) {
            .none => return false,
            .texture => |*value| .{ .session = value, .present = false },
            .surface => |*value| .{ .session = value, .present = true },
        };
        target.session.requestFrame(.{ .if_needed = true, .present = target.present }) catch |err| {
            diagnostics.logError("frame request failed", err, diagnostic_store);
            return types.AppError.TextureRenderFailed;
        };
        _ = target.session.serviceDriverWork(0) catch |err| {
            diagnostics.logError("render driver service failed", err, diagnostic_store);
            return types.AppError.TextureRenderFailed;
        };
        var batch = try target.session.drainFrameResults(0);
        defer batch.release();
        for (0..try batch.count()) |index| switch ((try batch.get(index)).disposition) {
            .rendered => return true,
            else => {},
        };
        return false;
    }
};

pub fn textureSession(attachment: maplibre.RenderSessionAttachment) !Session {
    var owned = attachment;
    errdefer {
        _ = owned.session.abandon() catch {};
        owned.session.destroy() catch {};
    }
    defer owned.operation.release();
    try serviceUntilComplete(&owned.session, owned.operation);
    return .{ .texture = owned.session };
}

pub fn surfaceSession(attachment: maplibre.RenderSessionAttachment) !Session {
    var owned = attachment;
    errdefer {
        _ = owned.session.abandon() catch {};
        owned.session.destroy() catch {};
    }
    defer owned.operation.release();
    try serviceUntilComplete(&owned.session, owned.operation);
    return .{ .surface = owned.session };
}

pub fn serviceUntilComplete(session: *maplibre.RenderSessionHandle, operation: maplibre.OperationHandle) !void {
    while (!try operation.poll()) _ = try session.serviceDriverWork(0);
    if (try operation.resultStatus() != 0) return error.NativeOperationFailed;
}

fn closeSession(session: *maplibre.RenderSessionHandle) void {
    var operation = session.detachStart() catch {
        _ = session.abandon() catch {};
        session.destroy() catch {};
        return;
    };
    defer operation.release();
    serviceUntilComplete(session, operation) catch {
        _ = session.abandon() catch {};
    };
    session.destroy() catch {};
}

pub fn extent(viewport: types.Viewport) maplibre.RenderTargetExtent {
    return .{ .width = viewport.logical_width, .height = viewport.logical_height, .scale_factor = viewport.scale_factor };
}

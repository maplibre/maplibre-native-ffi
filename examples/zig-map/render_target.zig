const std = @import("std");

const maplibre = @import("maplibre_native_ffi");
const diagnostics = @import("diagnostics.zig");
const types = @import("types.zig");

/// One frame demand's outcome: whether the session rendered the demand, and
/// whether the map asked for another frame while it rendered this one.
pub const FrameOutcome = struct {
    rendered: bool = false,
    needs_repaint: bool = false,
};

/// An attached render session plus the monotonic demand tokens that tie each
/// frame result back to the demand that produced it.
pub const Session = struct {
    pub const Target = union(enum) {
        none,
        texture: maplibre.RenderSessionHandle,
        surface: maplibre.RenderSessionHandle,
    };

    /// The one ordered submission that can be outstanding: attach, resize,
    /// target replacement, or detach.
    const Pending = struct {
        future: maplibre.Future(void),
        app_error: types.AppError,
        message: []const u8,
    };

    target: Target = .none,
    /// The map this session renders. Target replacement changes only the
    /// graphics resource, so those paths carry the extent to the map directly.
    map: ?*maplibre.MapHandle = null,
    next_token: u64 = 0,
    pending: ?Pending = null,

    pub fn deinit(self: *Session) void {
        const handle = self.nativeHandle() orelse {
            self.discardPending();
            return;
        };
        // The outstanding submission owns the completion slot the detach
        // needs, so it finishes first.
        var abandon = if (self.awaitPending()) |_| false else |_| true;
        if (!abandon) {
            if (handle.detach()) |completion| {
                self.beginPending(completion, self.detachError(), "render session detach failed");
                abandon = if (self.awaitPending()) |_| false else |_| true;
            } else |_| {
                abandon = true;
            }
        }
        if (abandon) _ = handle.abandon() catch {};
        handle.destroy() catch {};
        self.discardPending();
        self.target = .none;
    }

    pub fn nativeHandle(self: *Session) ?*maplibre.RenderSessionHandle {
        return switch (self.target) {
            .none => null,
            .texture => |*value| value,
            .surface => |*value| value,
        };
    }

    pub fn textureHandle(self: *Session) !*maplibre.RenderSessionHandle {
        return switch (self.target) {
            .texture => |*texture| texture,
            .none, .surface => types.AppError.TextureResizeFailed,
        };
    }

    pub fn surfaceHandle(self: *Session) !*maplibre.RenderSessionHandle {
        return switch (self.target) {
            .surface => |*surface| surface,
            .none, .texture => types.AppError.SurfaceAttachFailed,
        };
    }

    /// Takes ownership of the future one ordered submission returned, so the
    /// render loop can drive it instead of blocking the caller.
    pub fn beginPending(
        self: *Session,
        future: maplibre.Future(void),
        app_error: types.AppError,
        message: []const u8,
    ) void {
        std.debug.assert(self.pending == null);
        self.pending = .{ .future = future, .app_error = app_error, .message = message };
    }

    /// Services caller-driver work and reports whether the outstanding
    /// submission is still pending.
    pub fn poll(self: *Session) !bool {
        if (self.pending == null) return false;
        const session = self.nativeHandle() orelse {
            self.discardPending();
            return false;
        };
        var serviced = true;
        _ = session.serviceDriverWork(0) catch |err| {
            serviced = false;
            diagnostics.logError(self.pending.?.message, err, null);
        };
        if (serviced and !try self.pending.?.future.poll()) return true;
        var pending = self.takePending().?;
        defer pending.future.deinit();
        if (!serviced) return pending.app_error;
        pending.future.wait(null) catch |err| {
            diagnostics.logError(pending.message, err, null);
            return pending.app_error;
        };
        return false;
    }

    /// Services caller-driver work until the outstanding submission completes.
    /// Startup and shutdown block here; the render loop polls instead.
    pub fn awaitPending(self: *Session) !void {
        while (try self.poll()) {}
    }

    /// Starts the session resize that carries the new logical extent to the
    /// map. The render loop drives it to completion through `poll`.
    pub fn startResize(self: *Session, viewport: types.Viewport) !void {
        const session = self.nativeHandle() orelse return types.AppError.TextureResizeFailed;
        const app_error = self.resizeError();
        const completion = session.resize(extent(viewport)) catch |err| {
            diagnostics.logError("render target resize failed", err, null);
            return app_error;
        };
        self.beginPending(completion, app_error, "render target resize failed");
    }

    /// Carries the new logical extent to the map on the paths where the
    /// session cannot: a caller-owned texture the host sizes, and a replaced
    /// surface target. Both change only the graphics resource.
    pub fn resizeMap(self: *Session, viewport: types.Viewport) !void {
        const map = self.map orelse return self.resizeError();
        var completion = map.resize(
            viewport.logical_width,
            viewport.logical_height,
            viewport.scale_factor,
        ) catch |err| {
            diagnostics.logError("map resize failed", err, null);
            return self.resizeError();
        };
        completion.deinit();
    }

    /// Submits one frame demand, services caller-driver work, and reports the
    /// outcome of the result carrying this demand's token.
    pub fn renderUpdate(
        self: *Session,
        allocator: std.mem.Allocator,
        diagnostic_store: ?*const maplibre.DiagnosticStore,
    ) !FrameOutcome {
        const presents = self.target == .surface;
        const session = self.nativeHandle() orelse return FrameOutcome{};
        const app_error = if (presents)
            types.AppError.SurfaceRenderFailed
        else
            types.AppError.TextureRenderFailed;
        self.next_token += 1;
        const token = self.next_token;
        session.requestFrame(.{ .if_needed = true, .present = presents, .token = token }) catch |err| {
            diagnostics.logError("frame request failed", err, diagnostic_store);
            return app_error;
        };
        _ = session.serviceDriverWork(0) catch |err| {
            diagnostics.logError("render driver service failed", err, diagnostic_store);
            return app_error;
        };
        // The drain copies its results, so this loop owns them for the frame.
        var batch = try session.drainFrameResults(allocator);
        defer batch.deinit();
        for (0..batch.len()) |index| {
            const result = try batch.at(index);
            if (result.token != token) continue;
            return .{
                .rendered = result.disposition == .rendered,
                .needs_repaint = result.needs_repaint,
            };
        }
        return .{};
    }

    fn resizeError(self: *const Session) types.AppError {
        return if (self.target == .surface)
            types.AppError.SurfaceResizeFailed
        else
            types.AppError.TextureResizeFailed;
    }

    fn detachError(self: *const Session) types.AppError {
        return if (self.target == .surface)
            types.AppError.SurfaceAttachFailed
        else
            types.AppError.TextureAttachFailed;
    }

    fn takePending(self: *Session) ?Pending {
        const pending = self.pending;
        self.pending = null;
        return pending;
    }

    fn discardPending(self: *Session) void {
        if (self.takePending()) |pending| {
            var owned = pending;
            owned.future.deinit();
        }
    }
};

pub fn textureSession(map: *maplibre.MapHandle, attachment: maplibre.RenderSessionAttachment) !Session {
    return attachedSession(map, attachment, .{ .texture = attachment.session });
}

pub fn surfaceSession(map: *maplibre.MapHandle, attachment: maplibre.RenderSessionAttachment) !Session {
    return attachedSession(map, attachment, .{ .surface = attachment.session });
}

/// Services caller-driver work until the attachment resolves, abandoning the
/// session when it does not.
fn attachedSession(
    map: *maplibre.MapHandle,
    attachment: maplibre.RenderSessionAttachment,
    target: Session.Target,
) !Session {
    var owned = attachment;
    var session = Session{ .target = target, .map = map };
    errdefer {
        _ = owned.session.abandon() catch {};
        owned.session.destroy() catch {};
    }
    session.beginPending(owned.attached, session.detachError(), "render target attach failed");
    try session.awaitPending();
    return session;
}

pub fn extent(viewport: types.Viewport) maplibre.RenderTargetExtent {
    return .{ .width = viewport.logical_width, .height = viewport.logical_height, .scale_factor = viewport.scale_factor };
}

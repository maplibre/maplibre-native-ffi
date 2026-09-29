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
        texture: maplibre.RenderSession,
        surface: maplibre.RenderSession,
    };

    /// The one ordered submission that can be outstanding: attach, resize,
    /// target replacement, or detach.
    const Pending = struct {
        future: PendingFuture,
        app_error: types.AppError,
        message: []const u8,
    };

    target: Target = .none,
    /// The map this session renders. Target replacement changes only the
    /// graphics resource, so those paths carry the extent to the map directly.
    map: ?*maplibre.Map = null,
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
            if (maplibre.renderSessionDetach(handle.*, null)) |completion| {
                self.beginPending(completion, self.detachError(), "render session detach failed");
                abandon = if (self.awaitPending()) |_| false else |_| true;
            } else |_| {
                abandon = true;
            }
        }
        if (abandon) _ = maplibre.renderSessionAbandon(handle.*, null) catch {};
        handle.deinit();
        self.discardPending();
        self.target = .none;
    }

    pub fn nativeHandle(self: *Session) ?*maplibre.RenderSession {
        return switch (self.target) {
            .none => null,
            .texture => |*value| value,
            .surface => |*value| value,
        };
    }

    pub fn textureHandle(self: *Session) !*maplibre.RenderSession {
        return switch (self.target) {
            .texture => |*texture| texture,
            .none, .surface => types.AppError.TextureResizeFailed,
        };
    }

    pub fn surfaceHandle(self: *Session) !*maplibre.RenderSession {
        return switch (self.target) {
            .surface => |*surface| surface,
            .none, .texture => types.AppError.SurfaceAttachFailed,
        };
    }

    /// Takes ownership of the future one ordered submission returned, so the
    /// render loop can drive it instead of blocking the caller.
    pub fn beginPending(
        self: *Session,
        future: anytype,
        app_error: types.AppError,
        message: []const u8,
    ) void {
        std.debug.assert(self.pending == null);
        self.pending = .{ .future = PendingFuture.init(future), .app_error = app_error, .message = message };
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
        var diagnostic: maplibre.Diagnostic = .{};
        _ = maplibre.renderSessionServiceDriverWork(session.*, 0, &diagnostic) catch |err| {
            serviced = false;
            diagnostics.logError(self.pending.?.message, err, &diagnostic);
        };
        if (serviced and !try self.pending.?.future.poll()) return true;
        var pending = self.takePending().?;
        defer pending.future.deinit();
        if (!serviced) return pending.app_error;
        pending.future.wait(&diagnostic) catch |err| {
            diagnostics.logError(pending.message, err, &diagnostic);
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
        var diagnostic: maplibre.Diagnostic = .{};
        const completion = maplibre.renderSessionResize(std.heap.smp_allocator, session.*, extent(viewport), &diagnostic) catch |err| {
            diagnostics.logError("render target resize failed", err, &diagnostic);
            return app_error;
        };
        self.beginPending(completion, app_error, "render target resize failed");
    }

    /// Carries the new logical extent to the map on the paths where the
    /// session cannot: a caller-owned texture the host sizes, and a replaced
    /// surface target. Both change only the graphics resource.
    pub fn resizeMap(self: *Session, viewport: types.Viewport) !void {
        const map = self.map orelse return self.resizeError();
        var diagnostic: maplibre.Diagnostic = .{};
        var completion = maplibre.mapResize(map.*, .{ .width = viewport.logical_width, .height = viewport.logical_height, .scale_factor = viewport.scale_factor }, &diagnostic) catch |err| {
            diagnostics.logError("map resize failed", err, &diagnostic);
            return self.resizeError();
        };
        completion.deinit();
    }

    /// Submits one frame demand, services caller-driver work, and reports the
    /// outcome of the result carrying this demand's token.
    pub fn renderUpdate(
        self: *Session,
        allocator: std.mem.Allocator,
    ) !FrameOutcome {
        const presents = self.target == .surface;
        const session = self.nativeHandle() orelse return FrameOutcome{};
        const app_error = if (presents)
            types.AppError.SurfaceRenderFailed
        else
            types.AppError.TextureRenderFailed;
        self.next_token += 1;
        const token = self.next_token;
        var diagnostic: maplibre.Diagnostic = .{};
        maplibre.renderSessionRequestFrame(allocator, session.*, .{ .flags = .{ .if_needed = true, .present = presents }, .token = token }, &diagnostic) catch |err| {
            diagnostics.logError("frame request failed", err, &diagnostic);
            return app_error;
        };
        _ = maplibre.renderSessionServiceDriverWork(session.*, 0, &diagnostic) catch |err| {
            diagnostics.logError("render driver service failed", err, &diagnostic);
            return app_error;
        };
        // The batch owns the results for this frame.
        var batch = maplibre.renderSessionDrainFrameResults(session.*, null) catch |err| switch (err) {
            error.NotReady => return .{},
            else => return err,
        };
        defer batch.deinit();
        for (0..try maplibre.renderFrameBatchCount(batch, null)) |index| {
            const result = try maplibre.renderFrameBatchGet(batch, index, null);
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

pub fn textureSession(map: *maplibre.Map, attachment: anytype) !Session {
    return attachedSession(map, attachment, .{ .texture = attachment.session });
}

pub fn surfaceSession(map: *maplibre.Map, attachment: anytype) !Session {
    return attachedSession(map, attachment, .{ .surface = attachment.session });
}

/// Services caller-driver work until the attachment resolves, abandoning the
/// session when it does not.
fn attachedSession(
    map: *maplibre.Map,
    attachment: anytype,
    target: Session.Target,
) !Session {
    var owned = attachment;
    var session = Session{ .target = target, .map = map };
    errdefer {
        _ = maplibre.renderSessionAbandon(owned.session, null) catch {};
        owned.session.deinit();
    }
    session.beginPending(owned.ready, session.detachError(), "render target attach failed");
    try session.awaitPending();
    return session;
}

pub fn extent(viewport: types.Viewport) maplibre.RenderTargetExtent {
    return .{ .width = viewport.logical_width, .height = viewport.logical_height, .scale_factor = viewport.scale_factor };
}

const PendingFuture = union(enum) {
    ready: maplibre.Future(void),
    command: maplibre.Future(maplibre.CommandCompletion),

    fn init(future: anytype) PendingFuture {
        return if (@TypeOf(future) == maplibre.Future(void)) .{ .ready = future } else .{ .command = future };
    }
    fn poll(self: *PendingFuture) !bool {
        return switch (self.*) {
            inline else => |*future| future.poll(),
        };
    }
    fn wait(self: *PendingFuture, diagnostic: ?*maplibre.Diagnostic) !void {
        switch (self.*) {
            .ready => |*future| try future.wait(diagnostic),
            .command => |*future| try (try future.wait(diagnostic)).statusError(),
        }
    }
    fn deinit(self: *PendingFuture) void {
        switch (self.*) {
            inline else => |*future| future.deinit(),
        }
    }
};

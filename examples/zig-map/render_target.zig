//! The backend-agnostic slice of the render target: the attached session, its
//! frame demands and results, and the extent a viewport maps to.

const std = @import("std");

const maplibre = @import("maplibre_native_ffi");
const diagnostics = @import("diagnostics.zig");
const events = @import("events.zig");
const types = @import("types.zig");

/// What one drain of the frame-result queue found.
pub const FrameResults = struct {
    /// A demand rendered a frame.
    rendered: bool = false,
    /// The map asked for another frame while it rendered one.
    needs_repaint: bool = false,
    /// The target could not produce a frame, so the loop retries later.
    target_not_ready: bool = false,
};

/// Caller-driver attach options whose wakes post `frame_results` and
/// `driver_work` app events.
pub fn attachOptions() maplibre.RenderSessionAttachOptions {
    return .{
        .driver = .caller_graphics_thread,
        .requested_texture_ring_depth = 2,
        .frame_wake = events.wake(.frame_results),
        .driver_work_wake = events.wake(.driver_work),
    };
}

/// An attached render session plus the monotonic demand tokens that tie each
/// frame result back to the demand that produced it.
pub const Session = struct {
    handle: ?maplibre.RenderSession = null,
    /// The map this session renders. Target replacement changes only the
    /// graphics resource, so those paths carry the extent to the map directly.
    map: ?*maplibre.Map = null,
    /// Whether demands ask the driver to present, as a surface target does.
    presents: bool = false,
    next_token: u64 = 0,

    /// Services driver work until the attachment resolves, abandoning the
    /// session when it does not.
    pub fn attach(map: *maplibre.Map, attachment: anytype, presents: bool) !Session {
        var owned = attachment;
        defer owned.ready.deinit();
        const session = Session{ .handle = owned.session, .map = map, .presents = presents };
        serviceUntil(owned.session, &owned.ready) catch |err| {
            diagnostics.logError("render target attach failed", err, null);
            _ = maplibre.renderSessionAbandon(owned.session, null) catch {};
            owned.session.deinit();
            return types.AppError.AttachFailed;
        };
        return session;
    }

    /// Detaches through the driver, abandoning instead when that fails, then
    /// destroys the session.
    pub fn deinit(self: *Session) void {
        var handle = self.handle orelse return;
        self.handle = null;
        var detached = false;
        if (maplibre.renderSessionDetach(handle, null)) |future| {
            var completion = future;
            defer completion.deinit();
            detached = if (serviceUntil(handle, &completion)) |_| true else |_| false;
        } else |_| {}
        if (!detached) _ = maplibre.renderSessionAbandon(handle, null) catch {};
        handle.deinit();
    }

    /// Services every queued caller-driver item on the graphics thread.
    pub fn service(self: *Session) !void {
        var diagnostic: maplibre.Diagnostic = .{};
        _ = maplibre.renderSessionServiceDriverWork(self.handle.?, 0, &diagnostic) catch |err| {
            diagnostics.logError("render driver service failed", err, &diagnostic);
            return types.AppError.BackendDrawFailed;
        };
    }

    /// Demands a frame. A forced demand renders even without a newer map
    /// update, which a retry after an undrawn frame needs.
    pub fn requestFrame(self: *Session, force: bool) !void {
        self.next_token += 1;
        var diagnostic: maplibre.Diagnostic = .{};
        maplibre.renderSessionRequestFrame(std.heap.smp_allocator, self.handle.?, .{
            .flags = .{ .if_needed = !force, .present = self.presents },
            .token = self.next_token,
        }, &diagnostic) catch |err| {
            diagnostics.logError("frame demand failed", err, &diagnostic);
            return types.AppError.RenderFailed;
        };
    }

    /// Drains every queued frame result.
    pub fn drainResults(self: *Session) !FrameResults {
        var results: FrameResults = .{};
        var batch = maplibre.renderSessionDrainFrameResults(self.handle.?, null) catch |err| switch (err) {
            error.NotReady => return results,
            else => return err,
        };
        defer batch.deinit();
        for (0..try maplibre.renderFrameBatchCount(batch, null)) |index| {
            const result = try maplibre.renderFrameBatchGet(batch, index, null);
            // No update and size pending wait for the map's next update,
            // superseded demands have a newer one behind them, and no demand
            // carries a timeout.
            switch (result.disposition) {
                .rendered => {
                    results.rendered = true;
                    results.needs_repaint = result.needs_repaint;
                },
                .target_not_ready => results.target_not_ready = true,
                else => {},
            }
        }
        return results;
    }

    /// Starts the session resize that carries the new logical extent to the
    /// map. A later resize supersedes this one, so a live resize needs no
    /// pacing.
    pub fn resize(self: *Session, viewport: types.Viewport) !void {
        var diagnostic: maplibre.Diagnostic = .{};
        var completion = maplibre.renderSessionResize(std.heap.smp_allocator, self.handle.?, extent(viewport), &diagnostic) catch |err| {
            diagnostics.logError("render session resize failed", err, &diagnostic);
            return types.AppError.ResizeFailed;
        };
        completion.deinit();
    }

    /// Carries the new logical extent to the map on the paths where the
    /// session cannot: a caller-owned texture the host sizes, and a replaced
    /// surface target. Both change only the graphics resource.
    pub fn resizeMap(self: *Session, viewport: types.Viewport) !void {
        var diagnostic: maplibre.Diagnostic = .{};
        var completion = maplibre.mapResize(self.map.?.*, .{ .width = viewport.logical_width, .height = viewport.logical_height, .scale_factor = viewport.scale_factor }, &diagnostic) catch |err| {
            diagnostics.logError("map resize failed", err, &diagnostic);
            return types.AppError.ResizeFailed;
        };
        completion.deinit();
    }

    /// Acquires the newest rendered frame, releasing any older one unsampled.
    /// Returns null when the ring holds none.
    pub fn acquireNewest(self: *Session) !?maplibre.AcquiredFrame {
        var newest: ?maplibre.AcquiredFrame = null;
        while (true) {
            var diagnostic: maplibre.Diagnostic = .{};
            const frame = maplibre.renderSessionAcquireFrame(self.handle.?, &diagnostic) catch |err| switch (err) {
                error.NotReady => return newest,
                else => {
                    if (newest) |older| releaseFrame(older);
                    diagnostics.logError("texture acquire failed", err, &diagnostic);
                    return types.AppError.BackendDrawFailed;
                },
            };
            if (newest) |older| releaseFrame(older);
            newest = frame;
        }
    }
};

/// Services driver work until a lifecycle submission completes. Startup and
/// shutdown block here. The driver's wake goes to the SDL loop rather than to
/// this wait, so the loop yields between service calls instead.
fn serviceUntil(handle: maplibre.RenderSession, future: *maplibre.Future(void)) !void {
    var diagnostic: maplibre.Diagnostic = .{};
    while (!try future.poll()) {
        _ = maplibre.renderSessionServiceDriverWork(handle, 0, &diagnostic) catch |err| {
            diagnostics.logError("render driver service failed", err, &diagnostic);
            return err;
        };
        std.Thread.yield() catch {};
    }
    future.wait(&diagnostic) catch |err| {
        diagnostics.logError("render session lifecycle failed", err, &diagnostic);
        return err;
    };
}

/// Releases a sampled frame. The compositor waits for its GPU work before
/// returning, so CPU-complete consumer synchronization is accurate.
pub fn releaseFrame(frame: maplibre.AcquiredFrame) void {
    maplibre.acquiredFrameRelease(std.heap.smp_allocator, frame, .{ .kind = .cpu_complete }, null) catch |err| {
        diagnostics.logError("texture release failed", err, null);
    };
}

/// The caller-owned textures a borrowed-texture target hands over on resize,
/// oldest first. The session renders into a texture until its replacement
/// completes, so each outgoing texture stays alive until then.
pub fn Replacements(comptime Texture: type) type {
    return struct {
        const Self = @This();
        const Entry = struct {
            completion: maplibre.Future(void),
            texture: Texture,
        };

        entries: std.ArrayList(Entry) = .empty,

        pub fn deinit(self: *Self) void {
            self.entries.deinit(std.heap.smp_allocator);
        }

        /// Queues the texture a set_target call handed over, with that call's
        /// completion.
        pub fn push(self: *Self, completion: maplibre.Future(void), texture: Texture) !void {
            try self.entries.append(std.heap.smp_allocator, .{ .completion = completion, .texture = texture });
        }

        /// Takes the oldest replacement whose set_target completed, or null
        /// when none has. A failed replacement reports its error and stays
        /// queued: the session may still render into it or the texture before
        /// it, so neither is released before the session detaches.
        pub fn takeCompleted(self: *Self) !?Texture {
            if (self.entries.items.len == 0) return null;
            const oldest = &self.entries.items[0];
            if (!try oldest.completion.poll()) return null;
            var diagnostic: maplibre.Diagnostic = .{};
            oldest.completion.wait(&diagnostic) catch |err| {
                diagnostics.logError("texture replacement failed", err, &diagnostic);
                return types.AppError.ResizeFailed;
            };
            return self.takeOldest();
        }

        /// Takes the oldest replacement whatever its state, for teardown
        /// after the session detached.
        pub fn takeAny(self: *Self) ?Texture {
            if (self.entries.items.len == 0) return null;
            return self.takeOldest();
        }

        fn takeOldest(self: *Self) Texture {
            var entry = self.entries.orderedRemove(0);
            entry.completion.deinit();
            return entry.texture;
        }
    };
}

pub fn extent(viewport: types.Viewport) maplibre.RenderTargetExtent {
    return .{ .width = viewport.logical_width, .height = viewport.logical_height, .scale_factor = viewport.scale_factor };
}

//! The backend-agnostic slice of the render target: the attached session, its
//! frame demands and results, and the extent a viewport maps to.

const std = @import("std");

const maplibre = @import("maplibre_native_ffi");
const diagnostics = @import("diagnostics.zig");
const events = @import("events.zig");
const types = @import("types.zig");

/// What one drain of the frame-result queue found.
pub const FrameResults = struct {
    /// The drain found at least one result.
    any: bool = false,
    /// A demand rendered a frame.
    rendered: bool = false,
    /// The map asked for another frame while it rendered one.
    needs_repaint: bool = false,
    /// The target could not produce a frame, so the loop retries later.
    target_not_ready: bool = false,
};

pub fn driverLabel(driver: maplibre.RenderDriverKind) []const u8 {
    return if (driver == .core_worker) "core-worker" else "caller-graphics-thread";
}

/// Attach options for `driver` whose wakes post `frame_results` and, for a
/// caller driver, `driver_work` app events. Only an owned texture has a ring.
pub fn attachOptions(mode: types.RenderTargetMode, driver: maplibre.RenderDriverKind) maplibre.RenderSessionAttachOptions {
    return .{
        .driver = driver,
        .requested_texture_ring_depth = if (mode == .owned_texture) 2 else 0,
        .frame_wake = events.wake(.frame_results),
        .driver_work_wake = if (driver == .caller_graphics_thread) events.wake(.driver_work) else .{},
    };
}

/// An attached render session plus the monotonic demand tokens that tie each
/// frame result back to the demand that produced it.
pub const Session = struct {
    handle: ?maplibre.RenderSession = null,
    /// The map this session renders. Target replacement changes only the
    /// graphics resource, so those paths carry the extent to the map directly.
    map: ?*maplibre.Map = null,
    driver: maplibre.RenderDriverKind = .core_worker,
    /// Whether demands ask the driver to present, as a surface target does.
    presents: bool = false,
    /// Whether the session and the host take turns with one texture, as a
    /// core-worker borrowed texture does. The session owns it from a demand
    /// until its result, and the host owns it until the compositor's reads
    /// finish, so at most one demand is outstanding.
    takes_turns: bool = false,
    /// Whether a turn-taking session has a demand outstanding.
    demand_outstanding: bool = false,
    /// A demand that arrived while one was outstanding, sent once the
    /// compositor is done. A forced one renders without a newer map update.
    wanted: ?bool = null,
    next_token: u64 = 0,
    /// The newest demand token with a rendered result.
    rendered_token: u64 = 0,

    /// Awaits the attachment, servicing a caller driver meanwhile, and
    /// abandons the session when it fails.
    pub fn attach(
        map: *maplibre.Map,
        attachment: anytype,
        options: maplibre.RenderSessionAttachOptions,
        mode: types.RenderTargetMode,
    ) !Session {
        var owned = attachment;
        defer owned.ready.deinit();
        const session = Session{
            .handle = owned.session,
            .map = map,
            .driver = options.driver,
            .presents = mode == .native_surface,
            // A caller driver renders and composes on one thread, in order.
            .takes_turns = mode == .borrowed_texture and options.driver == .core_worker,
        };
        session.waitLifecycle(&owned.ready) catch |err| {
            diagnostics.logError("render target attach failed", err, null);
            abandon(owned.session);
            owned.session.deinit();
            return types.AppError.AttachFailed;
        };
        return session;
    }

    /// Detaches, abandoning instead when that fails, then destroys the
    /// session.
    pub fn deinit(self: *Session) void {
        var handle = self.handle orelse return;
        var detached = false;
        if (maplibre.renderSessionDetach(handle, null)) |future| {
            var completion = future;
            defer completion.deinit();
            detached = if (self.waitLifecycle(&completion)) |_| true else |_| false;
        } else |_| {}
        if (!detached) abandon(handle);
        handle.deinit();
        self.handle = null;
    }

    /// Services every queued caller-driver item on the graphics thread.
    pub fn service(self: *Session) !void {
        var diagnostic: maplibre.Diagnostic = .{};
        _ = maplibre.renderSessionServiceDriverWork(self.handle.?, 0, &diagnostic) catch |err| {
            diagnostics.logError("render driver service failed", err, &diagnostic);
            return types.AppError.BackendDrawFailed;
        };
    }

    /// Demands a frame and returns the token whose result shows it. A forced
    /// demand renders even without a newer map update, which a retry after a
    /// frame that missed the window needs. While a turn-taking session has a
    /// demand outstanding, the demand waits for `compositorDone`.
    pub fn requestFrame(self: *Session, force: bool) !u64 {
        if (self.demand_outstanding) {
            self.wanted = force or (self.wanted orelse false);
            return self.next_token + 1;
        }
        self.next_token += 1;
        var diagnostic: maplibre.Diagnostic = .{};
        maplibre.renderSessionRequestFrame(std.heap.smp_allocator, self.handle.?, .{
            .flags = .{ .if_needed = !force, .present = self.presents },
            .token = self.next_token,
        }, &diagnostic) catch |err| {
            diagnostics.logError("frame demand failed", err, &diagnostic);
            return types.AppError.RenderFailed;
        };
        self.demand_outstanding = self.takes_turns;
        return self.next_token;
    }

    /// Ends the host's turn with a turn-taking session's texture after a
    /// drain that found results, sending any demand that waited for it.
    pub fn compositorDone(self: *Session) !void {
        self.demand_outstanding = false;
        const force = self.wanted orelse return;
        self.wanted = null;
        _ = try self.requestFrame(force);
    }

    /// Drains every queued frame result.
    pub fn drainResults(self: *Session) !FrameResults {
        var results: FrameResults = .{};
        var batch = try maplibre.renderSessionDrainFrameResults(self.handle.?, null) orelse return results;
        defer batch.deinit();
        const count = try maplibre.renderFrameBatchCount(batch, null);
        results.any = count > 0;
        for (0..count) |index| {
            const result = try maplibre.renderFrameBatchGet(batch, index, null);
            // No update and size pending wait for the map's next update,
            // superseded demands have a newer one behind them, and no demand
            // carries a timeout.
            switch (result.disposition) {
                .rendered => {
                    results.rendered = true;
                    results.needs_repaint = result.needs_repaint;
                    self.rendered_token = @max(self.rendered_token, result.token);
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

    /// Replaces `held.*` with the newest rendered frame, releasing every
    /// older one, and reports whether it found one. The compositor waits for
    /// its GPU work before returning, so the held frame's reads are done.
    pub fn acquireNewest(self: *Session, held: *?maplibre.AcquiredFrame) !bool {
        var acquired = false;
        while (true) {
            var diagnostic: maplibre.Diagnostic = .{};
            const frame = maplibre.renderSessionAcquireFrame(self.handle.?, &diagnostic) catch |err| {
                diagnostics.logError("texture acquire failed", err, &diagnostic);
                return types.AppError.BackendDrawFailed;
            } orelse return acquired;
            releaseFrame(held);
            held.* = frame;
            acquired = true;
        }
    }

    /// Waits for a lifecycle submission. A core worker needs nothing from
    /// this thread. A caller driver completes the submission inside a service
    /// call, so startup and shutdown service it here, between driver wakes.
    fn waitLifecycle(self: Session, future: *maplibre.Future(void)) !void {
        var diagnostic: maplibre.Diagnostic = .{};
        if (self.driver == .caller_graphics_thread) {
            while (true) {
                // A wake that arrives after the clear ends the next wait at once.
                events.clearDriverWait();
                _ = maplibre.renderSessionServiceDriverWork(self.handle.?, 0, &diagnostic) catch |err| {
                    diagnostics.logError("render driver service failed", err, &diagnostic);
                    return err;
                };
                if (try future.poll()) break;
                events.waitDriver();
            }
        }
        future.wait(&diagnostic) catch |err| {
            diagnostics.logError("render session lifecycle failed", err, &diagnostic);
            return err;
        };
    }
};

var graphics_kept = std.atomic.Value(bool).init(false);

/// Whether an abandon kept graphics objects until the process exits. A kept
/// Vulkan object is a child of the host's device, and a kept swapchain of its
/// surface, so a Vulkan host then keeps those until the process exits too.
pub fn graphicsKept() bool {
    return graphics_kept.load(.acquire);
}

/// Ends a session's graphics work at once, which completes any pending
/// lifecycle submission with target loss.
fn abandon(handle: maplibre.RenderSession) void {
    var diagnostic: maplibre.Diagnostic = .{};
    const result = maplibre.renderSessionAbandon(handle, &diagnostic) catch |err| {
        // A session that already released its target reports invalid state.
        if (err != error.InvalidState) diagnostics.logError("render session abandon failed", err, &diagnostic);
        return;
    };
    if (result.quarantined_resource_count > 0) {
        graphics_kept.store(true, .release);
        std.debug.print("render session abandon kept {d} resource groups until exit\n", .{result.quarantined_resource_count});
    }
}

/// Releases a sampled frame with CPU-complete synchronization. The compositor
/// waits for its GPU work before returning, so that is accurate.
pub fn releaseFrame(frame: *?maplibre.AcquiredFrame) void {
    const owned = frame.* orelse return;
    frame.* = null;
    maplibre.acquiredFrameRelease(std.heap.smp_allocator, owned, .{ .kind = .cpu_complete }, null) catch |err| {
        diagnostics.logError("texture release failed", err, null);
    };
}

/// The caller-owned textures a borrowed-texture target hands over on resize,
/// oldest first. The session renders into a texture until its replacement
/// completes, so each outgoing texture stays alive until then. Each completion
/// posts a `target_replaced` app event when it arrives.
pub fn Replacements(comptime Texture: type) type {
    return struct {
        const Self = @This();
        const Entry = struct {
            completion: maplibre.Future(void),
            texture: Texture,
            /// Whether the completion arrived and reported its outcome.
            settled: bool = false,
            failed: bool = false,
            /// The demand whose rendered frame shows the replacement, once
            /// its set_target has completed.
            shown_token: u64 = 0,
        };

        entries: std.ArrayList(Entry) = .empty,

        pub fn deinit(self: *Self) void {
            self.entries.deinit(std.heap.smp_allocator);
        }

        /// Queues the texture a set_target call handed over, with that call's
        /// completion. On failure the caller keeps the completion.
        pub fn push(self: *Self, completion: maplibre.Future(void), texture: Texture) !void {
            try self.entries.ensureUnusedCapacity(std.heap.smp_allocator, 1);
            var owned = completion;
            try owned.notify(events.wake(.target_replaced));
            self.entries.appendAssumeCapacity(.{ .completion = owned, .texture = texture });
        }

        /// Takes the oldest replacement that a rendered frame has drawn
        /// into, or null when none has. A completed replacement holds no
        /// frame yet, so the first call that finds it demands one. A failed
        /// replacement reports its error and stays queued: the session may
        /// still render into it or the texture before it, so neither is
        /// released before the session detaches.
        pub fn takeShown(self: *Self, session: *Session) !?Texture {
            if (self.entries.items.len == 0) return null;
            const oldest = &self.entries.items[0];
            if (!oldest.settled) {
                if (!try oldest.completion.poll()) return null;
                // The completion arrived, so this wait returns at once.
                var diagnostic: maplibre.Diagnostic = .{};
                oldest.completion.wait(&diagnostic) catch |err| {
                    diagnostics.logError("texture replacement failed", err, &diagnostic);
                    oldest.failed = true;
                };
                oldest.settled = true;
            }
            if (oldest.failed) return types.AppError.ResizeFailed;
            if (oldest.shown_token == 0) oldest.shown_token = try session.requestFrame(true);
            if (session.rendered_token < oldest.shown_token) return null;
            return self.takeOldest();
        }

        /// Takes the oldest replacement whatever its state, for teardown
        /// after the session detached, which completes every replacement.
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

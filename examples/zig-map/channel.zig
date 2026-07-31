//! The entire cross-thread surface between the render loop, which owns the
//! window and the render session, and the runtime loop, which owns the runtime
//! and the map.
//!
//! Three channels, matching the map example specification: a camera-command
//! queue going one way, a render request coming back, and a map channel that
//! publishes the map once so the render loop can attach its own session.

const std = @import("std");
const maplibre = @import("maplibre_native");

/// A camera change decoded on the render loop and applied on the map's owner
/// thread.
///
/// Commands carry deltas rather than absolute targets wherever the map's
/// current camera is an input, because reading the camera and writing the new
/// one has to happen together on the thread that owns the map.
pub const CameraCommand = union(enum) {
    cancel_transitions,
    set_gesture_in_progress: struct { in_progress: bool },
    move_by: struct { dx: f64, dy: f64 },
    move_by_animated: struct { dx: f64, dy: f64, duration_ms: f64 },
    scale_by: struct { scale: f64, anchor: maplibre.ScreenPoint },
    scale_by_animated: struct { scale: f64, anchor: maplibre.ScreenPoint, duration_ms: f64 },
    pitch_by: struct { delta: f64 },
    adjust_bearing: struct { delta: f64 },
    adjust_bearing_animated: struct { delta: f64, duration_ms: f64 },
    adjust_pitch_animated: struct { delta: f64, duration_ms: f64 },
    reset_orientation: struct { duration_ms: f64 },
};

/// Ring of pending camera commands. Overflow drops the oldest, because a
/// dropped pan beats blocking the render loop on the runtime loop.
pub const CommandQueue = struct {
    pub const capacity = 256;

    lock: std.Io.Mutex = std.Io.Mutex.init,
    items: [capacity]CameraCommand = undefined,
    head: usize = 0,
    len: usize = 0,

    pub fn push(self: *CommandQueue, command: CameraCommand) void {
        std.Io.Threaded.mutexLock(&self.lock);
        defer std.Io.Threaded.mutexUnlock(&self.lock);
        if (self.len == capacity) {
            self.head = (self.head + 1) % capacity;
            self.len -= 1;
        }
        self.items[(self.head + self.len) % capacity] = command;
        self.len += 1;
    }

    /// Copies every queued command into `out` and returns the count, so the
    /// runtime loop can apply them without holding the lock.
    pub fn drain(self: *CommandQueue, out: []CameraCommand) usize {
        std.Io.Threaded.mutexLock(&self.lock);
        defer std.Io.Threaded.mutexUnlock(&self.lock);
        const count = @min(self.len, out.len);
        for (0..count) |index| {
            out[index] = self.items[(self.head + index) % capacity];
        }
        self.head = (self.head + count) % capacity;
        self.len -= count;
        return count;
    }
};

/// One-bit signal that a frame is worth drawing.
///
/// The render loop consumes before it renders and sets again when nothing was
/// rendered, so a request the runtime loop publishes during a render is not
/// lost.
pub const RenderRequest = struct {
    value: std.atomic.Value(bool) = .init(true),

    pub fn set(self: *RenderRequest) void {
        self.value.store(true, .release);
    }

    pub fn consume(self: *RenderRequest) bool {
        return self.value.swap(false, .acq_rel);
    }
};

/// Publishes the map and the runtime's wake source from the runtime loop to the
/// render loop, and carries shutdown and failure the other way.
///
/// The map handle is a plain value over a lock-guarded registry, so the render
/// loop may hold a copy. It uses that copy only to attach, which native serves
/// from any thread; every other map call stays on the runtime loop. The wake
/// source is signalled from this side to release the runtime loop's parked
/// pump.
pub const MapChannel = struct {
    lock: std.Io.Mutex = std.Io.Mutex.init,
    map: ?maplibre.MapHandle = null,
    wake: ?maplibre.WakeSourceHandle = null,
    published: std.atomic.Value(bool) = .init(false),
    shutdown: std.atomic.Value(bool) = .init(false),
    failure: ?anyerror = null,
    failed: std.atomic.Value(bool) = .init(false),

    /// Runtime loop: announces the map it just created and its wake source.
    pub fn publish(
        self: *MapChannel,
        map: maplibre.MapHandle,
        wake: maplibre.WakeSourceHandle,
    ) void {
        std.Io.Threaded.mutexLock(&self.lock);
        defer std.Io.Threaded.mutexUnlock(&self.lock);
        self.map = map;
        self.wake = wake;
        self.published.store(true, .release);
    }

    /// Render loop: releases the runtime loop's parked pump. A no-op before the
    /// runtime loop has published, when there is nothing parked yet.
    pub fn wakeRuntimeLoop(self: *MapChannel) void {
        if (!self.published.load(.acquire)) return;
        std.Io.Threaded.mutexLock(&self.lock);
        defer std.Io.Threaded.mutexUnlock(&self.lock);
        if (self.wake) |wake| wake.signal() catch {};
    }

    /// Render loop: the map to attach against, once the runtime loop has one.
    pub fn mapHandle(self: *MapChannel) ?maplibre.MapHandle {
        if (!self.published.load(.acquire)) return null;
        std.Io.Threaded.mutexLock(&self.lock);
        defer std.Io.Threaded.mutexUnlock(&self.lock);
        return self.map;
    }

    /// Render loop: asks the runtime loop to stop. Called only after the render
    /// session is closed, because the map cannot be destroyed before then.
    pub fn requestShutdown(self: *MapChannel) void {
        self.shutdown.store(true, .release);
        // Release the pump so shutdown is observed now rather than after the
        // parking bound expires.
        self.wakeRuntimeLoop();
    }

    pub fn shutdownRequested(self: *MapChannel) bool {
        return self.shutdown.load(.acquire);
    }

    /// Runtime loop: blocks until the render loop has closed its session. The
    /// map cannot be destroyed before then.
    pub fn awaitShutdown(self: *MapChannel, io: std.Io) void {
        while (!self.shutdownRequested()) {
            io.sleep(.fromMilliseconds(1), .awake) catch {};
        }
    }

    pub fn fail(self: *MapChannel, err: anyerror) void {
        std.Io.Threaded.mutexLock(&self.lock);
        defer std.Io.Threaded.mutexUnlock(&self.lock);
        if (self.failure == null) self.failure = err;
        self.failed.store(true, .release);
    }

    pub fn failureValue(self: *MapChannel) ?anyerror {
        if (!self.failed.load(.acquire)) return null;
        std.Io.Threaded.mutexLock(&self.lock);
        defer std.Io.Threaded.mutexUnlock(&self.lock);
        return self.failure;
    }
};

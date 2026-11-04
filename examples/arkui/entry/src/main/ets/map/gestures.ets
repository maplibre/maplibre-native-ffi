// Gestures. A drag pans, two fingers pinch, rotate, and pan, and a double tap
// zooms in. Touch points are in the component's virtual pixels, which are the
// map's logical pixels.

import { CameraDelta, GesturePhase, MapHandle, ScreenPoint } from '@maplibre/native-ffi';

const doubleTapMs = 300;
const tapSlop = 10;

interface Tap {
  point: ScreenPoint;
  time: number;
}

export class MapGestures {
  private readonly map: () => MapHandle | undefined;
  // The touches at the last event, by ID.
  private previous: Map<number, ScreenPoint> = new Map<number, ScreenPoint>();
  private start: ScreenPoint | undefined = undefined;
  private moved = false;
  private lastTap: Tap | undefined = undefined;

  constructor(map: () => MapHandle | undefined) {
    this.map = map;
  }

  onTouch(event: TouchEvent): void {
    switch (event.type) {
      case TouchType.Down:
        if (this.previous.size === 0) {
          this.begin(event);
        } else {
          this.previous = touches(event);
        }
        break;
      case TouchType.Move:
        this.move(event);
        break;
      case TouchType.Up:
        this.previous = touches(event);
        if (this.previous.size === 0) {
          this.end(event);
        }
        break;
      case TouchType.Cancel:
        this.previous.clear();
        this.apply({ gesturePhase: GesturePhase.cancel });
        break;
      default:
        break;
    }
  }

  private begin(event: TouchEvent): void {
    this.previous = touches(event);
    this.start = this.previous.values().next().value;
    this.moved = false;
    this.apply({ gesturePhase: GesturePhase.begin });
  }

  private move(event: TouchEvent): void {
    const current = touches(event);
    const befores: ScreenPoint[] = [];
    const afters: ScreenPoint[] = [];
    current.forEach((point: ScreenPoint, id: number) => {
      const before = this.previous.get(id);
      if (before !== undefined) {
        befores.push(before);
        afters.push(point);
      }
    });
    this.previous = current;
    if (befores.length === 0) {
      return;
    }
    const before = befores[0];
    const after = afters[0];
    if (this.start !== undefined && distance(this.start, after) > tapSlop) {
      this.moved = true;
    }

    if (befores.length === 1) {
      this.apply({
        offset: { x: after.x - before.x, y: after.y - before.y },
        gesturePhase: GesturePhase.update,
      });
      return;
    }
    // The span between the touches scales and turns about their midpoint,
    // which moves with them.
    this.moved = true;
    const otherBefore = befores[1];
    const otherAfter = afters[1];
    const midBefore = midpoint(before, otherBefore);
    const midAfter = midpoint(after, otherAfter);
    const spanBefore: ScreenPoint = { x: otherBefore.x - before.x, y: otherBefore.y - before.y };
    const spanAfter: ScreenPoint = { x: otherAfter.x - after.x, y: otherAfter.y - after.y };
    const scale = Math.hypot(spanAfter.x, spanAfter.y) / Math.hypot(spanBefore.x, spanBefore.y);
    const turn = (Math.atan2(spanAfter.y, spanAfter.x) - Math.atan2(spanBefore.y, spanBefore.x)) *
      (180 / Math.PI);
    this.apply({
      offset: { x: midAfter.x - midBefore.x, y: midAfter.y - midBefore.y },
      scale: Number.isFinite(scale) && scale > 0 ? scale : 1,
      bearing: -wrapDegrees(turn),
      anchor: midAfter,
      gesturePhase: GesturePhase.update,
    });
  }

  private end(event: TouchEvent): void {
    this.apply({ gesturePhase: GesturePhase.end });
    if (this.moved || event.changedTouches.length === 0) {
      return;
    }
    const touch = event.changedTouches[0];
    const point: ScreenPoint = { x: touch.x, y: touch.y };
    // The event's timestamp is in nanoseconds, from when the finger lifted.
    const now = event.timestamp / 1e6;
    if (this.lastTap !== undefined && now - this.lastTap.time < doubleTapMs &&
      distance(this.lastTap.point, point) < tapSlop * 3) {
      this.lastTap = undefined;
      this.apply({ scale: 2, anchor: point, animation: { durationMs: 300 } });
    } else {
      this.lastTap = { point: point, time: now };
    }
  }

  private apply(delta: CameraDelta): void {
    this.map()?.discarding.applyCameraDelta(delta);
  }
}

// The fingers still down after the event, by ID.
function touches(event: TouchEvent): Map<number, ScreenPoint> {
  const lifted = new Set<number>();
  if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
    for (const touch of event.changedTouches) {
      lifted.add(touch.id);
    }
  }
  const points = new Map<number, ScreenPoint>();
  for (const touch of event.touches) {
    if (!lifted.has(touch.id)) {
      points.set(touch.id, { x: touch.x, y: touch.y });
    }
  }
  return points;
}

function midpoint(a: ScreenPoint, b: ScreenPoint): ScreenPoint {
  return { x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 };
}

function distance(a: ScreenPoint, b: ScreenPoint): number {
  return Math.hypot(a.x - b.x, a.y - b.y);
}

// An angle in degrees, wrapped to [-180, 180).
function wrapDegrees(degrees: number): number {
  return (((degrees + 180) % 360) + 360) % 360 - 180;
}

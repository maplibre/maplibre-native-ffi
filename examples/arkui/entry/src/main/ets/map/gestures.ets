// Gestures. A drag pans, two fingers pinch, rotate, and pan, and a double tap
// zooms in. The map keeps the fingers' speed when they lift, and a touch stops
// it. Gesture events are in the component's virtual pixels, which are the
// map's logical pixels.

import { CameraDelta, GestureMomentum, GesturePhase, MapHandle, ScreenPoint } from '@maplibre/native-ffi';

// How far back a pinch or rotation's speed at release looks. ArkUI reports the
// speed of a pan, but not of a pinch or rotation.
const releaseWindowMs = 100;

interface Sample {
  time: number;
  value: number;
}

// How fast a value changed, from its recent samples.
class Rate {
  private samples: Sample[] = [];

  clear(): void {
    this.samples = [];
  }

  add(time: number, value: number): void {
    this.samples = this.recent(time);
    this.samples.push({ time: time, value: value });
  }

  // Per second, over the window before `time`.
  at(time: number): number | undefined {
    const recent = this.recent(time);
    if (recent.length < 2) {
      return undefined;
    }
    const first = recent[0];
    const last = recent[recent.length - 1];
    return last.time > first.time ? (last.value - first.value) / ((last.time - first.time) / 1000) : undefined;
  }

  private recent(time: number): Sample[] {
    return this.samples.filter((sample: Sample) => time - sample.time <= releaseWindowMs);
  }
}

export class MapGestures {
  private readonly map: () => MapHandle | undefined;
  private active = false;
  private panning = false;
  // The pan's offset, the pinch's scale, and the rotation's angle at their
  // last events.
  private offset: ScreenPoint = { x: 0, y: 0 };
  private scale = 1;
  private angle = 0;
  // The zoom levels and degrees of bearing that the gesture has added.
  private zoom = 0;
  private bearing = 0;
  private zoomRate = new Rate();
  private bearingRate = new Rate();
  private center: ScreenPoint | undefined = undefined;

  constructor(map: () => MapHandle | undefined) {
    this.map = map;
  }

  // The first finger down begins the gesture, which stops any glide. The last
  // finger up ends it, unless a pan ends it with its velocity.
  onTouch(event: TouchEvent): void {
    switch (event.type) {
      case TouchType.Down:
        if (!this.active) {
          this.begin();
        }
        break;
      case TouchType.Up:
        if (!this.panning && allLifted(event)) {
          this.end(event.timestamp, undefined);
        }
        break;
      case TouchType.Cancel:
        this.cancel();
        break;
      default:
        break;
    }
  }

  panStart(event: GestureEvent): void {
    this.panning = true;
    this.offset = { x: 0, y: 0 };
    this.pan(event);
  }

  pan(event: GestureEvent): void {
    this.apply({
      offset: { x: event.offsetX - this.offset.x, y: event.offsetY - this.offset.y },
      gesturePhase: GesturePhase.update,
    });
    this.offset = { x: event.offsetX, y: event.offsetY };
  }

  // The content keeps the fingers' velocity.
  panEnd(event: GestureEvent): void {
    this.panning = false;
    this.end(event.timestamp, { x: event.velocityX, y: event.velocityY });
  }

  pinchStart(event: GestureEvent): void {
    this.scale = 1;
    this.pinch(event);
  }

  pinch(event: GestureEvent): void {
    this.center = { x: event.pinchCenterX, y: event.pinchCenterY };
    this.zoom += Math.log2(event.scale / this.scale);
    this.zoomRate.add(event.timestamp / 1e6, this.zoom);
    this.apply({ scale: event.scale / this.scale, anchor: this.center, gesturePhase: GesturePhase.update });
    this.scale = event.scale;
  }

  rotateStart(event: GestureEvent): void {
    this.angle = 0;
    this.rotate(event);
  }

  // The content turns with the fingers, so the bearing turns the other way.
  rotate(event: GestureEvent): void {
    this.center = centroid(event);
    const turn = -wrapDegrees(event.angle - this.angle);
    this.bearing += turn;
    this.bearingRate.add(event.timestamp / 1e6, this.bearing);
    this.apply({ bearing: turn, anchor: this.center, gesturePhase: GesturePhase.update });
    this.angle = event.angle;
  }

  doubleTap(event: GestureEvent): void {
    const finger = event.fingerList[0];
    this.apply({ scale: 2, anchor: { x: finger.localX, y: finger.localY }, animation: { durationMs: 300 } });
  }

  cancel(): void {
    this.panning = false;
    if (this.active) {
      this.active = false;
      this.apply({ gesturePhase: GesturePhase.cancel });
    }
  }

  private begin(): void {
    this.active = true;
    this.zoom = 0;
    this.bearing = 0;
    this.zoomRate.clear();
    this.bearingRate.clear();
    this.apply({ gesturePhase: GesturePhase.begin });
  }

  // The event's timestamp is in nanoseconds.
  private end(timestamp: number, pan: ScreenPoint | undefined): void {
    if (!this.active) {
      return;
    }
    this.active = false;
    const momentum: GestureMomentum = {};
    if (pan !== undefined) {
      momentum.pan = { velocity: pan };
    }
    const zoom = this.zoomRate.at(timestamp / 1e6);
    if (zoom !== undefined) {
      momentum.zoom = { velocity: zoom, anchor: this.center };
    }
    const bearing = this.bearingRate.at(timestamp / 1e6);
    if (bearing !== undefined) {
      momentum.bearing = { velocity: bearing, anchor: this.center };
    }
    this.apply({ momentum: momentum, gesturePhase: GesturePhase.end });
  }

  private apply(delta: CameraDelta): void {
    this.map()?.discarding.applyCameraDelta(delta);
  }
}

// Whether the event lifts every finger that was down.
function allLifted(event: TouchEvent): boolean {
  return event.touches.every((touch: TouchObject) =>
    event.changedTouches.some((lifted: TouchObject) => lifted.id === touch.id));
}

function centroid(event: GestureEvent): ScreenPoint {
  let x = 0;
  let y = 0;
  event.fingerList.forEach((finger: FingerInfo) => {
    x += finger.localX;
    y += finger.localY;
  });
  return { x: x / event.fingerList.length, y: y / event.fingerList.length };
}

// An angle in degrees, wrapped to [-180, 180).
function wrapDegrees(degrees: number): number {
  return (((degrees + 180) % 360) + 360) % 360 - 180;
}

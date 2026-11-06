// Gestures. A drag pans, two fingers pinch, rotate, and pan, and a double tap
// zooms in. The map keeps moving after a drag or a pinch, until a touch stops
// it.

import {
  type CameraDelta,
  type GestureMomentum,
  GesturePhase,
  type MapHandle,
  type ScreenPoint,
} from "@maplibre/native-ffi-react-native";
import {
  type GestureResponderEvent,
  PanResponder,
  type PanResponderGestureState,
  type PanResponderInstance,
} from "react-native";

const doubleTapMs = 300;
const tapSlop = 10;
// PanResponder keeps the last move's velocity while a touch rests, so a release
// this long after the last move starts no momentum.
const releaseWindowMs = 100;

// How far a pinch has zoomed and turned since the gesture began.
interface PinchSample {
  readonly time: number;
  readonly zoom: number;
  readonly bearing: number;
  readonly anchor: ScreenPoint;
}

export function mapGestures(
  map: () => MapHandle | undefined,
): PanResponderInstance {
  // The touches at the last event, by identifier, in the view's coordinates.
  let previous = new Map<number, ScreenPoint>();
  let start: ScreenPoint | undefined;
  let moved = false;
  let lastMove = 0;
  // The pinch's recent samples, oldest first.
  let pinch: PinchSample[] = [];
  let lastTap: { point: ScreenPoint; time: number } | undefined;

  const apply = (delta: CameraDelta) =>
    map()?.discarding.applyCameraDelta(delta);

  const touches = (event: GestureResponderEvent) =>
    new Map(
      event.nativeEvent.touches.map((touch): [number, ScreenPoint] => [
        touch.identifier,
        { x: touch.locationX, y: touch.locationY },
      ]),
    );

  // PanResponder reports how fast the touches moved in pixels per
  // millisecond, and the pinch's rates come from its samples.
  const momentum = (
    time: number,
    gesture: PanResponderGestureState,
  ): GestureMomentum => {
    if (time - lastMove > releaseWindowMs) return {};
    const result: GestureMomentum = {
      pan: { velocity: { x: gesture.vx * 1000, y: gesture.vy * 1000 } },
    };
    const first = pinch[0];
    const last = pinch.at(-1);
    if (
      first === undefined ||
      last === undefined ||
      last.time === first.time ||
      time - last.time > releaseWindowMs
    ) {
      return result;
    }
    const seconds = (last.time - first.time) / 1000;
    result.zoom = {
      velocity: (last.zoom - first.zoom) / seconds,
      anchor: last.anchor,
    };
    result.bearing = {
      velocity: (last.bearing - first.bearing) / seconds,
      anchor: last.anchor,
    };
    return result;
  };

  return PanResponder.create({
    onStartShouldSetPanResponder: () => true,
    onMoveShouldSetPanResponder: () => true,
    onPanResponderTerminationRequest: () => false,

    // Beginning a gesture also stops the last one's momentum.
    onPanResponderGrant: (event) => {
      previous = touches(event);
      start = previous.values().next().value;
      moved = false;
      pinch = [];
      apply({ gesturePhase: GesturePhase.begin });
    },

    onPanResponderMove: (event) => {
      const time = event.nativeEvent.timestamp;
      const current = touches(event);
      const pairs = [...current]
        .filter(([id]) => previous.has(id))
        .map(([id, point]) => [previous.get(id)!, point] as const);
      previous = current;
      const [first, second] = pairs;
      if (first === undefined) return;
      lastMove = time;
      const [before, after] = first;
      if (start !== undefined && distance(start, after) > tapSlop) moved = true;

      if (second === undefined) {
        apply({
          offset: { x: after.x - before.x, y: after.y - before.y },
          gesturePhase: GesturePhase.update,
        });
        return;
      }
      // The span between the touches scales and turns about their midpoint,
      // which moves with them.
      moved = true;
      const [otherBefore, otherAfter] = second;
      const midBefore = midpoint(before, otherBefore);
      const midAfter = midpoint(after, otherAfter);
      const spanBefore = {
        x: otherBefore.x - before.x,
        y: otherBefore.y - before.y,
      };
      const spanAfter = {
        x: otherAfter.x - after.x,
        y: otherAfter.y - after.y,
      };
      const ratio =
        Math.hypot(spanAfter.x, spanAfter.y) /
        Math.hypot(spanBefore.x, spanBefore.y);
      const scale = Number.isFinite(ratio) && ratio > 0 ? ratio : 1;
      const bearing = -wrapDegrees(
        (Math.atan2(spanAfter.y, spanAfter.x) -
          Math.atan2(spanBefore.y, spanBefore.x)) *
          (180 / Math.PI),
      );
      apply({
        offset: { x: midAfter.x - midBefore.x, y: midAfter.y - midBefore.y },
        scale,
        bearing,
        anchor: midAfter,
        gesturePhase: GesturePhase.update,
      });
      const last = pinch.at(-1);
      pinch = pinch.filter((sample) => time - sample.time <= releaseWindowMs);
      pinch.push({
        time,
        zoom: (last?.zoom ?? 0) + Math.log2(scale),
        bearing: (last?.bearing ?? 0) + bearing,
        anchor: midAfter,
      });
    },

    onPanResponderRelease: (event, gesture) => {
      const time = event.nativeEvent.timestamp;
      if (moved) {
        apply({
          momentum: momentum(time, gesture),
          gesturePhase: GesturePhase.end,
        });
        return;
      }
      apply({ gesturePhase: GesturePhase.end });
      const point = {
        x: event.nativeEvent.locationX,
        y: event.nativeEvent.locationY,
      };
      if (
        lastTap !== undefined &&
        time - lastTap.time < doubleTapMs &&
        distance(lastTap.point, point) < tapSlop * 3
      ) {
        lastTap = undefined;
        apply({ scale: 2, anchor: point, animation: { durationMs: 300 } });
      } else {
        lastTap = { point, time };
      }
    },

    onPanResponderTerminate: () => {
      apply({ gesturePhase: GesturePhase.end });
    },
  });
}

function midpoint(a: ScreenPoint, b: ScreenPoint): ScreenPoint {
  return { x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 };
}

function distance(a: ScreenPoint, b: ScreenPoint): number {
  return Math.hypot(a.x - b.x, a.y - b.y);
}

/** An angle in degrees, wrapped to [-180, 180). */
function wrapDegrees(degrees: number): number {
  return ((((degrees + 180) % 360) + 360) % 360) - 180;
}

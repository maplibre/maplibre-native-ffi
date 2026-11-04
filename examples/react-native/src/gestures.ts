// Gestures. A drag pans, two fingers pinch, rotate, and pan, and a double tap
// zooms in.

import {
  type CameraDelta,
  GesturePhase,
  type MapHandle,
  type ScreenPoint,
} from "@maplibre/native-ffi-react-native";
import {
  type GestureResponderEvent,
  PanResponder,
  type PanResponderInstance,
} from "react-native";

const doubleTapMs = 300;
const tapSlop = 10;

export function mapGestures(
  map: () => MapHandle | undefined,
): PanResponderInstance {
  // The touches at the last event, by identifier, in the view's coordinates.
  let previous = new Map<number, ScreenPoint>();
  let start: ScreenPoint | undefined;
  let moved = false;
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

  return PanResponder.create({
    onStartShouldSetPanResponder: () => true,
    onMoveShouldSetPanResponder: () => true,
    onPanResponderTerminationRequest: () => false,

    onPanResponderGrant: (event) => {
      previous = touches(event);
      start = previous.values().next().value;
      moved = false;
      apply({ gesturePhase: GesturePhase.begin });
    },

    onPanResponderMove: (event) => {
      const current = touches(event);
      const pairs = [...current]
        .filter(([id]) => previous.has(id))
        .map(([id, point]) => [previous.get(id)!, point] as const);
      previous = current;
      const [first, second] = pairs;
      if (first === undefined) return;
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
      const scale =
        Math.hypot(spanAfter.x, spanAfter.y) /
        Math.hypot(spanBefore.x, spanBefore.y);
      const turn =
        (Math.atan2(spanAfter.y, spanAfter.x) -
          Math.atan2(spanBefore.y, spanBefore.x)) *
        (180 / Math.PI);
      apply({
        offset: { x: midAfter.x - midBefore.x, y: midAfter.y - midBefore.y },
        scale: Number.isFinite(scale) && scale > 0 ? scale : 1,
        bearing: -wrapDegrees(turn),
        anchor: midAfter,
        gesturePhase: GesturePhase.update,
      });
    },

    onPanResponderRelease: (event) => {
      apply({ gesturePhase: GesturePhase.end });
      const point = {
        x: event.nativeEvent.locationX,
        y: event.nativeEvent.locationY,
      };
      if (moved) return;
      const now = Date.now();
      if (
        lastTap !== undefined &&
        now - lastTap.time < doubleTapMs &&
        distance(lastTap.point, point) < tapSlop * 3
      ) {
        lastTap = undefined;
        apply({ scale: 2, anchor: point, animation: { durationMs: 300 } });
      } else {
        lastTap = { point, time: now };
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

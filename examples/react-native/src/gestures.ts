// Gestures. The view's touches go to the map's gesture recognizer, which moves
// the camera: a drag pans, two fingers pinch, rotate, and pitch, and a double
// tap zooms in. A flick keeps the map gliding.

import {
  type GestureInput,
  GestureNextTick,
  type GestureRecognizerHandle,
  type GestureResult,
  PointerKind,
} from "@maplibre/native-ffi-react-native";
import type { GestureResponderEvent, ViewProps } from "react-native";

// React Native has the monotonic performance.now(), which its types leave out.
declare const performance: { now(): number };

type TouchInput = "pointerDown" | "pointerMove" | "pointerUp" | "pointerCancel";

/** The view's touch handlers, which pass the touches to `recognizer`. */
export function mapGestures(
  recognizer: () => GestureRecognizerHandle | undefined,
): ViewProps {
  // The recognizer's next tick: each frame while the camera glides or eases,
  // or at a time, such as when a press becomes a long press.
  let tickTimer: ReturnType<typeof setTimeout> | undefined;
  let tickOnFrame = false;

  const scheduleTick = ({ nextTick, nextTickMs }: GestureResult) => {
    clearTimeout(tickTimer);
    if (nextTick === GestureNextTick.frame && !tickOnFrame) {
      tickOnFrame = true;
      requestAnimationFrame(() => {
        tickOnFrame = false;
        tick();
      });
    } else if (nextTick === GestureNextTick.at) {
      tickTimer = setTimeout(tick, nextTickMs - performance.now());
    }
  };

  const tick = () => {
    const result = recognizer()?.tick(performance.now());
    if (result !== undefined) scheduleTick(result);
  };

  // Each input takes the time that it arrives, on the clock of the ticks.
  const handle = (kind: TouchInput) => (event: GestureResponderEvent) => {
    const current = recognizer();
    if (current === undefined) return;
    const time = performance.now();
    for (const touch of event.nativeEvent.changedTouches) {
      const pointer = {
        id: Number(touch.identifier),
        position: { x: touch.locationX, y: touch.locationY },
      };
      const input: GestureInput =
        kind === "pointerDown"
          ? { kind, value: { ...pointer, kind: PointerKind.touch } }
          : { kind, value: pointer };
      scheduleTick(current.handle(input, time));
    }
  };

  return {
    onTouchStart: handle("pointerDown"),
    onTouchMove: handle("pointerMove"),
    onTouchEnd: handle("pointerUp"),
    onTouchCancel: handle("pointerCancel"),
  };
}

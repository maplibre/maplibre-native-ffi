// Gestures. The component's touches go to the map's gesture recognizer, which
// moves the camera: a drag pans, two fingers pinch, rotate, and pitch, and a
// double tap zooms in. A flick keeps the map gliding. Touch events are in the
// component's virtual pixels, which are the map's logical pixels.

import {
  GestureInput,
  GestureNextTick,
  GestureRecognizerHandle,
  GestureResult,
  PointerKind,
  ScreenPoint,
} from '@maplibre/native-ffi';
import { FrameCallback, UIContext } from '@ohos.arkui.UIContext';
import systemDateTime from '@ohos.systemDateTime';

// Milliseconds since the system started, the clock of every input and tick.
function now(): number {
  return systemDateTime.getUptime(systemDateTime.TimeType.ACTIVE, true) / 1e6;
}

// Ticks the recognizer at the display's next frame.
class FrameTick extends FrameCallback {
  private readonly gestures: MapGestures;

  constructor(gestures: MapGestures) {
    super();
    this.gestures = gestures;
  }

  onFrame(frameTimeInNano: number): void {
    this.gestures.onFrame();
  }
}

export class MapGestures {
  private readonly recognizer: () => GestureRecognizerHandle | undefined;
  private readonly context: UIContext;
  // The recognizer's next tick: each frame while the camera glides or eases,
  // or at a time, such as when a press becomes a long press.
  private tickTimer: number | undefined = undefined;
  private tickOnFrame = false;

  constructor(recognizer: () => GestureRecognizerHandle | undefined, context: UIContext) {
    this.recognizer = recognizer;
    this.context = context;
  }

  // Each input takes the time that it arrives, on the clock of the ticks.
  onTouch(event: TouchEvent): void {
    const recognizer = this.recognizer();
    if (recognizer === undefined) {
      return;
    }
    const time = now();
    const kind = event.source === SourceType.Mouse ? PointerKind.mouse : PointerKind.touch;
    for (const touch of event.changedTouches) {
      const position: ScreenPoint = { x: touch.x, y: touch.y };
      let input: GestureInput;
      if (touch.type === TouchType.Down) {
        input = { kind: 'pointerDown', value: { id: touch.id, kind: kind, position: position } };
      } else if (touch.type === TouchType.Move) {
        input = { kind: 'pointerMove', value: { id: touch.id, position: position } };
      } else if (touch.type === TouchType.Up) {
        input = { kind: 'pointerUp', value: { id: touch.id, position: position } };
      } else if (touch.type === TouchType.Cancel) {
        input = { kind: 'pointerCancel', value: { id: touch.id } };
      } else {
        continue;
      }
      this.scheduleTick(recognizer.handle(input, time));
    }
  }

  onFrame(): void {
    this.tickOnFrame = false;
    this.tick();
  }

  private tick(): void {
    const recognizer = this.recognizer();
    if (recognizer !== undefined) {
      this.scheduleTick(recognizer.tick(now()));
    }
  }

  private scheduleTick(result: GestureResult): void {
    clearTimeout(this.tickTimer);
    if (result.nextTick === GestureNextTick.frame && !this.tickOnFrame) {
      this.tickOnFrame = true;
      this.context.postFrameCallback(new FrameTick(this));
    } else if (result.nextTick === GestureNextTick.at) {
      this.tickTimer = setTimeout(() => this.tick(), result.nextTickMs - now());
    }
  }
}

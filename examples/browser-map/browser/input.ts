import type { BrowserMapModule, Viewport } from "./types";

interface DragState {
  pointerId: number;
  x: number;
  y: number;
  mode: "pan" | "rotate";
  buttonMask: number;
}

export class InputController {
  private drag: DragState | null = null;

  constructor(
    private readonly canvas: HTMLCanvasElement,
    private readonly getModule: () => BrowserMapModule | null,
    private readonly getViewport: () => Viewport,
  ) {}

  attach(): void {
    this.canvas.addEventListener("contextmenu", (event) => {
      event.preventDefault();
    });
    this.canvas.addEventListener("pointerdown", (event) => {
      this.pointerDown(event);
    });
    this.canvas.addEventListener("pointermove", (event) => {
      this.pointerMove(event);
    });
    this.canvas.addEventListener("pointerup", (event) => {
      this.endDrag(event);
    });
    this.canvas.addEventListener("pointercancel", (event) => {
      this.endDrag(event);
    });
    this.canvas.addEventListener("lostpointercapture", (event) => {
      this.endDrag(event);
    });
    this.canvas.addEventListener("wheel", (event) => this.wheel(event), {
      passive: false,
    });
    window.addEventListener("blur", () => this.clearDrag());
    window.addEventListener("keydown", (event) => this.keydown(event));
  }

  private pointerDown(event: PointerEvent): void {
    if (event.button !== 0 && event.button !== 2) return;
    this.canvas.focus();
    this.canvas.setPointerCapture(event.pointerId);
    this.getModule()?._mln_browser_map_cancel_transitions();
    this.drag = {
      pointerId: event.pointerId,
      x: event.clientX,
      y: event.clientY,
      buttonMask: pointerButtonMask(event.button),
      mode:
        event.button === 2 || (event.button === 0 && event.ctrlKey)
          ? "rotate"
          : "pan",
    };
    event.preventDefault();
  }

  private pointerMove(event: PointerEvent): void {
    const module = this.getModule();
    if (!this.drag || this.drag.pointerId !== event.pointerId || !module) {
      return;
    }
    if ((event.buttons & this.drag.buttonMask) === 0) {
      this.endDrag(event);
      return;
    }
    const delta = this.logicalDelta(this.drag, event);
    if (this.drag.mode === "pan") {
      module._mln_browser_map_move_by(delta.x, delta.y);
    } else {
      module._mln_browser_map_rotate_pitch_by(delta.x * 0.5, -delta.y * 0.5);
    }
    this.drag.x = event.clientX;
    this.drag.y = event.clientY;
    event.preventDefault();
  }

  private endDrag(event: PointerEvent): void {
    if (!this.drag || this.drag.pointerId !== event.pointerId) return;
    this.releasePointerCapture(this.drag.pointerId);
    this.drag = null;
  }

  private clearDrag(): void {
    if (!this.drag) return;
    this.releasePointerCapture(this.drag.pointerId);
    this.drag = null;
  }

  private releasePointerCapture(pointerId: number): void {
    if (!this.canvas.hasPointerCapture(pointerId)) return;
    this.canvas.releasePointerCapture(pointerId);
  }

  private wheel(event: WheelEvent): void {
    const module = this.getModule();
    if (!module) return;
    const divisor = event.deltaMode === WheelEvent.DOM_DELTA_PIXEL ? 100 : 3;
    const delta = -event.deltaY / divisor;
    const scale = Math.pow(2, delta * 0.25);
    const point = this.logicalPoint(event);
    module._mln_browser_map_scale_by(scale, point.x, point.y);
    event.preventDefault();
  }

  private keydown(event: KeyboardEvent): void {
    const module = this.getModule();
    if (!module) return;
    const viewport = this.getViewport();
    const centerX = viewport.width / 2;
    const centerY = viewport.height / 2;
    const pan = 120;
    switch (event.key) {
      case "ArrowLeft":
      case "a":
      case "A":
        module._mln_browser_map_move_by(-pan, 0);
        break;
      case "ArrowRight":
      case "d":
      case "D":
        module._mln_browser_map_move_by(pan, 0);
        break;
      case "ArrowUp":
      case "w":
      case "W":
        module._mln_browser_map_move_by(0, -pan);
        break;
      case "ArrowDown":
      case "s":
      case "S":
        module._mln_browser_map_move_by(0, pan);
        break;
      case "+":
      case "=":
        module._mln_browser_map_scale_by(1.25, centerX, centerY);
        break;
      case "-":
      case "_":
        module._mln_browser_map_scale_by(1 / 1.25, centerX, centerY);
        break;
      case "q":
      case "Q":
        module._mln_browser_map_rotate_by(-10);
        break;
      case "e":
      case "E":
        module._mln_browser_map_rotate_by(10);
        break;
      case "]":
        module._mln_browser_map_pitch_by(5);
        break;
      case "[":
        module._mln_browser_map_pitch_by(-5);
        break;
      case "0":
        module._mln_browser_map_reset_orientation();
        break;
      default:
        return;
    }
    event.preventDefault();
  }

  private logicalPoint(event: MouseEvent): { x: number; y: number } {
    const rect = this.canvas.getBoundingClientRect();
    const viewport = this.getViewport();
    return {
      x: ((event.clientX - rect.left) / rect.width) * viewport.width,
      y: ((event.clientY - rect.top) / rect.height) * viewport.height,
    };
  }

  private logicalDelta(
    previous: { x: number; y: number },
    event: MouseEvent,
  ): { x: number; y: number } {
    const rect = this.canvas.getBoundingClientRect();
    const viewport = this.getViewport();
    return {
      x: ((event.clientX - previous.x) / rect.width) * viewport.width,
      y: ((event.clientY - previous.y) / rect.height) * viewport.height,
    };
  }
}

function pointerButtonMask(button: number): number {
  return button === 2 ? 2 : 1;
}

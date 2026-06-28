import type { BrowserMapModule, Viewport } from "./types";

interface DragState {
  pointerId: number;
  x: number;
  y: number;
  mode: "pan" | "rotate";
}

export class InputController {
  private drag: DragState | null = null;
  private pendingPanX = 0;
  private pendingPanY = 0;
  private pendingBearingDelta = 0;
  private pendingPitchDelta = 0;
  private pendingScale = 1;
  private pendingScaleAnchor: { x: number; y: number } | null = null;

  constructor(
    private readonly canvas: HTMLCanvasElement,
    private readonly getModule: () => BrowserMapModule | null,
    private readonly getViewport: () => Viewport,
  ) {}

  attach(): void {
    this.canvas.style.touchAction = "none";
    this.canvas.addEventListener(
      "contextmenu",
      (event) => this.suppress(event),
      {
        capture: true,
      },
    );
    this.canvas.addEventListener("auxclick", (event) => this.suppress(event), {
      capture: true,
    });
    this.canvas.addEventListener(
      "mousedown",
      (event) => {
        if (event.button === 1 || event.button === 2) {
          this.suppress(event);
        }
      },
      {
        capture: true,
      },
    );
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
    window.addEventListener(
      "contextmenu",
      (event) => {
        if (this.drag?.mode === "rotate") {
          this.suppress(event);
        }
      },
      {
        capture: true,
      },
    );
    window.addEventListener("keydown", (event) => this.keydown(event));
  }

  private pointerDown(event: PointerEvent): void {
    const leftDown =
      event.button === 0 || (event.buttons & pointerButtonMask(0)) !== 0;
    const rightDown =
      event.button === 2 || (event.buttons & pointerButtonMask(2)) !== 0;
    if (!leftDown && !rightDown) return;
    this.canvas.focus();
    this.canvas.setPointerCapture(event.pointerId);
    this.drag = {
      pointerId: event.pointerId,
      x: event.clientX,
      y: event.clientY,
      mode: rightDown || event.ctrlKey ? "rotate" : "pan",
    };
    event.preventDefault();
  }

  private pointerMove(event: PointerEvent): void {
    const module = this.getModule();
    if (!this.drag || this.drag.pointerId !== event.pointerId || !module) {
      return;
    }
    if (event.buttons === 0) {
      this.endDrag(event);
      return;
    }
    const delta = this.logicalDelta(this.drag, event);
    if (this.drag.mode === "rotate") {
      this.pendingBearingDelta += delta.x * 0.5;
      this.pendingPitchDelta += -delta.y * 0.5;
    } else {
      this.pendingPanX += delta.x;
      this.pendingPanY += delta.y;
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
    if (!Number.isFinite(scale) || scale <= 0) {
      event.preventDefault();
      return;
    }
    const point = this.logicalPoint(event);
    const pendingScale = this.pendingScale * scale;
    this.pendingScale =
      Number.isFinite(pendingScale) && pendingScale > 0 ? pendingScale : scale;
    this.pendingScaleAnchor = point;
    event.preventDefault();
  }

  applyPending(): void {
    const module = this.getModule();
    if (!module) return;

    if (this.pendingPanX !== 0 || this.pendingPanY !== 0) {
      module._mln_browser_map_move_by(this.pendingPanX, this.pendingPanY);
      this.pendingPanX = 0;
      this.pendingPanY = 0;
    }

    if (this.pendingScale !== 1 && this.pendingScaleAnchor) {
      module._mln_browser_map_scale_by(
        this.pendingScale,
        this.pendingScaleAnchor.x,
        this.pendingScaleAnchor.y,
      );
      this.pendingScale = 1;
      this.pendingScaleAnchor = null;
    }

    if (this.pendingBearingDelta !== 0 || this.pendingPitchDelta !== 0) {
      module._mln_browser_map_rotate_pitch_by(
        this.pendingBearingDelta,
        this.pendingPitchDelta,
      );
      this.pendingBearingDelta = 0;
      this.pendingPitchDelta = 0;
    }
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
        module._mln_browser_map_move_by_animated(pan, 0);
        break;
      case "ArrowRight":
      case "d":
      case "D":
        module._mln_browser_map_move_by_animated(-pan, 0);
        break;
      case "ArrowUp":
      case "w":
      case "W":
        module._mln_browser_map_move_by_animated(0, pan);
        break;
      case "ArrowDown":
      case "s":
      case "S":
        module._mln_browser_map_move_by_animated(0, -pan);
        break;
      case "+":
      case "=":
        module._mln_browser_map_scale_by_animated(1.25, centerX, centerY);
        break;
      case "-":
      case "_":
        module._mln_browser_map_scale_by_animated(1 / 1.25, centerX, centerY);
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

  private suppress(event: Event): void {
    event.preventDefault();
    event.stopPropagation();
  }
}

function pointerButtonMask(button: number): number {
  return button === 2 ? 2 : 1;
}

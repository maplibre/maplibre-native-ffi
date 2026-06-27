import type { Viewport } from "./types";

export function readViewport(canvas: HTMLCanvasElement): Viewport {
  const rect = canvas.getBoundingClientRect();
  return {
    width: Math.max(1, Math.round(rect.width)),
    height: Math.max(1, Math.round(rect.height)),
    scale: Math.max(1, window.devicePixelRatio || 1),
  };
}

export function physicalViewportSize(viewport: Viewport): {
  width: number;
  height: number;
} {
  return {
    width: Math.max(1, Math.ceil(viewport.width * viewport.scale)),
    height: Math.max(1, Math.ceil(viewport.height * viewport.scale)),
  };
}

export function setCanvasPhysicalSize(
  canvas: HTMLCanvasElement,
  viewport: Viewport,
): void {
  const physical = physicalViewportSize(viewport);
  canvas.width = physical.width;
  canvas.height = physical.height;
}

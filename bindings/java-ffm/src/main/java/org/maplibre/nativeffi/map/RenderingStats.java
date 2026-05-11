package org.maplibre.nativeffi.map;

/** Rendering statistics copied from a native render-frame event. */
public record RenderingStats(
    double encodingTime,
    double renderingTime,
    long frameCount,
    long drawCallCount,
    long totalDrawCallCount) {}

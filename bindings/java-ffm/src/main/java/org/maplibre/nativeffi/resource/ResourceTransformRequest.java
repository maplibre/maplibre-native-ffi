package org.maplibre.nativeffi.resource;

/** Copied request passed to a runtime resource transform callback. */
public record ResourceTransformRequest(ResourceKind kind, int rawKind, String url) {}

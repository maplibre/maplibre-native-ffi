package org.maplibre.nativejni.render;

/** OpenGL platform context provider data for OpenGL render targets. */
public sealed interface OpenGLContextDescriptor
    permits WglContextDescriptor, EglContextDescriptor {}

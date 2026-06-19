#pragma once

// DevEco Studio's OpenHarmony SDK ships GLES3/gl3.h without the companion
// GLES3/gl3ext.h header expected by MapLibre Native's shared GLES loader.
// The loader uses core GLES declarations only, so this compatibility header
// satisfies that include without adding extension prototypes.

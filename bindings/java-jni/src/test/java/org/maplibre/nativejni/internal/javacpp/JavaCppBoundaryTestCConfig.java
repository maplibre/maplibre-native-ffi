package org.maplibre.nativejni.internal.javacpp;

import org.bytedeco.javacpp.annotation.Platform;
import org.bytedeco.javacpp.annotation.Properties;

@Properties(
    value =
        @Platform(
            includepath = {"src/test/javacpp"},
            include = {"javacpp_boundary_test.h"}),
    target = "org.maplibre.nativejni.internal.javacpp.JavaCppBoundaryTestC")
public class JavaCppBoundaryTestCConfig {}

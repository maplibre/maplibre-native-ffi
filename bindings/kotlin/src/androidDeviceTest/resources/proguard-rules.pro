# AndroidX Test references these compile-time annotations, but the test APK
# does not need their definitions at runtime.
-dontwarn com.google.errorprone.annotations.CanIgnoreReturnValue
-dontwarn com.google.errorprone.annotations.MustBeClosed

# AndroidJUnitRunner discovers tests by name and annotation rather than through
# static calls, so R8 must retain those entry points.
-keep,allowoptimization class org.maplibre.nativeffi.**Test { *; }

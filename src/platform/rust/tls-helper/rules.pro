# R8 rules for the TLS helper dex. The Rust verifier calls these members over JNI
# by name, as upstream's proguard-rules.pro lists them. Everything else, the Kotlin
# standard library included, is renamed into a package of our own, so the dex
# shares no class with the host even though its loader delegates to the host's.
-keep class org.maplibre.nativeffi.internal.rustlsplatformverifier.CertificateVerifier {
    verifyCertificateChain(
        android.content.Context,
        java.lang.String,
        java.lang.String,
        java.lang.String[],
        byte[],
        long,
        byte[][]
    );
}
-keep class org.maplibre.nativeffi.internal.rustlsplatformverifier.StatusCode { *; }
-keep class org.maplibre.nativeffi.internal.rustlsplatformverifier.VerificationResult { *; }
-repackageclasses org.maplibre.nativeffi.internal.rustlsplatformverifier.r8
-keepattributes SourceFile,LineNumberTable
# The standard library's nullability annotations have no runtime retention.
-dontwarn org.jetbrains.annotations.**

# rustls-platform-verifier Android helper

`CertificateVerifier.kt` comes from
<https://github.com/rustls/rustls-platform-verifier/tree/v/0.6.2> and is
distributed under the upstream MIT OR Apache-2.0 license. The source is
reformatted to the repository's Kotlin style.

The license texts are copied into the Android artifact at build time from the
exact `rustls-platform-verifier` package selected by Cargo.lock.

The local `BuildConfig` fixes the upstream `TEST` build constant to `false`
because this publication contains the production verifier.

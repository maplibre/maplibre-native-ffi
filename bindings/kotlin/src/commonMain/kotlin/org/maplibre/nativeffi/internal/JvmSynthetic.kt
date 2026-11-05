@file:OptIn(ExperimentalMultiplatform::class)

package org.maplibre.nativeffi.internal

/**
 * `kotlin.jvm.JvmSynthetic` for common code, which hides a declaration from Java on the JVM and
 * Android. Java sees an `internal` declaration as public, so each internal member of a class or
 * file in `org.maplibre.nativeffi` carries this annotation. The other targets have no Java and drop
 * it.
 */
@OptionalExpectation
@Target(
  AnnotationTarget.FUNCTION,
  AnnotationTarget.PROPERTY_GETTER,
  AnnotationTarget.PROPERTY_SETTER,
  AnnotationTarget.FIELD,
)
@Retention(AnnotationRetention.SOURCE)
internal expect annotation class JvmSynthetic()

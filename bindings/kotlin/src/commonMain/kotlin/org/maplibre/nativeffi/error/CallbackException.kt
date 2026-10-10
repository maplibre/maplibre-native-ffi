package org.maplibre.nativeffi.error

/**
 * Reports an exception that a native callback threw, which native cannot receive. Native receives
 * the callback's failure value instead, and the binding hands this exception to the platform's
 * handler for exceptions that no caller can receive.
 */
public class CallbackException
internal constructor(
  /** The C callback type that threw, such as `mln_resource_provider_callback`. */
  public val callback: String,
  cause: Throwable,
) : RuntimeException("$callback threw, and native received its failure value", cause)

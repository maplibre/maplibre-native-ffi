package org.maplibre.nativeffi.internal.access;

import java.security.CodeSource;
import java.util.Objects;

/** API-hygiene token for cross-package calls inside this module. */
public enum InternalAccess {
  INSTANCE;

  private static final StackWalker STACK_WALKER =
      StackWalker.getInstance(StackWalker.Option.RETAIN_CLASS_REFERENCE);
  private static final Module LIBRARY_MODULE = InternalAccess.class.getModule();
  private static final String LIBRARY_LOCATION = codeSourceLocation(InternalAccess.class);

  public void checkCaller() {
    checkCaller(1);
  }

  public void checkCaller(int implementationFrameCount) {
    var caller =
        STACK_WALKER.walk(
            frames ->
                frames
                    .map(StackWalker.StackFrame::getDeclaringClass)
                    .filter(type -> type != InternalAccess.class)
                    .skip(implementationFrameCount)
                    .findFirst());
    if (caller.isEmpty() || !isBindingImplementation(caller.orElseThrow())) {
      throw new SecurityException("Internal access is restricted to Maplibre FFM implementation");
    }
  }

  private static boolean isBindingImplementation(Class<?> type) {
    if (!type.getName().startsWith("org.maplibre.nativeffi.")) {
      return false;
    }
    if (LIBRARY_MODULE.isNamed()) {
      return type.getModule() == LIBRARY_MODULE;
    }
    return type.getModule() == LIBRARY_MODULE
        && LIBRARY_LOCATION != null
        && Objects.equals(codeSourceLocation(type), LIBRARY_LOCATION);
  }

  private static String codeSourceLocation(Class<?> type) {
    CodeSource source = type.getProtectionDomain().getCodeSource();
    return source == null || source.getLocation() == null
        ? null
        : source.getLocation().toExternalForm();
  }
}

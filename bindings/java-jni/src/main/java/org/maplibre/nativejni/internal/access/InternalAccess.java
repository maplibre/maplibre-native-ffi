package org.maplibre.nativejni.internal.access;

import java.security.CodeSource;
import java.util.Objects;

/** Token for cross-package calls inside this module. */
public enum InternalAccess {
  INSTANCE;

  private static final StackWalker STACK_WALKER =
      StackWalker.getInstance(StackWalker.Option.RETAIN_CLASS_REFERENCE);
  private static final String LIBRARY_LOCATION = codeSourceLocation(InternalAccess.class);

  public void checkCaller() {
    checkCaller(1);
  }

  public void checkCaller(int implementationFrameCount) {
    if (this != INSTANCE) {
      throw new SecurityException("Invalid internal access token");
    }
    var caller =
        STACK_WALKER.walk(
            frames ->
                frames
                    .map(StackWalker.StackFrame::getDeclaringClass)
                    .filter(type -> type != InternalAccess.class)
                    .skip(implementationFrameCount)
                    .findFirst());
    if (caller.isEmpty() || !isBindingImplementation(caller.orElseThrow())) {
      throw new SecurityException("Internal access is restricted to Maplibre JNI implementation");
    }
  }

  private static boolean isBindingImplementation(Class<?> type) {
    return type.getName().startsWith("org.maplibre.nativejni.")
        && Objects.equals(codeSourceLocation(type), LIBRARY_LOCATION);
  }

  private static String codeSourceLocation(Class<?> type) {
    CodeSource source = type.getProtectionDomain().getCodeSource();
    return source == null || source.getLocation() == null
        ? null
        : source.getLocation().toExternalForm();
  }
}

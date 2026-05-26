package org.maplibre.nativejni.internal.access;

import java.lang.reflect.InvocationTargetException;
import java.util.Objects;
import org.maplibre.nativejni.map.MapHandle;
import org.maplibre.nativejni.resource.ResourceProviderDecision;
import org.maplibre.nativejni.resource.ResourceRequestHandle;
import org.maplibre.nativejni.runtime.RuntimeHandle;

/** Internal bridge for cross-package handle access inside this module. */
public enum InternalAccess {
  INSTANCE;

  public long nativeAddress(MapHandle map) {
    return invokeLong(Objects.requireNonNull(map, "map"), "nativeAddress");
  }

  public long nativeAddress(RuntimeHandle runtime) {
    return invokeLong(Objects.requireNonNull(runtime, "runtime"), "nativeAddress");
  }

  public void registerMap(RuntimeHandle runtime, MapHandle map) {
    invokeVoid(Objects.requireNonNull(runtime, "runtime"), "registerMap", MapHandle.class, map);
  }

  public void unregisterMap(RuntimeHandle runtime, MapHandle map) {
    invokeVoid(Objects.requireNonNull(runtime, "runtime"), "unregisterMap", MapHandle.class, map);
  }

  public void releaseDetachedCustomGeometrySources(MapHandle map) {
    invokeVoid(Objects.requireNonNull(map, "map"), "releaseDetachedCustomGeometrySources");
  }

  public ResourceRequestHandle resourceRequestHandle(long nativeAddress) {
    try {
      var constructor = ResourceRequestHandle.class.getDeclaredConstructor(long.class);
      constructor.setAccessible(true);
      return constructor.newInstance(nativeAddress);
    } catch (ReflectiveOperationException exception) {
      throw rethrow(exception);
    }
  }

  public int finishProviderDecision(
      ResourceRequestHandle handle, ResourceProviderDecision decision) {
    return invokeInt(
        Objects.requireNonNull(handle, "handle"),
        "finishProviderDecision",
        ResourceProviderDecision.class,
        decision);
  }

  public int finishProviderException(ResourceRequestHandle handle) {
    return invokeInt(Objects.requireNonNull(handle, "handle"), "finishProviderException");
  }

  private static long invokeLong(Object target, String methodName) {
    try {
      var method = target.getClass().getDeclaredMethod(methodName);
      method.setAccessible(true);
      return (long) method.invoke(target);
    } catch (ReflectiveOperationException exception) {
      throw rethrow(exception);
    }
  }

  private static void invokeVoid(Object target, String methodName) {
    try {
      var method = target.getClass().getDeclaredMethod(methodName);
      method.setAccessible(true);
      method.invoke(target);
    } catch (ReflectiveOperationException exception) {
      throw rethrow(exception);
    }
  }

  private static int invokeInt(Object target, String methodName) {
    try {
      var method = target.getClass().getDeclaredMethod(methodName);
      method.setAccessible(true);
      return (int) method.invoke(target);
    } catch (ReflectiveOperationException exception) {
      throw rethrow(exception);
    }
  }

  private static int invokeInt(
      Object target, String methodName, Class<?> parameterType, Object argument) {
    try {
      var method = target.getClass().getDeclaredMethod(methodName, parameterType);
      method.setAccessible(true);
      return (int) method.invoke(target, argument);
    } catch (ReflectiveOperationException exception) {
      throw rethrow(exception);
    }
  }

  private static void invokeVoid(
      Object target, String methodName, Class<?> parameterType, Object argument) {
    try {
      var method = target.getClass().getDeclaredMethod(methodName, parameterType);
      method.setAccessible(true);
      method.invoke(target, argument);
    } catch (ReflectiveOperationException exception) {
      throw rethrow(exception);
    }
  }

  private static RuntimeException rethrow(ReflectiveOperationException exception) {
    if (exception instanceof InvocationTargetException invocation) {
      var cause = invocation.getCause();
      if (cause instanceof RuntimeException runtimeException) {
        return runtimeException;
      }
      if (cause instanceof Error error) {
        throw error;
      }
      return new IllegalStateException(cause);
    }
    return new IllegalStateException(exception);
  }
}

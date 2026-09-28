import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

// Vulkan 1.0 host ABI structures used to create a headless graphics device.
final class _ApplicationInfo extends Struct {
  @Uint32()
  external int type;
  external Pointer<Void> next;
  external Pointer<Utf8> name;
  @Uint32()
  external int version;
  external Pointer<Utf8> engine;
  @Uint32()
  external int engineVersion;
  @Uint32()
  external int apiVersion;
}

final class _InstanceInfo extends Struct {
  @Uint32()
  external int type;
  external Pointer<Void> next;
  @Uint32()
  external int flags;
  external Pointer<_ApplicationInfo> application;
  @Uint32()
  external int layerCount;
  external Pointer<Pointer<Utf8>> layers;
  @Uint32()
  external int extensionCount;
  external Pointer<Pointer<Utf8>> extensions;
}

final class _QueueInfo extends Struct {
  @Uint32()
  external int type;
  external Pointer<Void> next;
  @Uint32()
  external int flags;
  @Uint32()
  external int family;
  @Uint32()
  external int count;
  external Pointer<Float> priorities;
}

final class _DeviceInfo extends Struct {
  @Uint32()
  external int type;
  external Pointer<Void> next;
  @Uint32()
  external int flags;
  @Uint32()
  external int queueCount;
  external Pointer<_QueueInfo> queues;
  @Uint32()
  external int layerCount;
  external Pointer<Pointer<Utf8>> layers;
  @Uint32()
  external int extensionCount;
  external Pointer<Pointer<Utf8>> extensions;
  external Pointer<Void> features;
}

final class _Extension extends Struct {
  @Array(256)
  external Array<Uint8> name;
  @Uint32()
  external int version;
}

final class _QueueFamily extends Struct {
  @Uint32()
  external int flags;
  @Uint32()
  external int count;
  @Uint32()
  external int timestampBits;
  @Array(3)
  external Array<Uint32> granularity;
}

final class VulkanTestContext {
  VulkanTestContext() {
    library = DynamicLibrary.open(
      Platform.isMacOS
          ? '${Platform.environment['MLN_FFI_VULKAN_LOADER_DIR'] ?? '/opt/homebrew/lib'}/libvulkan.dylib'
          : Platform.isWindows
          ? 'vulkan-1.dll'
          : 'libvulkan.so.1',
    );
    try {
      using((arena) {
        final enumerateInstance = library
            .lookupFunction<
              Int32 Function(
                Pointer<Utf8>,
                Pointer<Uint32>,
                Pointer<_Extension>,
              ),
              int Function(Pointer<Utf8>, Pointer<Uint32>, Pointer<_Extension>)
            >('vkEnumerateInstanceExtensionProperties');
        final count = arena<Uint32>();
        _check(enumerateInstance(nullptr, count, nullptr));
        final extensions = arena<_Extension>(count.value);
        _check(enumerateInstance(nullptr, count, extensions));
        final portability = _hasExtension(
          extensions,
          count.value,
          'VK_KHR_portability_enumeration',
        );
        final app = arena<_ApplicationInfo>()..ref.type = 0;
        app.ref.apiVersion = 1 << 22;
        final info = arena<_InstanceInfo>()..ref.type = 1;
        info.ref.application = app;
        if (portability) {
          info.ref.flags = 1;
          info.ref.extensionCount = 1;
          info.ref.extensions = arena<Pointer<Utf8>>()
            ..value = 'VK_KHR_portability_enumeration'.toNativeUtf8(
              allocator: arena,
            );
        }
        final instanceOut = arena<Pointer<Void>>();
        _check(
          library.lookupFunction<
            Int32 Function(
              Pointer<_InstanceInfo>,
              Pointer<Void>,
              Pointer<Pointer<Void>>,
            ),
            int Function(
              Pointer<_InstanceInfo>,
              Pointer<Void>,
              Pointer<Pointer<Void>>,
            )
          >('vkCreateInstance')(info, nullptr, instanceOut),
        );
        instance = instanceOut.value;
        final enumerateDevices = library
            .lookupFunction<
              Int32 Function(
                Pointer<Void>,
                Pointer<Uint32>,
                Pointer<Pointer<Void>>,
              ),
              int Function(
                Pointer<Void>,
                Pointer<Uint32>,
                Pointer<Pointer<Void>>,
              )
            >('vkEnumeratePhysicalDevices');
        _check(enumerateDevices(instance, count, nullptr));
        final physicals = arena<Pointer<Void>>(count.value);
        _check(enumerateDevices(instance, count, physicals));
        final deviceCount = count.value;
        final getFamilies = library
            .lookupFunction<
              Void Function(
                Pointer<Void>,
                Pointer<Uint32>,
                Pointer<_QueueFamily>,
              ),
              void Function(
                Pointer<Void>,
                Pointer<Uint32>,
                Pointer<_QueueFamily>,
              )
            >('vkGetPhysicalDeviceQueueFamilyProperties');
        for (var i = 0; i < deviceCount && physical == nullptr; i++) {
          getFamilies(physicals[i], count, nullptr);
          final families = arena<_QueueFamily>(count.value);
          getFamilies(physicals[i], count, families);
          for (var j = 0; j < count.value; j++) {
            if (families[j].count > 0 && families[j].flags & 1 != 0) {
              physical = physicals[i];
              family = j;
              break;
            }
          }
        }
        if (physical == nullptr) {
          throw StateError('Vulkan has no graphics queue');
        }
        final enumerateDeviceExtensions = library
            .lookupFunction<
              Int32 Function(
                Pointer<Void>,
                Pointer<Utf8>,
                Pointer<Uint32>,
                Pointer<_Extension>,
              ),
              int Function(
                Pointer<Void>,
                Pointer<Utf8>,
                Pointer<Uint32>,
                Pointer<_Extension>,
              )
            >('vkEnumerateDeviceExtensionProperties');
        _check(enumerateDeviceExtensions(physical, nullptr, count, nullptr));
        final deviceExtensions = arena<_Extension>(count.value);
        _check(
          enumerateDeviceExtensions(physical, nullptr, count, deviceExtensions),
        );
        final queueInfo = arena<_QueueInfo>()..ref.type = 2;
        queueInfo.ref.family = family;
        queueInfo.ref.count = 1;
        queueInfo.ref.priorities = arena<Float>()..value = 1;
        final deviceInfo = arena<_DeviceInfo>()..ref.type = 3;
        deviceInfo.ref.queueCount = 1;
        deviceInfo.ref.queues = queueInfo;
        if (_hasExtension(
          deviceExtensions,
          count.value,
          'VK_KHR_portability_subset',
        )) {
          deviceInfo.ref.extensionCount = 1;
          deviceInfo.ref.extensions = arena<Pointer<Utf8>>()
            ..value = 'VK_KHR_portability_subset'.toNativeUtf8(
              allocator: arena,
            );
        }
        // VkPhysicalDeviceFeatures is the fixed Vulkan 1.0 array of 55 VkBool32s.
        final features = arena<Uint32>(55);
        library.lookupFunction<
          Void Function(Pointer<Void>, Pointer<Void>),
          void Function(Pointer<Void>, Pointer<Void>)
        >('vkGetPhysicalDeviceFeatures')(physical, features.cast());
        deviceInfo.ref.features = features.cast();
        final deviceOut = arena<Pointer<Void>>();
        _check(
          library.lookupFunction<
            Int32 Function(
              Pointer<Void>,
              Pointer<_DeviceInfo>,
              Pointer<Void>,
              Pointer<Pointer<Void>>,
            ),
            int Function(
              Pointer<Void>,
              Pointer<_DeviceInfo>,
              Pointer<Void>,
              Pointer<Pointer<Void>>,
            )
          >('vkCreateDevice')(physical, deviceInfo, nullptr, deviceOut),
        );
        device = deviceOut.value;
        final queueOut = arena<Pointer<Void>>();
        library.lookupFunction<
          Void Function(Pointer<Void>, Uint32, Uint32, Pointer<Pointer<Void>>),
          void Function(Pointer<Void>, int, int, Pointer<Pointer<Void>>)
        >('vkGetDeviceQueue')(device, family, 0, queueOut);
        queue = queueOut.value;
      });
    } catch (_) {
      close();
      rethrow;
    }
  }

  late final DynamicLibrary library;
  Pointer<Void> instance = nullptr,
      physical = nullptr,
      device = nullptr,
      queue = nullptr;
  int family = 0;

  VulkanContextDescriptor get descriptor => VulkanContextDescriptor(
    instance: NativePointer(instance.address),
    physicalDevice: NativePointer(physical.address),
    device: NativePointer(device.address),
    graphicsQueue: NativePointer(queue.address),
    graphicsQueueFamilyIndex: family,
    getInstanceProcAddr: NativePointer(
      library.lookup<Void>('vkGetInstanceProcAddr').address,
    ),
    getDeviceProcAddr: NativePointer(
      library.lookup<Void>('vkGetDeviceProcAddr').address,
    ),
  );

  void close() {
    if (device != nullptr) {
      _check(
        library.lookupFunction<
          Int32 Function(Pointer<Void>),
          int Function(Pointer<Void>)
        >('vkDeviceWaitIdle')(device),
      );
      library.lookupFunction<
        Void Function(Pointer<Void>, Pointer<Void>),
        void Function(Pointer<Void>, Pointer<Void>)
      >('vkDestroyDevice')(device, nullptr);
      device = nullptr;
    }
    if (instance != nullptr) {
      library.lookupFunction<
        Void Function(Pointer<Void>, Pointer<Void>),
        void Function(Pointer<Void>, Pointer<Void>)
      >('vkDestroyInstance')(instance, nullptr);
      instance = nullptr;
    }
  }
}

bool _hasExtension(Pointer<_Extension> extensions, int count, String name) {
  for (var i = 0; i < count; i++) {
    if ((extensions + i).cast<Utf8>().toDartString() == name) return true;
  }
  return false;
}

void _check(int result) {
  if (result != 0) throw StateError('Vulkan returned $result');
}

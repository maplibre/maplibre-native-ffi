from __future__ import annotations

from dataclasses import dataclass
from importlib import metadata, util
from pathlib import Path
from typing import Any
import os
import sys
import types

from maplibre_native import render


class VulkanUnavailableError(RuntimeError):
    pass


def _import_vulkan() -> Any:
    if sys.platform != "darwin":
        import vulkan

        return vulkan

    library_dir = Path(os.environ.get("MLN_FFI_DEPENDENCY_LIBRARY_DIR", ""))
    library_path = library_dir / "libvulkan.dylib"
    if not library_path.is_file():
        import vulkan

        return vulkan

    distribution = metadata.distribution("vulkan")
    package_root = Path(distribution.locate_file("vulkan"))
    sys.modules.pop("vulkan", None)

    package = types.ModuleType("vulkan")
    package.__file__ = str(package_root / "__init__.py")
    package.__path__ = [str(package_root)]
    package.__package__ = "vulkan"
    package.__version__ = distribution.version
    sys.modules["vulkan"] = package

    cache_spec = util.spec_from_file_location(
        "vulkan._vulkancache",
        package_root / "_vulkancache.py",
    )
    if cache_spec is None or cache_spec.loader is None:
        msg = "could not load vulkan._vulkancache"
        raise VulkanUnavailableError(msg)
    cache_module = util.module_from_spec(cache_spec)
    sys.modules[cache_spec.name] = cache_module
    cache_spec.loader.exec_module(cache_module)

    vulkan_spec = util.spec_from_file_location(
        "vulkan._vulkan", package_root / "_vulkan.py"
    )
    if vulkan_spec is None or vulkan_spec.loader is None:
        msg = "could not load vulkan._vulkan"
        raise VulkanUnavailableError(msg)
    vulkan_module = util.module_from_spec(vulkan_spec)
    sys.modules[vulkan_spec.name] = vulkan_module
    source = (package_root / "_vulkan.py").read_text(encoding="utf-8")
    source = source.replace(
        "_lib_names = ('libvulkan.so.1', 'vulkan-1.dll', 'libvulkan.dylib')",
        f"_lib_names = ({str(library_path)!r}, 'libvulkan.so.1', 'vulkan-1.dll', 'libvulkan.dylib')",
    )
    exec(
        compile(source, str(package_root / "_vulkan.py"), "exec"),
        vulkan_module.__dict__,
    )

    for name, value in vulkan_module.__dict__.items():
        if not name.startswith("_"):
            setattr(package, name, value)
    return package


vk = _import_vulkan()


def _addr(value: Any) -> int:
    return int(vk.ffi.cast("uintptr_t", value))


def _pointer(value: Any, name: str) -> render.NativePointer:
    return render.NativePointer(_addr(value), _diagnostic_name=name)


def _extension_name(extension: Any) -> str:
    raw = extension.extensionName
    if isinstance(raw, str):
        return raw.split("\0", 1)[0]
    return bytes(raw).split(b"\0", 1)[0].decode()


def _has_device_extension(physical_device: Any, name: str) -> bool:
    return any(
        _extension_name(extension) == name
        for extension in vk.vkEnumerateDeviceExtensionProperties(physical_device, None)
    )


def _has_instance_extension(name: str) -> bool:
    return any(
        _extension_name(extension) == name
        for extension in vk.vkEnumerateInstanceExtensionProperties(None)
    )


def _find_graphics_queue_family(physical_device: Any) -> int | None:
    for index, family in enumerate(
        vk.vkGetPhysicalDeviceQueueFamilyProperties(physical_device)
    ):
        if family.queueCount > 0 and family.queueFlags & vk.VK_QUEUE_GRAPHICS_BIT:
            return index
    return None


def _find_memory_type(
    physical_device: Any,
    type_filter: int,
    properties: int,
) -> int:
    memory = vk.vkGetPhysicalDeviceMemoryProperties(physical_device)
    for index in range(memory.memoryTypeCount):
        if not (type_filter & (1 << index)):
            continue
        if memory.memoryTypes[index].propertyFlags & properties == properties:
            return index
    msg = "no compatible Vulkan memory type found"
    raise VulkanUnavailableError(msg)


def _device_features(physical_device: Any) -> Any:
    supported = vk.vkGetPhysicalDeviceFeatures(physical_device)
    features = vk.VkPhysicalDeviceFeatures()
    features.samplerAnisotropy = supported.samplerAnisotropy
    features.wideLines = supported.wideLines
    return features


@dataclass(slots=True)
class VulkanContext:
    instance: Any
    physical_device: Any
    device: Any
    queue: Any
    queue_family_index: int
    _closed: bool = False

    @classmethod
    def create(cls) -> "VulkanContext":
        extensions: list[str] = []
        instance_flags = 0
        if _has_instance_extension("VK_KHR_portability_enumeration"):
            extensions.append("VK_KHR_portability_enumeration")
            instance_flags |= vk.VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR

        app = vk.VkApplicationInfo(
            pApplicationName="maplibre-native-python-render-tests",
            applicationVersion=1,
            pEngineName="maplibre-native-ffi",
            engineVersion=1,
            apiVersion=vk.VK_API_VERSION_1_0,
        )
        instance_info = vk.VkInstanceCreateInfo(
            flags=instance_flags,
            pApplicationInfo=app,
            enabledExtensionCount=len(extensions),
            ppEnabledExtensionNames=extensions,
        )
        try:
            instance = vk.vkCreateInstance(instance_info, None)
        except Exception as error:  # pragma: no cover - depends on host Vulkan ICD
            msg = f"Vulkan instance creation failed: {error}"
            raise VulkanUnavailableError(msg) from error

        device = None
        try:
            for physical_device in vk.vkEnumeratePhysicalDevices(instance):
                queue_family_index = _find_graphics_queue_family(physical_device)
                if queue_family_index is None:
                    continue

                enabled_extensions: list[str] = []
                if _has_device_extension(physical_device, "VK_KHR_portability_subset"):
                    enabled_extensions.append("VK_KHR_portability_subset")

                queue = vk.VkDeviceQueueCreateInfo(
                    queueFamilyIndex=queue_family_index,
                    queueCount=1,
                    pQueuePriorities=[1.0],
                )
                device_info = vk.VkDeviceCreateInfo(
                    queueCreateInfoCount=1,
                    pQueueCreateInfos=[queue],
                    enabledExtensionCount=len(enabled_extensions),
                    ppEnabledExtensionNames=enabled_extensions,
                    pEnabledFeatures=_device_features(physical_device),
                )
                try:
                    device = vk.vkCreateDevice(physical_device, device_info, None)
                except Exception:
                    continue

                graphics_queue = vk.vkGetDeviceQueue(device, queue_family_index, 0)
                return cls(
                    instance=instance,
                    physical_device=physical_device,
                    device=device,
                    queue=graphics_queue,
                    queue_family_index=queue_family_index,
                )
        except Exception:
            if device is not None:
                vk.vkDestroyDevice(device, None)
            vk.vkDestroyInstance(instance, None)
            raise

        vk.vkDestroyInstance(instance, None)
        msg = "no Vulkan physical device with a graphics queue was found"
        raise VulkanUnavailableError(msg)

    def descriptor(self) -> render.VulkanContextDescriptor:
        return render.VulkanContextDescriptor(
            instance=_pointer(self.instance, "VkInstance"),
            physical_device=_pointer(self.physical_device, "VkPhysicalDevice"),
            device=_pointer(self.device, "VkDevice"),
            graphics_queue=_pointer(self.queue, "VkQueue"),
            graphics_queue_family_index=self.queue_family_index,
            get_instance_proc_addr=_pointer(
                vk.ffi.addressof(vk.lib, "vkGetInstanceProcAddr"),
                "vkGetInstanceProcAddr",
            ),
            get_device_proc_addr=_pointer(
                vk.ffi.addressof(vk.lib, "vkGetDeviceProcAddr"),
                "vkGetDeviceProcAddr",
            ),
        )

    def owned_texture_descriptor(
        self,
        width: int = 64,
        height: int = 64,
        scale_factor: float = 1.0,
    ) -> render.VulkanOwnedTextureDescriptor:
        return render.VulkanOwnedTextureDescriptor(
            extent=render.RenderTargetExtent(width, height, scale_factor),
            context=self.descriptor(),
        )

    def borrowed_image(
        self,
        width: int = 64,
        height: int = 64,
        scale_factor: float = 1.0,
    ) -> "VulkanBorrowedImage":
        return VulkanBorrowedImage.create(self, width, height, scale_factor)

    def close(self) -> None:
        if self._closed:
            return
        vk.vkDeviceWaitIdle(self.device)
        vk.vkDestroyDevice(self.device, None)
        vk.vkDestroyInstance(self.instance, None)
        self._closed = True

    def __enter__(self) -> "VulkanContext":
        return self

    def __exit__(self, *args: object) -> None:
        self.close()


@dataclass(slots=True)
class VulkanBorrowedImage:
    context: VulkanContext
    image: Any
    image_view: Any
    memory: Any
    width: int
    height: int
    scale_factor: float
    _closed: bool = False

    @classmethod
    def create(
        cls,
        context: VulkanContext,
        width: int,
        height: int,
        scale_factor: float,
    ) -> "VulkanBorrowedImage":
        image = vk.vkCreateImage(
            context.device,
            vk.VkImageCreateInfo(
                imageType=vk.VK_IMAGE_TYPE_2D,
                format=vk.VK_FORMAT_R8G8B8A8_UNORM,
                extent=vk.VkExtent3D(width=width, height=height, depth=1),
                mipLevels=1,
                arrayLayers=1,
                samples=vk.VK_SAMPLE_COUNT_1_BIT,
                tiling=vk.VK_IMAGE_TILING_OPTIMAL,
                usage=(
                    vk.VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                    | vk.VK_IMAGE_USAGE_SAMPLED_BIT
                    | vk.VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                ),
                sharingMode=vk.VK_SHARING_MODE_EXCLUSIVE,
                initialLayout=vk.VK_IMAGE_LAYOUT_UNDEFINED,
            ),
            None,
        )
        memory = None
        image_view = None
        try:
            requirements = vk.vkGetImageMemoryRequirements(context.device, image)
            memory = vk.vkAllocateMemory(
                context.device,
                vk.VkMemoryAllocateInfo(
                    allocationSize=requirements.size,
                    memoryTypeIndex=_find_memory_type(
                        context.physical_device,
                        requirements.memoryTypeBits,
                        vk.VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    ),
                ),
                None,
            )
            vk.vkBindImageMemory(context.device, image, memory, 0)
            image_view = vk.vkCreateImageView(
                context.device,
                vk.VkImageViewCreateInfo(
                    image=image,
                    viewType=vk.VK_IMAGE_VIEW_TYPE_2D,
                    format=vk.VK_FORMAT_R8G8B8A8_UNORM,
                    subresourceRange=vk.VkImageSubresourceRange(
                        aspectMask=vk.VK_IMAGE_ASPECT_COLOR_BIT,
                        baseMipLevel=0,
                        levelCount=1,
                        baseArrayLayer=0,
                        layerCount=1,
                    ),
                ),
                None,
            )
            return cls(context, image, image_view, memory, width, height, scale_factor)
        except Exception:
            if image_view is not None:
                vk.vkDestroyImageView(context.device, image_view, None)
            if memory is not None:
                vk.vkFreeMemory(context.device, memory, None)
            vk.vkDestroyImage(context.device, image, None)
            raise

    def descriptor(self) -> render.VulkanBorrowedTextureDescriptor:
        return render.VulkanBorrowedTextureDescriptor(
            extent=render.RenderTargetExtent(
                self.width,
                self.height,
                self.scale_factor,
            ),
            context=self.context.descriptor(),
            image=_pointer(self.image, "VkImage"),
            image_view=_pointer(self.image_view, "VkImageView"),
            format=vk.VK_FORMAT_R8G8B8A8_UNORM,
            initial_layout=vk.VK_IMAGE_LAYOUT_UNDEFINED,
            final_layout=vk.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        )

    def close(self) -> None:
        if self._closed:
            return
        vk.vkDeviceWaitIdle(self.context.device)
        vk.vkDestroyImageView(self.context.device, self.image_view, None)
        vk.vkDestroyImage(self.context.device, self.image, None)
        vk.vkFreeMemory(self.context.device, self.memory, None)
        self._closed = True

    def __enter__(self) -> "VulkanBorrowedImage":
        return self

    def __exit__(self, *args: object) -> None:
        self.close()

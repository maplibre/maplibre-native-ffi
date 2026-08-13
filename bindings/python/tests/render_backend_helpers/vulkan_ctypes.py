from __future__ import annotations

# Android tests use this ctypes subset because the vulkan package requires CFFI.
# Keep its constants and layouts synchronized with the canonical Vulkan header:
# https://github.com/KhronosGroup/Vulkan-Headers/blob/main/include/vulkan/vulkan_core.h
import ctypes
from collections.abc import Callable
from typing import Any

VK_SUCCESS = 0
VK_INCOMPLETE = 5

VK_API_VERSION_1_0 = 1 << 22
VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR = 1
VK_QUEUE_GRAPHICS_BIT = 1
VK_IMAGE_TYPE_2D = 1
VK_FORMAT_R8G8B8A8_UNORM = 37
VK_SAMPLE_COUNT_1_BIT = 1
VK_IMAGE_TILING_OPTIMAL = 0
VK_IMAGE_USAGE_TRANSFER_SRC_BIT = 1
VK_IMAGE_USAGE_SAMPLED_BIT = 4
VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT = 16
VK_SHARING_MODE_EXCLUSIVE = 0
VK_IMAGE_LAYOUT_UNDEFINED = 0
VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL = 5
VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = 1
VK_IMAGE_VIEW_TYPE_2D = 1
VK_IMAGE_ASPECT_COLOR_BIT = 1

_VK_STRUCTURE_TYPE_APPLICATION_INFO = 0
_VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1
_VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO = 2
_VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO = 3
_VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO = 5
_VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO = 14
_VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO = 15

_VkBool32 = ctypes.c_uint32
_VkDeviceSize = ctypes.c_uint64
_Handle = ctypes.c_void_p


class VkError(RuntimeError):
    pass


class _VkExtent3D(ctypes.Structure):
    _fields_ = [
        ("width", ctypes.c_uint32),
        ("height", ctypes.c_uint32),
        ("depth", ctypes.c_uint32),
    ]


class _VkApplicationInfo(ctypes.Structure):
    _fields_ = [
        ("sType", ctypes.c_uint32),
        ("pNext", ctypes.c_void_p),
        ("pApplicationName", ctypes.c_char_p),
        ("applicationVersion", ctypes.c_uint32),
        ("pEngineName", ctypes.c_char_p),
        ("engineVersion", ctypes.c_uint32),
        ("apiVersion", ctypes.c_uint32),
    ]


class _VkInstanceCreateInfo(ctypes.Structure):
    _fields_ = [
        ("sType", ctypes.c_uint32),
        ("pNext", ctypes.c_void_p),
        ("flags", ctypes.c_uint32),
        ("pApplicationInfo", ctypes.POINTER(_VkApplicationInfo)),
        ("enabledLayerCount", ctypes.c_uint32),
        ("ppEnabledLayerNames", ctypes.POINTER(ctypes.c_char_p)),
        ("enabledExtensionCount", ctypes.c_uint32),
        ("ppEnabledExtensionNames", ctypes.POINTER(ctypes.c_char_p)),
    ]


class _VkExtensionProperties(ctypes.Structure):
    _fields_ = [
        ("extensionName", ctypes.c_char * 256),
        ("specVersion", ctypes.c_uint32),
    ]


class _VkPhysicalDeviceFeatures(ctypes.Structure):
    _fields_ = [
        (name, _VkBool32)
        for name in (
            "robustBufferAccess",
            "fullDrawIndexUint32",
            "imageCubeArray",
            "independentBlend",
            "geometryShader",
            "tessellationShader",
            "sampleRateShading",
            "dualSrcBlend",
            "logicOp",
            "multiDrawIndirect",
            "drawIndirectFirstInstance",
            "depthClamp",
            "depthBiasClamp",
            "fillModeNonSolid",
            "depthBounds",
            "wideLines",
            "largePoints",
            "alphaToOne",
            "multiViewport",
            "samplerAnisotropy",
            "textureCompressionETC2",
            "textureCompressionASTC_LDR",
            "textureCompressionBC",
            "occlusionQueryPrecise",
            "pipelineStatisticsQuery",
            "vertexPipelineStoresAndAtomics",
            "fragmentStoresAndAtomics",
            "shaderTessellationAndGeometryPointSize",
            "shaderImageGatherExtended",
            "shaderStorageImageExtendedFormats",
            "shaderStorageImageMultisample",
            "shaderStorageImageReadWithoutFormat",
            "shaderStorageImageWriteWithoutFormat",
            "shaderUniformBufferArrayDynamicIndexing",
            "shaderSampledImageArrayDynamicIndexing",
            "shaderStorageBufferArrayDynamicIndexing",
            "shaderStorageImageArrayDynamicIndexing",
            "shaderClipDistance",
            "shaderCullDistance",
            "shaderFloat64",
            "shaderInt64",
            "shaderInt16",
            "shaderResourceResidency",
            "shaderResourceMinLod",
            "sparseBinding",
            "sparseResidencyBuffer",
            "sparseResidencyImage2D",
            "sparseResidencyImage3D",
            "sparseResidency2Samples",
            "sparseResidency4Samples",
            "sparseResidency8Samples",
            "sparseResidency16Samples",
            "sparseResidencyAliased",
            "variableMultisampleRate",
            "inheritedQueries",
        )
    ]


class _VkQueueFamilyProperties(ctypes.Structure):
    _fields_ = [
        ("queueFlags", ctypes.c_uint32),
        ("queueCount", ctypes.c_uint32),
        ("timestampValidBits", ctypes.c_uint32),
        ("minImageTransferGranularity", _VkExtent3D),
    ]


class _VkDeviceQueueCreateInfo(ctypes.Structure):
    _fields_ = [
        ("sType", ctypes.c_uint32),
        ("pNext", ctypes.c_void_p),
        ("flags", ctypes.c_uint32),
        ("queueFamilyIndex", ctypes.c_uint32),
        ("queueCount", ctypes.c_uint32),
        ("pQueuePriorities", ctypes.POINTER(ctypes.c_float)),
    ]


class _VkDeviceCreateInfo(ctypes.Structure):
    _fields_ = [
        ("sType", ctypes.c_uint32),
        ("pNext", ctypes.c_void_p),
        ("flags", ctypes.c_uint32),
        ("queueCreateInfoCount", ctypes.c_uint32),
        ("pQueueCreateInfos", ctypes.POINTER(_VkDeviceQueueCreateInfo)),
        ("enabledLayerCount", ctypes.c_uint32),
        ("ppEnabledLayerNames", ctypes.POINTER(ctypes.c_char_p)),
        ("enabledExtensionCount", ctypes.c_uint32),
        ("ppEnabledExtensionNames", ctypes.POINTER(ctypes.c_char_p)),
        ("pEnabledFeatures", ctypes.POINTER(_VkPhysicalDeviceFeatures)),
    ]


class _VkMemoryType(ctypes.Structure):
    _fields_ = [
        ("propertyFlags", ctypes.c_uint32),
        ("heapIndex", ctypes.c_uint32),
    ]


class _VkMemoryHeap(ctypes.Structure):
    _fields_ = [("size", _VkDeviceSize), ("flags", ctypes.c_uint32)]


class _VkPhysicalDeviceMemoryProperties(ctypes.Structure):
    _fields_ = [
        ("memoryTypeCount", ctypes.c_uint32),
        ("memoryTypes", _VkMemoryType * 32),
        ("memoryHeapCount", ctypes.c_uint32),
        ("memoryHeaps", _VkMemoryHeap * 16),
    ]


class _VkImageCreateInfo(ctypes.Structure):
    _fields_ = [
        ("sType", ctypes.c_uint32),
        ("pNext", ctypes.c_void_p),
        ("flags", ctypes.c_uint32),
        ("imageType", ctypes.c_uint32),
        ("format", ctypes.c_uint32),
        ("extent", _VkExtent3D),
        ("mipLevels", ctypes.c_uint32),
        ("arrayLayers", ctypes.c_uint32),
        ("samples", ctypes.c_uint32),
        ("tiling", ctypes.c_uint32),
        ("usage", ctypes.c_uint32),
        ("sharingMode", ctypes.c_uint32),
        ("queueFamilyIndexCount", ctypes.c_uint32),
        ("pQueueFamilyIndices", ctypes.POINTER(ctypes.c_uint32)),
        ("initialLayout", ctypes.c_uint32),
    ]


class _VkMemoryRequirements(ctypes.Structure):
    _fields_ = [
        ("size", _VkDeviceSize),
        ("alignment", _VkDeviceSize),
        ("memoryTypeBits", ctypes.c_uint32),
    ]


class _VkMemoryAllocateInfo(ctypes.Structure):
    _fields_ = [
        ("sType", ctypes.c_uint32),
        ("pNext", ctypes.c_void_p),
        ("allocationSize", _VkDeviceSize),
        ("memoryTypeIndex", ctypes.c_uint32),
    ]


class _VkComponentMapping(ctypes.Structure):
    _fields_ = [
        ("r", ctypes.c_uint32),
        ("g", ctypes.c_uint32),
        ("b", ctypes.c_uint32),
        ("a", ctypes.c_uint32),
    ]


class _VkImageSubresourceRange(ctypes.Structure):
    _fields_ = [
        ("aspectMask", ctypes.c_uint32),
        ("baseMipLevel", ctypes.c_uint32),
        ("levelCount", ctypes.c_uint32),
        ("baseArrayLayer", ctypes.c_uint32),
        ("layerCount", ctypes.c_uint32),
    ]


class _VkImageViewCreateInfo(ctypes.Structure):
    _fields_ = [
        ("sType", ctypes.c_uint32),
        ("pNext", ctypes.c_void_p),
        ("flags", ctypes.c_uint32),
        ("image", _Handle),
        ("viewType", ctypes.c_uint32),
        ("format", ctypes.c_uint32),
        ("components", _VkComponentMapping),
        ("subresourceRange", _VkImageSubresourceRange),
    ]


_lib = ctypes.CDLL("libvulkan.so")


def _function(
    name: str,
    argtypes: list[Any],
    restype: Any = ctypes.c_int32,
) -> Callable[..., Any]:
    function = getattr(_lib, name)
    function.argtypes = argtypes
    function.restype = restype
    return function


_vkCreateInstance = _function(
    "vkCreateInstance",
    [ctypes.POINTER(_VkInstanceCreateInfo), ctypes.c_void_p, ctypes.POINTER(_Handle)],
)
_vkEnumerateInstanceExtensionProperties = _function(
    "vkEnumerateInstanceExtensionProperties",
    [ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint32), ctypes.c_void_p],
)
_vkDestroyInstance = _function("vkDestroyInstance", [_Handle, ctypes.c_void_p], None)
_vkEnumeratePhysicalDevices = _function(
    "vkEnumeratePhysicalDevices",
    [_Handle, ctypes.POINTER(ctypes.c_uint32), ctypes.c_void_p],
)
_vkGetPhysicalDeviceQueueFamilyProperties = _function(
    "vkGetPhysicalDeviceQueueFamilyProperties",
    [_Handle, ctypes.POINTER(ctypes.c_uint32), ctypes.c_void_p],
    None,
)
_vkEnumerateDeviceExtensionProperties = _function(
    "vkEnumerateDeviceExtensionProperties",
    [_Handle, ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint32), ctypes.c_void_p],
)
_vkGetPhysicalDeviceFeatures = _function(
    "vkGetPhysicalDeviceFeatures",
    [_Handle, ctypes.POINTER(_VkPhysicalDeviceFeatures)],
    None,
)
_vkGetPhysicalDeviceMemoryProperties = _function(
    "vkGetPhysicalDeviceMemoryProperties",
    [_Handle, ctypes.POINTER(_VkPhysicalDeviceMemoryProperties)],
    None,
)
_vkCreateDevice = _function(
    "vkCreateDevice",
    [
        _Handle,
        ctypes.POINTER(_VkDeviceCreateInfo),
        ctypes.c_void_p,
        ctypes.POINTER(_Handle),
    ],
)
_vkDestroyDevice = _function("vkDestroyDevice", [_Handle, ctypes.c_void_p], None)
_vkGetDeviceQueue = _function(
    "vkGetDeviceQueue",
    [_Handle, ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(_Handle)],
    None,
)
_vkDeviceWaitIdle = _function("vkDeviceWaitIdle", [_Handle])
_vkCreateImage = _function(
    "vkCreateImage",
    [
        _Handle,
        ctypes.POINTER(_VkImageCreateInfo),
        ctypes.c_void_p,
        ctypes.POINTER(_Handle),
    ],
)
_vkDestroyImage = _function("vkDestroyImage", [_Handle, _Handle, ctypes.c_void_p], None)
_vkGetImageMemoryRequirements = _function(
    "vkGetImageMemoryRequirements",
    [_Handle, _Handle, ctypes.POINTER(_VkMemoryRequirements)],
    None,
)
_vkAllocateMemory = _function(
    "vkAllocateMemory",
    [
        _Handle,
        ctypes.POINTER(_VkMemoryAllocateInfo),
        ctypes.c_void_p,
        ctypes.POINTER(_Handle),
    ],
)
_vkFreeMemory = _function("vkFreeMemory", [_Handle, _Handle, ctypes.c_void_p], None)
_vkBindImageMemory = _function(
    "vkBindImageMemory", [_Handle, _Handle, _Handle, _VkDeviceSize]
)
_vkCreateImageView = _function(
    "vkCreateImageView",
    [
        _Handle,
        ctypes.POINTER(_VkImageViewCreateInfo),
        ctypes.c_void_p,
        ctypes.POINTER(_Handle),
    ],
)
_vkDestroyImageView = _function(
    "vkDestroyImageView", [_Handle, _Handle, ctypes.c_void_p], None
)


def _check(result: int, operation: str) -> None:
    if result != VK_SUCCESS:
        raise VkError(f"{operation} returned VkResult {result}")


def _encoded_names(names: list[str]) -> Any:
    if not names:
        return None
    encoded = [name.encode() for name in names]
    return (ctypes.c_char_p * len(encoded))(*encoded)


def address(value: Any) -> int:
    if isinstance(value, int):
        return value
    return ctypes.cast(value, ctypes.c_void_p).value or 0


def function_address(name: str) -> int:
    return address(getattr(_lib, name))


def VkExtent3D(*, width: int, height: int, depth: int) -> _VkExtent3D:
    return _VkExtent3D(width, height, depth)


def VkApplicationInfo(
    *,
    pApplicationName: str,
    applicationVersion: int,
    pEngineName: str,
    engineVersion: int,
    apiVersion: int,
) -> _VkApplicationInfo:
    return _VkApplicationInfo(
        sType=_VK_STRUCTURE_TYPE_APPLICATION_INFO,
        pApplicationName=pApplicationName.encode(),
        applicationVersion=applicationVersion,
        pEngineName=pEngineName.encode(),
        engineVersion=engineVersion,
        apiVersion=apiVersion,
    )


def VkInstanceCreateInfo(
    *,
    flags: int,
    pApplicationInfo: _VkApplicationInfo,
    enabledExtensionCount: int,
    ppEnabledExtensionNames: list[str],
) -> _VkInstanceCreateInfo:
    names = _encoded_names(ppEnabledExtensionNames)
    info = _VkInstanceCreateInfo(
        sType=_VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        flags=flags,
        pApplicationInfo=ctypes.pointer(pApplicationInfo),
        enabledExtensionCount=enabledExtensionCount,
        ppEnabledExtensionNames=names,
    )
    info._references = (pApplicationInfo, names)
    return info


def VkPhysicalDeviceFeatures() -> _VkPhysicalDeviceFeatures:
    return _VkPhysicalDeviceFeatures()


def VkDeviceQueueCreateInfo(
    *, queueFamilyIndex: int, queueCount: int, pQueuePriorities: list[float]
) -> _VkDeviceQueueCreateInfo:
    priorities = (ctypes.c_float * len(pQueuePriorities))(*pQueuePriorities)
    info = _VkDeviceQueueCreateInfo(
        sType=_VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        queueFamilyIndex=queueFamilyIndex,
        queueCount=queueCount,
        pQueuePriorities=priorities,
    )
    info._references = priorities
    return info


def VkDeviceCreateInfo(
    *,
    queueCreateInfoCount: int,
    pQueueCreateInfos: list[_VkDeviceQueueCreateInfo],
    enabledExtensionCount: int,
    ppEnabledExtensionNames: list[str],
    pEnabledFeatures: _VkPhysicalDeviceFeatures,
) -> _VkDeviceCreateInfo:
    queues = (_VkDeviceQueueCreateInfo * len(pQueueCreateInfos))(*pQueueCreateInfos)
    names = _encoded_names(ppEnabledExtensionNames)
    info = _VkDeviceCreateInfo(
        sType=_VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        queueCreateInfoCount=queueCreateInfoCount,
        pQueueCreateInfos=queues,
        enabledExtensionCount=enabledExtensionCount,
        ppEnabledExtensionNames=names,
        pEnabledFeatures=ctypes.pointer(pEnabledFeatures),
    )
    info._references = (queues, names, pEnabledFeatures)
    return info


def VkImageCreateInfo(**values: Any) -> _VkImageCreateInfo:
    return _VkImageCreateInfo(sType=_VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, **values)


def VkMemoryAllocateInfo(
    *, allocationSize: int, memoryTypeIndex: int
) -> _VkMemoryAllocateInfo:
    return _VkMemoryAllocateInfo(
        sType=_VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        allocationSize=allocationSize,
        memoryTypeIndex=memoryTypeIndex,
    )


def VkImageSubresourceRange(**values: Any) -> _VkImageSubresourceRange:
    return _VkImageSubresourceRange(**values)


def VkImageViewCreateInfo(**values: Any) -> _VkImageViewCreateInfo:
    return _VkImageViewCreateInfo(
        sType=_VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        components=_VkComponentMapping(),
        **values,
    )


def vkEnumerateInstanceExtensionProperties(_: None) -> list[_VkExtensionProperties]:
    count = ctypes.c_uint32()
    _check(
        _vkEnumerateInstanceExtensionProperties(None, ctypes.byref(count), None),
        "vkEnumerateInstanceExtensionProperties",
    )
    properties = (_VkExtensionProperties * count.value)()
    result = _vkEnumerateInstanceExtensionProperties(
        None, ctypes.byref(count), properties
    )
    if result not in (VK_SUCCESS, VK_INCOMPLETE):
        _check(result, "vkEnumerateInstanceExtensionProperties")
    return list(properties[: count.value])


def vkCreateInstance(info: _VkInstanceCreateInfo, _: None) -> int:
    instance = _Handle()
    _check(
        _vkCreateInstance(ctypes.byref(info), None, ctypes.byref(instance)),
        "vkCreateInstance",
    )
    return address(instance)


def vkDestroyInstance(instance: int, _: None) -> None:
    _vkDestroyInstance(instance, None)


def vkEnumeratePhysicalDevices(instance: int) -> list[int]:
    count = ctypes.c_uint32()
    _check(
        _vkEnumeratePhysicalDevices(instance, ctypes.byref(count), None),
        "vkEnumeratePhysicalDevices",
    )
    devices = (_Handle * count.value)()
    result = _vkEnumeratePhysicalDevices(instance, ctypes.byref(count), devices)
    if result not in (VK_SUCCESS, VK_INCOMPLETE):
        _check(result, "vkEnumeratePhysicalDevices")
    return [address(device) for device in devices[: count.value]]


def vkGetPhysicalDeviceQueueFamilyProperties(
    physical_device: int,
) -> list[_VkQueueFamilyProperties]:
    count = ctypes.c_uint32()
    _vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, ctypes.byref(count), None
    )
    properties = (_VkQueueFamilyProperties * count.value)()
    _vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, ctypes.byref(count), properties
    )
    return list(properties[: count.value])


def vkEnumerateDeviceExtensionProperties(
    physical_device: int, _: None
) -> list[_VkExtensionProperties]:
    count = ctypes.c_uint32()
    _check(
        _vkEnumerateDeviceExtensionProperties(
            physical_device, None, ctypes.byref(count), None
        ),
        "vkEnumerateDeviceExtensionProperties",
    )
    properties = (_VkExtensionProperties * count.value)()
    result = _vkEnumerateDeviceExtensionProperties(
        physical_device, None, ctypes.byref(count), properties
    )
    if result not in (VK_SUCCESS, VK_INCOMPLETE):
        _check(result, "vkEnumerateDeviceExtensionProperties")
    return list(properties[: count.value])


def vkGetPhysicalDeviceFeatures(physical_device: int) -> _VkPhysicalDeviceFeatures:
    features = _VkPhysicalDeviceFeatures()
    _vkGetPhysicalDeviceFeatures(physical_device, ctypes.byref(features))
    return features


def vkGetPhysicalDeviceMemoryProperties(
    physical_device: int,
) -> _VkPhysicalDeviceMemoryProperties:
    properties = _VkPhysicalDeviceMemoryProperties()
    _vkGetPhysicalDeviceMemoryProperties(physical_device, ctypes.byref(properties))
    return properties


def vkCreateDevice(physical_device: int, info: _VkDeviceCreateInfo, _: None) -> int:
    device = _Handle()
    _check(
        _vkCreateDevice(
            physical_device, ctypes.byref(info), None, ctypes.byref(device)
        ),
        "vkCreateDevice",
    )
    return address(device)


def vkDestroyDevice(device: int, _: None) -> None:
    _vkDestroyDevice(device, None)


def vkGetDeviceQueue(device: int, family_index: int, queue_index: int) -> int:
    queue = _Handle()
    _vkGetDeviceQueue(device, family_index, queue_index, ctypes.byref(queue))
    return address(queue)


def vkDeviceWaitIdle(device: int) -> None:
    _check(_vkDeviceWaitIdle(device), "vkDeviceWaitIdle")


def vkCreateImage(device: int, info: _VkImageCreateInfo, _: None) -> int:
    image = _Handle()
    _check(
        _vkCreateImage(device, ctypes.byref(info), None, ctypes.byref(image)),
        "vkCreateImage",
    )
    return address(image)


def vkDestroyImage(device: int, image: int, _: None) -> None:
    _vkDestroyImage(device, image, None)


def vkGetImageMemoryRequirements(device: int, image: int) -> _VkMemoryRequirements:
    requirements = _VkMemoryRequirements()
    _vkGetImageMemoryRequirements(device, image, ctypes.byref(requirements))
    return requirements


def vkAllocateMemory(device: int, info: _VkMemoryAllocateInfo, _: None) -> int:
    memory = _Handle()
    _check(
        _vkAllocateMemory(device, ctypes.byref(info), None, ctypes.byref(memory)),
        "vkAllocateMemory",
    )
    return address(memory)


def vkFreeMemory(device: int, memory: int, _: None) -> None:
    _vkFreeMemory(device, memory, None)


def vkBindImageMemory(device: int, image: int, memory: int, offset: int) -> None:
    _check(
        _vkBindImageMemory(device, image, memory, offset),
        "vkBindImageMemory",
    )


def vkCreateImageView(device: int, info: _VkImageViewCreateInfo, _: None) -> int:
    image_view = _Handle()
    _check(
        _vkCreateImageView(device, ctypes.byref(info), None, ctypes.byref(image_view)),
        "vkCreateImageView",
    )
    return address(image_view)


def vkDestroyImageView(device: int, image_view: int, _: None) -> None:
    _vkDestroyImageView(device, image_view, None)

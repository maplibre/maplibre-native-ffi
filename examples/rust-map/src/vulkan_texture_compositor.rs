use std::io::Cursor;

use ash::vk;
use ash::vk::Handle;
use maplibre_native::{Error, ErrorKind, VulkanOwnedTextureFrameHandle};

use crate::viewport::Viewport;
use crate::vulkan::VulkanContext;

const VERT_SHADER: &[u8] = include_bytes!("vulkan_texture_compositor/shaders/fullscreen.vert.spv");
const FRAG_SHADER: &[u8] = include_bytes!("vulkan_texture_compositor/shaders/sample.frag.spv");

pub struct VulkanTextureCompositor {
    surface_loader: ash::khr::surface::Instance,
    surface: vk::SurfaceKHR,
    physical_device: vk::PhysicalDevice,
    device: ash::Device,
    graphics_queue: vk::Queue,
    graphics_queue_family_index: u32,
    swapchain_loader: ash::khr::swapchain::Device,
    swapchain: vk::SwapchainKHR,
    swapchain_format: vk::Format,
    extent: vk::Extent2D,
    image_views: Vec<vk::ImageView>,
    framebuffers: Vec<vk::Framebuffer>,
    render_pass: vk::RenderPass,
    descriptor_set_layout: vk::DescriptorSetLayout,
    pipeline_layout: vk::PipelineLayout,
    pipeline: vk::Pipeline,
    sampler: vk::Sampler,
    descriptor_pool: vk::DescriptorPool,
    descriptor_set: vk::DescriptorSet,
    command_pool: vk::CommandPool,
    command_buffer: vk::CommandBuffer,
    image_available: vk::Semaphore,
    render_finished: vk::Semaphore,
    in_flight: vk::Fence,
    closed: bool,
}

impl VulkanTextureCompositor {
    pub fn new(context: &VulkanContext, viewport: Viewport) -> Result<Self, vk::Result> {
        let instance = context.instance().clone();
        let device = context.device().clone();
        let swapchain_loader = ash::khr::swapchain::Device::new(&instance, &device);
        let mut compositor = Self {
            surface_loader: context.surface_loader().clone(),
            surface: context.surface(),
            physical_device: context.physical_device(),
            device,
            graphics_queue: context.graphics_queue(),
            graphics_queue_family_index: context.graphics_queue_family_index(),
            swapchain_loader,
            swapchain: vk::SwapchainKHR::null(),
            swapchain_format: vk::Format::UNDEFINED,
            extent: vk::Extent2D {
                width: 0,
                height: 0,
            },
            image_views: Vec::new(),
            framebuffers: Vec::new(),
            render_pass: vk::RenderPass::null(),
            descriptor_set_layout: vk::DescriptorSetLayout::null(),
            pipeline_layout: vk::PipelineLayout::null(),
            pipeline: vk::Pipeline::null(),
            sampler: vk::Sampler::null(),
            descriptor_pool: vk::DescriptorPool::null(),
            descriptor_set: vk::DescriptorSet::null(),
            command_pool: vk::CommandPool::null(),
            command_buffer: vk::CommandBuffer::null(),
            image_available: vk::Semaphore::null(),
            render_finished: vk::Semaphore::null(),
            in_flight: vk::Fence::null(),
            closed: false,
        };
        compositor.create_swapchain(viewport, vk::SwapchainKHR::null())?;
        compositor.create_render_pass()?;
        compositor.create_descriptor_state()?;
        compositor.create_pipeline()?;
        compositor.create_framebuffers()?;
        compositor.create_commands()?;
        Ok(compositor)
    }

    pub fn resize(&mut self, viewport: Viewport) -> Result<(), vk::Result> {
        self.wait_idle()?;
        self.destroy_swapchain_dependents();
        let old_swapchain = self.swapchain;
        self.create_swapchain(viewport, old_swapchain)?;
        if old_swapchain != vk::SwapchainKHR::null() {
            unsafe { self.swapchain_loader.destroy_swapchain(old_swapchain, None) };
        }
        self.create_framebuffers()?;
        Ok(())
    }

    pub fn draw(&mut self, frame: &VulkanOwnedTextureFrameHandle) -> maplibre_native::Result<()> {
        let metadata = frame.frame()?;
        if metadata.width == 0 || metadata.height == 0 {
            return Err(compositor_error("owned Vulkan frame has an empty extent"));
        }
        if metadata.layout != vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL.as_raw() as u32 {
            return Err(compositor_error(format!(
                "owned Vulkan frame has layout {}, expected SHADER_READ_ONLY_OPTIMAL",
                metadata.layout
            )));
        }
        let image_view = unsafe { vk::ImageView::from_raw(frame.image_view()?.address() as u64) };
        if image_view == vk::ImageView::null() {
            return Err(compositor_error("owned Vulkan frame has a null image view"));
        }
        self.draw_image_view(image_view).map_err(|error| {
            compositor_error(format!("Vulkan texture compositor draw failed: {error:?}"))
        })
    }

    pub fn close(&mut self) -> Result<(), vk::Result> {
        if self.closed {
            return Ok(());
        }
        self.wait_idle()?;
        self.closed = true;
        unsafe {
            if self.in_flight != vk::Fence::null() {
                self.device.destroy_fence(self.in_flight, None);
                self.in_flight = vk::Fence::null();
            }
            if self.render_finished != vk::Semaphore::null() {
                self.device.destroy_semaphore(self.render_finished, None);
                self.render_finished = vk::Semaphore::null();
            }
            if self.image_available != vk::Semaphore::null() {
                self.device.destroy_semaphore(self.image_available, None);
                self.image_available = vk::Semaphore::null();
            }
            if self.command_pool != vk::CommandPool::null() {
                self.device.destroy_command_pool(self.command_pool, None);
                self.command_pool = vk::CommandPool::null();
                self.command_buffer = vk::CommandBuffer::null();
            }
        }
        self.destroy_swapchain_dependents();
        unsafe {
            if self.swapchain != vk::SwapchainKHR::null() {
                self.swapchain_loader
                    .destroy_swapchain(self.swapchain, None);
                self.swapchain = vk::SwapchainKHR::null();
            }
            if self.pipeline != vk::Pipeline::null() {
                self.device.destroy_pipeline(self.pipeline, None);
                self.pipeline = vk::Pipeline::null();
            }
            if self.pipeline_layout != vk::PipelineLayout::null() {
                self.device
                    .destroy_pipeline_layout(self.pipeline_layout, None);
                self.pipeline_layout = vk::PipelineLayout::null();
            }
            if self.descriptor_pool != vk::DescriptorPool::null() {
                self.device
                    .destroy_descriptor_pool(self.descriptor_pool, None);
                self.descriptor_pool = vk::DescriptorPool::null();
                self.descriptor_set = vk::DescriptorSet::null();
            }
            if self.sampler != vk::Sampler::null() {
                self.device.destroy_sampler(self.sampler, None);
                self.sampler = vk::Sampler::null();
            }
            if self.descriptor_set_layout != vk::DescriptorSetLayout::null() {
                self.device
                    .destroy_descriptor_set_layout(self.descriptor_set_layout, None);
                self.descriptor_set_layout = vk::DescriptorSetLayout::null();
            }
            if self.render_pass != vk::RenderPass::null() {
                self.device.destroy_render_pass(self.render_pass, None);
                self.render_pass = vk::RenderPass::null();
            }
        }
        Ok(())
    }

    pub(crate) fn draw_image_view(&mut self, image_view: vk::ImageView) -> Result<(), vk::Result> {
        unsafe {
            let device = &self.device;
            device.wait_for_fences(&[self.in_flight], true, u64::MAX)?;

            self.update_descriptor(image_view);
            let image_index = match self.swapchain_loader.acquire_next_image(
                self.swapchain,
                u64::MAX,
                self.image_available,
                vk::Fence::null(),
            ) {
                Ok((image_index, _suboptimal)) => image_index,
                Err(vk::Result::ERROR_OUT_OF_DATE_KHR) => return Ok(()),
                Err(error) => return Err(error),
            };

            self.record(image_index)?;
            device.reset_fences(&[self.in_flight])?;

            let wait_semaphores = [self.image_available];
            let wait_stages = [vk::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT];
            let command_buffers = [self.command_buffer];
            let signal_semaphores = [self.render_finished];
            let submit_info = [vk::SubmitInfo::default()
                .wait_semaphores(&wait_semaphores)
                .wait_dst_stage_mask(&wait_stages)
                .command_buffers(&command_buffers)
                .signal_semaphores(&signal_semaphores)];
            device.queue_submit(self.graphics_queue, &submit_info, self.in_flight)?;
            device.wait_for_fences(&[self.in_flight], true, u64::MAX)?;

            let swapchains = [self.swapchain];
            let image_indices = [image_index];
            let present_info = vk::PresentInfoKHR::default()
                .wait_semaphores(&signal_semaphores)
                .swapchains(&swapchains)
                .image_indices(&image_indices);
            match self
                .swapchain_loader
                .queue_present(self.graphics_queue, &present_info)
            {
                Ok(_) | Err(vk::Result::ERROR_OUT_OF_DATE_KHR) => Ok(()),
                Err(error) => Err(error),
            }
        }
    }

    fn create_swapchain(
        &mut self,
        viewport: Viewport,
        old_swapchain: vk::SwapchainKHR,
    ) -> Result<(), vk::Result> {
        unsafe {
            let capabilities = self
                .surface_loader
                .get_physical_device_surface_capabilities(self.physical_device, self.surface)?;
            let formats = &self
                .surface_loader
                .get_physical_device_surface_formats(self.physical_device, self.surface)?;
            let surface_format = choose_surface_format(formats);
            let swapchain_format = surface_format.format;
            let extent = choose_extent(capabilities, viewport);
            let mut image_count = capabilities.min_image_count + 1;
            if capabilities.max_image_count > 0 && image_count > capabilities.max_image_count {
                image_count = capabilities.max_image_count;
            }
            let create_info = vk::SwapchainCreateInfoKHR::default()
                .surface(self.surface)
                .min_image_count(image_count)
                .image_format(swapchain_format)
                .image_color_space(surface_format.color_space)
                .image_extent(extent)
                .image_array_layers(1)
                .image_usage(vk::ImageUsageFlags::COLOR_ATTACHMENT)
                .image_sharing_mode(vk::SharingMode::EXCLUSIVE)
                .pre_transform(capabilities.current_transform)
                .composite_alpha(vk::CompositeAlphaFlagsKHR::OPAQUE)
                .present_mode(vk::PresentModeKHR::FIFO)
                .clipped(true)
                .old_swapchain(old_swapchain);
            let swapchain = self.swapchain_loader.create_swapchain(&create_info, None)?;
            let images = match self.swapchain_loader.get_swapchain_images(swapchain) {
                Ok(images) => images,
                Err(error) => {
                    self.swapchain_loader.destroy_swapchain(swapchain, None);
                    return Err(error);
                }
            };
            let mut image_views = Vec::with_capacity(images.len());
            for image in images {
                match create_image_view(&self.device, image, swapchain_format) {
                    Ok(view) => image_views.push(view),
                    Err(error) => {
                        for view in image_views {
                            self.device.destroy_image_view(view, None);
                        }
                        self.swapchain_loader.destroy_swapchain(swapchain, None);
                        return Err(error);
                    }
                }
            }
            self.swapchain = swapchain;
            self.swapchain_format = swapchain_format;
            self.extent = extent;
            self.image_views = image_views;
            Ok(())
        }
    }

    fn create_render_pass(&mut self) -> Result<(), vk::Result> {
        let attachment = vk::AttachmentDescription::default()
            .format(self.swapchain_format)
            .samples(vk::SampleCountFlags::TYPE_1)
            .load_op(vk::AttachmentLoadOp::CLEAR)
            .store_op(vk::AttachmentStoreOp::STORE)
            .stencil_load_op(vk::AttachmentLoadOp::DONT_CARE)
            .stencil_store_op(vk::AttachmentStoreOp::DONT_CARE)
            .initial_layout(vk::ImageLayout::UNDEFINED)
            .final_layout(vk::ImageLayout::PRESENT_SRC_KHR);
        let color_ref = vk::AttachmentReference::default()
            .attachment(0)
            .layout(vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL);
        let color_refs = [color_ref];
        let subpass = vk::SubpassDescription::default()
            .pipeline_bind_point(vk::PipelineBindPoint::GRAPHICS)
            .color_attachments(&color_refs);
        let dependency = vk::SubpassDependency::default()
            .src_subpass(vk::SUBPASS_EXTERNAL)
            .dst_subpass(0)
            .src_stage_mask(vk::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT)
            .dst_stage_mask(vk::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT)
            .dst_access_mask(vk::AccessFlags::COLOR_ATTACHMENT_WRITE);
        let attachments = [attachment];
        let subpasses = [subpass];
        let dependencies = [dependency];
        let create_info = vk::RenderPassCreateInfo::default()
            .attachments(&attachments)
            .subpasses(&subpasses)
            .dependencies(&dependencies);
        unsafe {
            self.render_pass = self.device.create_render_pass(&create_info, None)?;
        }
        Ok(())
    }

    fn create_descriptor_state(&mut self) -> Result<(), vk::Result> {
        unsafe {
            let binding = vk::DescriptorSetLayoutBinding::default()
                .binding(0)
                .descriptor_type(vk::DescriptorType::COMBINED_IMAGE_SAMPLER)
                .descriptor_count(1)
                .stage_flags(vk::ShaderStageFlags::FRAGMENT);
            let bindings = [binding];
            let layout_info = vk::DescriptorSetLayoutCreateInfo::default().bindings(&bindings);
            self.descriptor_set_layout = self
                .device
                .create_descriptor_set_layout(&layout_info, None)?;

            let sampler_info = vk::SamplerCreateInfo::default()
                .mag_filter(vk::Filter::LINEAR)
                .min_filter(vk::Filter::LINEAR)
                .mipmap_mode(vk::SamplerMipmapMode::LINEAR)
                .address_mode_u(vk::SamplerAddressMode::CLAMP_TO_EDGE)
                .address_mode_v(vk::SamplerAddressMode::CLAMP_TO_EDGE)
                .address_mode_w(vk::SamplerAddressMode::CLAMP_TO_EDGE)
                .max_anisotropy(1.0)
                .compare_op(vk::CompareOp::ALWAYS)
                .border_color(vk::BorderColor::INT_OPAQUE_BLACK);
            self.sampler = self.device.create_sampler(&sampler_info, None)?;

            let pool_size = vk::DescriptorPoolSize::default()
                .ty(vk::DescriptorType::COMBINED_IMAGE_SAMPLER)
                .descriptor_count(1);
            let pool_sizes = [pool_size];
            let pool_info = vk::DescriptorPoolCreateInfo::default()
                .max_sets(1)
                .pool_sizes(&pool_sizes);
            self.descriptor_pool = self.device.create_descriptor_pool(&pool_info, None)?;
            let layouts = [self.descriptor_set_layout];
            let alloc_info = vk::DescriptorSetAllocateInfo::default()
                .descriptor_pool(self.descriptor_pool)
                .set_layouts(&layouts);
            self.descriptor_set = self.device.allocate_descriptor_sets(&alloc_info)?[0];
            Ok(())
        }
    }

    fn create_pipeline(&mut self) -> Result<(), vk::Result> {
        unsafe {
            let vert = ShaderModuleGuard::new(&self.device, VERT_SHADER)?;
            let frag = ShaderModuleGuard::new(&self.device, FRAG_SHADER)?;
            let main = c"main";
            let stages = [
                vk::PipelineShaderStageCreateInfo::default()
                    .stage(vk::ShaderStageFlags::VERTEX)
                    .module(vert.module)
                    .name(main),
                vk::PipelineShaderStageCreateInfo::default()
                    .stage(vk::ShaderStageFlags::FRAGMENT)
                    .module(frag.module)
                    .name(main),
            ];
            let vertex_input = vk::PipelineVertexInputStateCreateInfo::default();
            let input_assembly = vk::PipelineInputAssemblyStateCreateInfo::default()
                .topology(vk::PrimitiveTopology::TRIANGLE_LIST);
            let viewport_state = vk::PipelineViewportStateCreateInfo::default()
                .viewport_count(1)
                .scissor_count(1);
            let rasterizer = vk::PipelineRasterizationStateCreateInfo::default()
                .polygon_mode(vk::PolygonMode::FILL)
                .cull_mode(vk::CullModeFlags::NONE)
                .front_face(vk::FrontFace::COUNTER_CLOCKWISE)
                .line_width(1.0);
            let multisample = vk::PipelineMultisampleStateCreateInfo::default()
                .rasterization_samples(vk::SampleCountFlags::TYPE_1);
            let color_blend_attachment = vk::PipelineColorBlendAttachmentState::default()
                .color_write_mask(
                    vk::ColorComponentFlags::R
                        | vk::ColorComponentFlags::G
                        | vk::ColorComponentFlags::B
                        | vk::ColorComponentFlags::A,
                );
            let color_blend_attachments = [color_blend_attachment];
            let color_blend = vk::PipelineColorBlendStateCreateInfo::default()
                .attachments(&color_blend_attachments);
            let dynamic_states = [vk::DynamicState::VIEWPORT, vk::DynamicState::SCISSOR];
            let dynamic_state =
                vk::PipelineDynamicStateCreateInfo::default().dynamic_states(&dynamic_states);
            let set_layouts = [self.descriptor_set_layout];
            let layout_info = vk::PipelineLayoutCreateInfo::default().set_layouts(&set_layouts);
            self.pipeline_layout = self.device.create_pipeline_layout(&layout_info, None)?;
            let pipeline_info = [vk::GraphicsPipelineCreateInfo::default()
                .stages(&stages)
                .vertex_input_state(&vertex_input)
                .input_assembly_state(&input_assembly)
                .viewport_state(&viewport_state)
                .rasterization_state(&rasterizer)
                .multisample_state(&multisample)
                .color_blend_state(&color_blend)
                .dynamic_state(&dynamic_state)
                .layout(self.pipeline_layout)
                .render_pass(self.render_pass)
                .subpass(0)];
            self.pipeline = self
                .device
                .create_graphics_pipelines(vk::PipelineCache::null(), &pipeline_info, None)
                .map_err(|(_, error)| error)?[0];
            Ok(())
        }
    }

    fn create_framebuffers(&mut self) -> Result<(), vk::Result> {
        unsafe {
            let mut framebuffers = Vec::with_capacity(self.image_views.len());
            for view in &self.image_views {
                let attachments = [*view];
                let info = vk::FramebufferCreateInfo::default()
                    .render_pass(self.render_pass)
                    .attachments(&attachments)
                    .width(self.extent.width)
                    .height(self.extent.height)
                    .layers(1);
                match self.device.create_framebuffer(&info, None) {
                    Ok(framebuffer) => framebuffers.push(framebuffer),
                    Err(error) => {
                        for framebuffer in framebuffers {
                            self.device.destroy_framebuffer(framebuffer, None);
                        }
                        return Err(error);
                    }
                }
            }
            self.framebuffers = framebuffers;
            Ok(())
        }
    }

    fn create_commands(&mut self) -> Result<(), vk::Result> {
        unsafe {
            let pool_info = vk::CommandPoolCreateInfo::default()
                .flags(vk::CommandPoolCreateFlags::RESET_COMMAND_BUFFER)
                .queue_family_index(self.graphics_queue_family_index);
            self.command_pool = self.device.create_command_pool(&pool_info, None)?;
            let alloc_info = vk::CommandBufferAllocateInfo::default()
                .command_pool(self.command_pool)
                .level(vk::CommandBufferLevel::PRIMARY)
                .command_buffer_count(1);
            self.command_buffer = self.device.allocate_command_buffers(&alloc_info)?[0];
            let semaphore_info = vk::SemaphoreCreateInfo::default();
            self.image_available = self.device.create_semaphore(&semaphore_info, None)?;
            self.render_finished = self.device.create_semaphore(&semaphore_info, None)?;
            let fence_info = vk::FenceCreateInfo::default().flags(vk::FenceCreateFlags::SIGNALED);
            self.in_flight = self.device.create_fence(&fence_info, None)?;
            Ok(())
        }
    }

    fn update_descriptor(&self, image_view: vk::ImageView) {
        let image_info = [vk::DescriptorImageInfo::default()
            .sampler(self.sampler)
            .image_view(image_view)
            .image_layout(vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL)];
        let write = [vk::WriteDescriptorSet::default()
            .dst_set(self.descriptor_set)
            .dst_binding(0)
            .descriptor_type(vk::DescriptorType::COMBINED_IMAGE_SAMPLER)
            .image_info(&image_info)];
        unsafe { self.device.update_descriptor_sets(&write, &[]) };
    }

    fn record(&self, image_index: u32) -> Result<(), vk::Result> {
        unsafe {
            let device = &self.device;
            device
                .reset_command_buffer(self.command_buffer, vk::CommandBufferResetFlags::empty())?;
            let begin_info = vk::CommandBufferBeginInfo::default();
            device.begin_command_buffer(self.command_buffer, &begin_info)?;
            let clear = [vk::ClearValue {
                color: vk::ClearColorValue {
                    float32: [0.08, 0.09, 0.11, 1.0],
                },
            }];
            let render_area = vk::Rect2D::default().extent(self.extent);
            let render_pass_info = vk::RenderPassBeginInfo::default()
                .render_pass(self.render_pass)
                .framebuffer(self.framebuffers[image_index as usize])
                .render_area(render_area)
                .clear_values(&clear);
            device.cmd_begin_render_pass(
                self.command_buffer,
                &render_pass_info,
                vk::SubpassContents::INLINE,
            );
            let viewport = [vk::Viewport {
                x: 0.0,
                y: 0.0,
                width: self.extent.width as f32,
                height: self.extent.height as f32,
                min_depth: 0.0,
                max_depth: 1.0,
            }];
            let scissor = [render_area];
            device.cmd_bind_pipeline(
                self.command_buffer,
                vk::PipelineBindPoint::GRAPHICS,
                self.pipeline,
            );
            device.cmd_set_viewport(self.command_buffer, 0, &viewport);
            device.cmd_set_scissor(self.command_buffer, 0, &scissor);
            device.cmd_bind_descriptor_sets(
                self.command_buffer,
                vk::PipelineBindPoint::GRAPHICS,
                self.pipeline_layout,
                0,
                &[self.descriptor_set],
                &[],
            );
            device.cmd_draw(self.command_buffer, 3, 1, 0, 0);
            device.cmd_end_render_pass(self.command_buffer);
            device.end_command_buffer(self.command_buffer)?;
            Ok(())
        }
    }

    fn wait_idle(&self) -> Result<(), vk::Result> {
        unsafe { self.device.device_wait_idle() }
    }

    fn destroy_swapchain_dependents(&mut self) {
        let device = &self.device;
        for framebuffer in self.framebuffers.drain(..) {
            // SAFETY: framebuffer belongs to this live device and is no longer in use after waits.
            unsafe { device.destroy_framebuffer(framebuffer, None) };
        }
        for view in self.image_views.drain(..) {
            // SAFETY: image view belongs to this live device and no framebuffer references it now.
            unsafe { device.destroy_image_view(view, None) };
        }
    }
}

impl Drop for VulkanTextureCompositor {
    fn drop(&mut self) {
        let _ = self.close();
    }
}

fn choose_surface_format(formats: &[vk::SurfaceFormatKHR]) -> vk::SurfaceFormatKHR {
    formats
        .iter()
        .copied()
        .find(|format| {
            matches!(
                format.format,
                vk::Format::B8G8R8A8_UNORM | vk::Format::R8G8B8A8_UNORM
            ) && format.color_space == vk::ColorSpaceKHR::SRGB_NONLINEAR
        })
        .unwrap_or_else(|| formats[0])
}

fn choose_extent(capabilities: vk::SurfaceCapabilitiesKHR, viewport: Viewport) -> vk::Extent2D {
    if capabilities.current_extent.width != u32::MAX {
        capabilities.current_extent
    } else {
        vk::Extent2D {
            width: viewport.physical_width.clamp(
                capabilities.min_image_extent.width,
                capabilities.max_image_extent.width,
            ),
            height: viewport.physical_height.clamp(
                capabilities.min_image_extent.height,
                capabilities.max_image_extent.height,
            ),
        }
    }
}

fn create_image_view(
    device: &ash::Device,
    image: vk::Image,
    format: vk::Format,
) -> Result<vk::ImageView, vk::Result> {
    let create_info = vk::ImageViewCreateInfo::default()
        .image(image)
        .view_type(vk::ImageViewType::TYPE_2D)
        .format(format)
        .subresource_range(
            vk::ImageSubresourceRange::default()
                .aspect_mask(vk::ImageAspectFlags::COLOR)
                .level_count(1)
                .layer_count(1),
        );
    // SAFETY: create_info references a swapchain image from the same live device.
    unsafe { device.create_image_view(&create_info, None) }
}

struct ShaderModuleGuard<'device> {
    device: &'device ash::Device,
    module: vk::ShaderModule,
}

impl<'device> ShaderModuleGuard<'device> {
    fn new(device: &'device ash::Device, bytes: &[u8]) -> Result<Self, vk::Result> {
        let words = ash::util::read_spv(&mut Cursor::new(bytes))
            .map_err(|_| vk::Result::ERROR_INVALID_SHADER_NV)?;
        let create_info = vk::ShaderModuleCreateInfo::default().code(&words);
        // SAFETY: create_info points at immutable SPIR-V words for the duration of the call.
        let module = unsafe { device.create_shader_module(&create_info, None)? };
        Ok(Self { device, module })
    }
}

impl Drop for ShaderModuleGuard<'_> {
    fn drop(&mut self) {
        // SAFETY: module belongs to this live device and is no longer referenced after pipeline creation returns.
        unsafe { self.device.destroy_shader_module(self.module, None) };
    }
}

fn compositor_error(message: impl Into<String>) -> Error {
    Error::new(ErrorKind::NativeError, None, message)
}

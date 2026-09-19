//! A MapLibre Native layer plugin that renders a procedural location
//! indicator ("location-puck" layers) around each point feature of a GeoJSON
//! or vector source. All shapes are drawn analytically in fragment shaders,
//! so the layer needs no images.

const std = @import("std");
const c = @import("maplibre_native_c");

const allocator = std.heap.c_allocator;

const layer_type_name = "location-puck";

// One shader per component; each has one drawable-scope uniform block with a
// globally distinct id.
const shader_accuracy = "accuracy";
const shader_sector = "sector";
const shader_shadow = "shadow";
const shader_pulse = "pulse";
const shader_arrow = "arrow";
const shader_puck = "puck";

const accuracy_uniform_id = 0;
const sector_uniform_id = 1;
const shadow_uniform_id = 2;
const pulse_uniform_id = 3;
const arrow_uniform_id = 4;
const puck_uniform_id = 5;

// Drawable keys, in render order (bottom to top).
const component_accuracy = 0;
const component_sector = 1;
const component_shadow = 2;
const component_pulse = 3;
const component_arrow = 4;
const component_puck = 5;

const drawable_accuracy = 1;
const drawable_sector = 2;
const drawable_shadow = 3;
const drawable_pulse = 4;
const drawable_arrow = 5;
const drawable_puck = 6;

const max_points_per_tile = 256;
const quads_per_point = 6;
const vertices_per_quad = 4;
const indices_per_quad = 6;

const earth_circumference_meters: f32 = 40075016.68557849;

// Position moves of up to one quarter tile animate over this fixed duration;
// anything larger teleports.

/// One vertex stream record. `scale` converts meters to frame units for the
/// accuracy circle, baked per frame. `prev_pos` and `start_time` drive the
/// vertex shader's position easing; the frame path emits prev == target, so
/// host transitions animate the position instead.
const Vertex = extern struct {
    packed_pos: [2]i16,
    prev_pos: [2]i16,
    scale: f32,
    start_time: f32,
};

comptime {
    std.debug.assert(@sizeOf(Vertex) == 16);
    std.debug.assert(@offsetOf(Vertex, "prev_pos") == 4);
    std.debug.assert(@offsetOf(Vertex, "scale") == 8);
    std.debug.assert(@offsetOf(Vertex, "start_time") == 12);
}

// std140 uniform blocks. The host writes the bound paint properties into the
// property and interpolation slots after update_uniform_block returns, so the
// callback writes only the matrix/camera/frame header and zeroes the rest.
const AccuracyUBO = extern struct {
    matrix: [16]f32,
    camera: [4]f32,
    frame: [4]f32,
    accuracy_color: [4]f32,
    accuracy_border_color: [4]f32,
    accuracy_radius: f32,
    accuracy_border_width: f32,
    pad0: f32,
    accuracy_radius_t: f32,
    accuracy_border_width_t: f32,
    accuracy_color_t: f32,
    accuracy_border_color_t: f32,
    pad1: f32,
    pad2: f32,
    pad3: f32,
    pad4: f32,
    pad5: f32,
};

const SectorUBO = extern struct {
    matrix: [16]f32,
    camera: [4]f32,
    frame: [4]f32,
    bearing_accuracy_color: [4]f32,
    bearing: f32,
    bearing_accuracy: f32,
    bearing_accuracy_radius: f32,
    bearing_visible: f32,
    bearing_t: f32,
    bearing_accuracy_t: f32,
    bearing_accuracy_radius_t: f32,
    bearing_visible_t: f32,
    bearing_accuracy_color_t: f32,
    pad1: f32,
    pad2: f32,
    pad3: f32,
};

const ShadowUBO = extern struct {
    matrix: [16]f32,
    camera: [4]f32,
    frame: [4]f32,
    shadow_color: [4]f32,
    shadow_radius: f32,
    shadow_radius_t: f32,
    shadow_color_t: f32,
    pad0: f32,
};

const PulseUBO = extern struct {
    matrix: [16]f32,
    camera: [4]f32,
    frame: [4]f32,
    pulse_color: [4]f32,
    puck_radius: f32,
    pulse_radius: f32,
    pulse_period: f32,
    puck_radius_t: f32,
    pulse_radius_t: f32,
    pulse_period_t: f32,
    pulse_color_t: f32,
    pad0: f32,
};

const ArrowUBO = extern struct {
    matrix: [16]f32,
    camera: [4]f32,
    frame: [4]f32,
    bearing_arrow_color: [4]f32,
    bearing: f32,
    puck_radius: f32,
    puck_border_width: f32,
    bearing_visible: f32,
    bearing_t: f32,
    puck_radius_t: f32,
    puck_border_width_t: f32,
    bearing_visible_t: f32,
    bearing_arrow_color_t: f32,
    pad1: f32,
    pad2: f32,
    pad3: f32,
};

const PuckUBO = extern struct {
    matrix: [16]f32,
    camera: [4]f32,
    frame: [4]f32,
    puck_color: [4]f32,
    puck_border_color: [4]f32,
    puck_radius: f32,
    puck_border_width: f32,
    puck_radius_t: f32,
    puck_border_width_t: f32,
    puck_color_t: f32,
    puck_border_color_t: f32,
    pad0: f32,
    pad1: f32,
};

comptime {
    std.debug.assert(@sizeOf(AccuracyUBO) == 176);
    std.debug.assert(@offsetOf(AccuracyUBO, "accuracy_color") == 96);
    std.debug.assert(@offsetOf(AccuracyUBO, "accuracy_border_color") == 112);
    std.debug.assert(@offsetOf(AccuracyUBO, "accuracy_radius") == 128);
    std.debug.assert(@offsetOf(AccuracyUBO, "accuracy_border_width") == 132);
    std.debug.assert(@offsetOf(AccuracyUBO, "accuracy_radius_t") == 140);
    std.debug.assert(@offsetOf(AccuracyUBO, "accuracy_border_color_t") == 152);

    std.debug.assert(@sizeOf(SectorUBO) == 160);
    std.debug.assert(@offsetOf(SectorUBO, "bearing_accuracy_color") == 96);
    std.debug.assert(@offsetOf(SectorUBO, "bearing") == 112);
    std.debug.assert(@offsetOf(SectorUBO, "bearing_accuracy_radius") == 120);
    std.debug.assert(@offsetOf(SectorUBO, "bearing_visible") == 124);
    std.debug.assert(@offsetOf(SectorUBO, "bearing_t") == 128);
    std.debug.assert(@offsetOf(SectorUBO, "bearing_visible_t") == 140);
    std.debug.assert(@offsetOf(SectorUBO, "bearing_accuracy_color_t") == 144);

    std.debug.assert(@sizeOf(ShadowUBO) == 128);
    std.debug.assert(@offsetOf(ShadowUBO, "shadow_color") == 96);
    std.debug.assert(@offsetOf(ShadowUBO, "shadow_radius") == 112);
    std.debug.assert(@offsetOf(ShadowUBO, "shadow_radius_t") == 116);
    std.debug.assert(@offsetOf(ShadowUBO, "shadow_color_t") == 120);

    std.debug.assert(@sizeOf(PulseUBO) == 144);
    std.debug.assert(@offsetOf(PulseUBO, "pulse_color") == 96);
    std.debug.assert(@offsetOf(PulseUBO, "puck_radius") == 112);
    std.debug.assert(@offsetOf(PulseUBO, "pulse_period") == 120);
    std.debug.assert(@offsetOf(PulseUBO, "puck_radius_t") == 124);
    std.debug.assert(@offsetOf(PulseUBO, "pulse_color_t") == 136);

    std.debug.assert(@sizeOf(ArrowUBO) == 160);
    std.debug.assert(@offsetOf(ArrowUBO, "bearing_arrow_color") == 96);
    std.debug.assert(@offsetOf(ArrowUBO, "bearing") == 112);
    std.debug.assert(@offsetOf(ArrowUBO, "puck_border_width") == 120);
    std.debug.assert(@offsetOf(ArrowUBO, "bearing_visible") == 124);
    std.debug.assert(@offsetOf(ArrowUBO, "bearing_t") == 128);
    std.debug.assert(@offsetOf(ArrowUBO, "bearing_visible_t") == 140);
    std.debug.assert(@offsetOf(ArrowUBO, "bearing_arrow_color_t") == 144);

    std.debug.assert(@sizeOf(PuckUBO) == 160);
    std.debug.assert(@offsetOf(PuckUBO, "puck_color") == 96);
    std.debug.assert(@offsetOf(PuckUBO, "puck_border_color") == 112);
    std.debug.assert(@offsetOf(PuckUBO, "puck_radius") == 128);
    std.debug.assert(@offsetOf(PuckUBO, "puck_border_width") == 132);
    std.debug.assert(@offsetOf(PuckUBO, "puck_radius_t") == 136);
    std.debug.assert(@offsetOf(PuckUBO, "puck_border_color_t") == 148);
}

fn str(comptime s: []const u8) c.mln_plugin_string {
    return .{ .data = s.ptr, .size = s.len };
}

const empty_str = c.mln_plugin_string{ .data = null, .size = 0 };

// --------------------------------------------------------------------------
// Shader source generation. One algorithm per shader, in the three shading
// languages the plugin API accepts; the host prepends the binding macros and,
// per property, an `_IS_UNIFORM` macro that selects the uniform or the
// attribute pair.
// --------------------------------------------------------------------------

fn propertyMacro(comptime name: []const u8) []const u8 {
    @setEvalBranchQuota(100_000);
    comptime var result: []const u8 = "MLN_PLUGIN_PROPERTY_";
    inline for (name) |character| {
        const is_alnum = (character >= '0' and character <= '9') or
            (character >= 'a' and character <= 'z') or (character >= 'A' and character <= 'Z');
        const upper = if (character >= 'a' and character <= 'z') character - ('a' - 'A') else character;
        result = result ++ &[1]u8{if (is_alnum) upper else '_'};
    }
    return result ++ "_IS_UNIFORM";
}

fn guard(comptime property: []const u8, comptime body: []const u8) []const u8 {
    return "#if !" ++ propertyMacro(property) ++ "\n" ++ body ++ "#endif\n";
}

fn num(comptime value: u8) []const u8 {
    return switch (value) {
        // Locations and uniform ids stay below 16 by the ABI's attribute cap.
        inline 0...15 => |v| &[1]u8{'0' + v},
        else => @compileError("shader location out of range"),
    };
}

const GLSL = enum { opengl, vulkan };

fn glslIn(comptime backend: GLSL, comptime location: u8, comptime glsl_type: []const u8, comptime name: []const u8) []const u8 {
    return switch (backend) {
        .opengl => "in " ++ glsl_type ++ " " ++ name ++ ";\n",
        .vulkan => "layout(location = " ++ num(location) ++ ") in " ++ glsl_type ++ " " ++ name ++ ";\n",
    };
}

fn glslOut(comptime backend: GLSL, comptime location: u8, comptime glsl_type: []const u8, comptime name: []const u8) []const u8 {
    return switch (backend) {
        .opengl => "out " ++ glsl_type ++ " " ++ name ++ ";\n",
        .vulkan => "layout(location = " ++ num(location) ++ ") out " ++ glsl_type ++ " " ++ name ++ ";\n",
    };
}

fn glslUbo(comptime backend: GLSL, comptime uniform_id: u8, comptime name: []const u8, comptime members: []const u8) []const u8 {
    return switch (backend) {
        .opengl => "layout(std140) uniform " ++ name ++ " {\n" ++ members ++ "} u;\n",
        .vulkan => "layout(std140, set = DRAWABLE_UBO_SET_INDEX, binding = MLN_PLUGIN_UNIFORM_" ++ num(uniform_id) ++ "_BINDING) uniform " ++
            name ++ " {\n" ++ members ++ "} u;\n",
    };
}

fn glslEvalFloat(
    comptime var_name: []const u8,
    comptime attr_ref: []const u8,
    comptime field: []const u8,
    comptime t_field: []const u8,
    comptime property: []const u8,
) []const u8 {
    return "    float " ++ var_name ++ " = u." ++ field ++ ";\n" ++
        guard(property, "    " ++ var_name ++ " = mix(" ++ attr_ref ++ ".x, " ++ attr_ref ++ ".y, u." ++ t_field ++ ");\n");
}

fn glslEvalColor(
    comptime vec4: []const u8,
    comptime var_name: []const u8,
    comptime min_ref: []const u8,
    comptime max_ref: []const u8,
    comptime field: []const u8,
    comptime t_field: []const u8,
    comptime property: []const u8,
) []const u8 {
    return "    " ++ vec4 ++ " " ++ var_name ++ " = u." ++ field ++ ";\n" ++
        guard(property, "    " ++ var_name ++ " = mix(" ++ min_ref ++ ", " ++ max_ref ++ ", u." ++ t_field ++ ");\n");
}

const glsl_transition =
    // Smoothstep easing over a fixed 300 ms; the hour-wrapped clock needs a
    // single wrap correction.
    "    float dt = u.frame.y - a_start_time;\n" ++
    "    if (dt < 0.0) dt += 3600.0;\n" ++
    "    float t = clamp(dt / 0.3, 0.0, 1.0);\n" ++
    "    t = t * t * (3.0 - 2.0 * t);\n";

const glsl_project =
    "    vec2 center = floor(encoded * 0.5);\n" ++
    "    vec2 corner = (encoded - 2.0 * center) * 2.0 - 1.0;\n" ++
    "    vec2 tile_pos = mix(vec2(a_prev_pos), center, t);\n" ++
    "    vec4 p = u.matrix * vec4(tile_pos, 0.0, 1.0);\n" ++
    "    vec4 q = u.matrix * vec4(tile_pos + vec2(1.0, 0.0), 0.0, 1.0);\n" ++
    "    float px_per_unit = length((q.xy / q.w - p.xy / p.w) * u.camera.zw * 0.5);\n";

const glsl_emit_position =
    "    gl_Position = p + vec4(corner * half_px * u.camera.xy * p.w, 0.0, 0.0);\n";

// The fixed pipeline keeps read-only depth testing; pin the overlay to the
// near plane so the ground never occludes it under pitch. GL NDC z spans
// [-1,1]; Vulkan and Metal span [0,1]. applySurfaceTransform() touches only
// x and y, so the pin survives it.
fn glslPinDepth(comptime backend: GLSL) []const u8 {
    return switch (backend) {
        .opengl => "    gl_Position.z = -gl_Position.w;\n",
        .vulkan => "    gl_Position.z = 0.0;\n",
    };
}

const glsl_sd_triangle =
    "float sdTriangle(vec2 p, vec2 p0, vec2 p1, vec2 p2) {\n" ++
    "    vec2 e0 = p1 - p0;\n" ++
    "    vec2 e1 = p2 - p1;\n" ++
    "    vec2 e2 = p0 - p2;\n" ++
    "    vec2 v0 = p - p0;\n" ++
    "    vec2 v1 = p - p1;\n" ++
    "    vec2 v2 = p - p2;\n" ++
    "    vec2 pq0 = v0 - e0 * clamp(dot(v0, e0) / max(dot(e0, e0), 1e-6), 0.0, 1.0);\n" ++
    "    vec2 pq1 = v1 - e1 * clamp(dot(v1, e1) / max(dot(e1, e1), 1e-6), 0.0, 1.0);\n" ++
    "    vec2 pq2 = v2 - e2 * clamp(dot(v2, e2) / max(dot(e2, e2), 1e-6), 0.0, 1.0);\n" ++
    "    float s = sign(e0.x * e2.y - e0.y * e2.x);\n" ++
    "    vec2 d = min(min(vec2(dot(pq0, pq0), s * (v0.x * e0.y - v0.y * e0.x)),\n" ++
    "                     vec2(dot(pq1, pq1), s * (v1.x * e1.y - v1.y * e1.x))),\n" ++
    "                 vec2(dot(pq2, pq2), s * (v2.x * e2.y - v2.y * e2.x)));\n" ++
    "    return -sqrt(d.x) * sign(d.y);\n" ++
    "}\n";

const accuracy_ubo_members =
    "    mat4 matrix;\n" ++
    "    vec4 camera;\n" ++
    "    vec4 frame;\n" ++
    "    vec4 accuracy_color;\n" ++
    "    vec4 accuracy_border_color;\n" ++
    "    float accuracy_radius;\n" ++
    "    float accuracy_border_width;\n" ++
    "    float ubo_pad0;\n" ++
    "    float accuracy_radius_t;\n" ++
    "    float accuracy_border_width_t;\n" ++
    "    float accuracy_color_t;\n" ++
    "    float accuracy_border_color_t;\n" ++
    "    float ubo_pad1;\n" ++
    "    float ubo_pad2;\n" ++
    "    float ubo_pad3;\n" ++
    "    float ubo_pad4;\n" ++
    "    float ubo_pad5;\n";

const sector_ubo_members =
    "    mat4 matrix;\n" ++
    "    vec4 camera;\n" ++
    "    vec4 frame;\n" ++
    "    vec4 bearing_accuracy_color;\n" ++
    "    float bearing;\n" ++
    "    float bearing_accuracy;\n" ++
    "    float bearing_accuracy_radius;\n" ++
    "    float bearing_visible;\n" ++
    "    float bearing_t;\n" ++
    "    float bearing_accuracy_t;\n" ++
    "    float bearing_accuracy_radius_t;\n" ++
    "    float bearing_visible_t;\n" ++
    "    float bearing_accuracy_color_t;\n" ++
    "    float ubo_pad1;\n" ++
    "    float ubo_pad2;\n" ++
    "    float ubo_pad3;\n";

const shadow_ubo_members =
    "    mat4 matrix;\n" ++
    "    vec4 camera;\n" ++
    "    vec4 frame;\n" ++
    "    vec4 shadow_color;\n" ++
    "    float shadow_radius;\n" ++
    "    float shadow_radius_t;\n" ++
    "    float shadow_color_t;\n" ++
    "    float ubo_pad0;\n";

const pulse_ubo_members =
    "    mat4 matrix;\n" ++
    "    vec4 camera;\n" ++
    "    vec4 frame;\n" ++
    "    vec4 pulse_color;\n" ++
    "    float puck_radius;\n" ++
    "    float pulse_radius;\n" ++
    "    float pulse_period;\n" ++
    "    float puck_radius_t;\n" ++
    "    float pulse_radius_t;\n" ++
    "    float pulse_period_t;\n" ++
    "    float pulse_color_t;\n" ++
    "    float ubo_pad0;\n";

const arrow_ubo_members =
    "    mat4 matrix;\n" ++
    "    vec4 camera;\n" ++
    "    vec4 frame;\n" ++
    "    vec4 bearing_arrow_color;\n" ++
    "    float bearing;\n" ++
    "    float puck_radius;\n" ++
    "    float puck_border_width;\n" ++
    "    float bearing_visible;\n" ++
    "    float bearing_t;\n" ++
    "    float puck_radius_t;\n" ++
    "    float puck_border_width_t;\n" ++
    "    float bearing_visible_t;\n" ++
    "    float bearing_arrow_color_t;\n" ++
    "    float ubo_pad1;\n" ++
    "    float ubo_pad2;\n" ++
    "    float ubo_pad3;\n";

const puck_ubo_members =
    "    mat4 matrix;\n" ++
    "    vec4 camera;\n" ++
    "    vec4 frame;\n" ++
    "    vec4 puck_color;\n" ++
    "    vec4 puck_border_color;\n" ++
    "    float puck_radius;\n" ++
    "    float puck_border_width;\n" ++
    "    float puck_radius_t;\n" ++
    "    float puck_border_width_t;\n" ++
    "    float puck_color_t;\n" ++
    "    float puck_border_color_t;\n" ++
    "    float ubo_pad0;\n";

// Shared vertex prologue for every shader: transition, projection, half-size.
// Each shader appends its half-size expression and varying writes.
fn glslVertexPrologue(comptime backend: GLSL) []const u8 {
    return "    vec2 encoded = " ++ (if (backend == .vulkan) "vec2(a_pos)" else "a_pos") ++ ";\n" ++
        glsl_transition ++
        glsl_project;
}

fn glslVertexEpilogue(comptime backend: GLSL) []const u8 {
    return glsl_emit_position ++
        glslPinDepth(backend) ++
        (if (backend == .vulkan) "    applySurfaceTransform();\n" else "");
}

fn accuracyVertexGlsl(comptime backend: GLSL) []const u8 {
    return glslIn(backend, 0, if (backend == .vulkan) "ivec2" else "vec2", "a_pos") ++
        glslIn(backend, 1, if (backend == .vulkan) "ivec2" else "vec2", "a_prev_pos") ++
        glslIn(backend, 2, "float", "a_start_time") ++
        glslIn(backend, 3, "float", "a_scale") ++
        guard("accuracy-radius", glslIn(backend, 4, "vec2", "a_accuracy_radius")) ++
        guard("accuracy-border-width", glslIn(backend, 5, "vec2", "a_accuracy_border_width")) ++
        guard("accuracy-color", glslIn(backend, 6, "vec4", "a_accuracy_color_min") ++ glslIn(backend, 7, "vec4", "a_accuracy_color_max")) ++
        guard("accuracy-border-color", glslIn(backend, 8, "vec4", "a_accuracy_border_color_min") ++ glslIn(backend, 9, "vec4", "a_accuracy_border_color_max")) ++
        glslUbo(backend, accuracy_uniform_id, "AccuracyUBO", accuracy_ubo_members) ++
        glslOut(backend, 0, "vec2", "v_px") ++
        glslOut(backend, 1, "vec4", "v_fill") ++
        glslOut(backend, 2, "vec4", "v_border") ++
        glslOut(backend, 3, "float", "v_radius_px") ++
        "void main() {\n" ++
        glslEvalFloat("accuracy_radius", "a_accuracy_radius", "accuracy_radius", "accuracy_radius_t", "accuracy-radius") ++
        glslEvalFloat("accuracy_border_width", "a_accuracy_border_width", "accuracy_border_width", "accuracy_border_width_t", "accuracy-border-width") ++
        glslEvalColor("vec4", "accuracy_color", "a_accuracy_color_min", "a_accuracy_color_max", "accuracy_color", "accuracy_color_t", "accuracy-color") ++
        glslEvalColor("vec4", "accuracy_border_color", "a_accuracy_border_color_min", "a_accuracy_border_color_max", "accuracy_border_color", "accuracy_border_color_t", "accuracy-border-color") ++
        glslVertexPrologue(backend) ++
        "    gl_Position = p;\n" ++
        glslPinDepth(backend) ++
        (if (backend == .vulkan) "    applySurfaceTransform();\n" else "") ++
        "    v_px = corner;\n" ++
        "    v_fill = accuracy_color;\n" ++
        "    v_border = accuracy_border_color;\n" ++
        "    v_radius_px = accuracy_radius * px_per_unit / max(a_scale, 1e-6);\n" ++
        "}\n";
}

fn accuracyFragmentGlsl(comptime backend: GLSL) []const u8 {
    return glslIn(backend, 0, "vec2", "v_px") ++
        glslIn(backend, 1, "vec4", "v_fill") ++
        glslIn(backend, 2, "vec4", "v_border") ++
        glslIn(backend, 3, "float", "v_radius_px") ++
        (if (backend == .vulkan) "layout(location = 0) out vec4 fragColor;\n" else "") ++
        "void main() {\n" ++
        // v_px carries the fill (-1) or border (+1) flag from the packed
        // corner slot; the fan and strip antialias by geometry density.
        "    float gate = smoothstep(0.0, 1.0, v_radius_px);\n" ++
        "    fragColor = mix(v_fill, v_border, step(0.0, v_px.x)) * gate;\n" ++
        "}\n";
}

fn sectorVertexGlsl(comptime backend: GLSL) []const u8 {
    return glslIn(backend, 0, if (backend == .vulkan) "ivec2" else "vec2", "a_pos") ++
        glslIn(backend, 1, if (backend == .vulkan) "ivec2" else "vec2", "a_prev_pos") ++
        glslIn(backend, 2, "float", "a_start_time") ++
        guard("bearing", glslIn(backend, 3, "vec2", "a_bearing")) ++
        guard("bearing-accuracy", glslIn(backend, 4, "vec2", "a_bearing_accuracy")) ++
        guard("bearing-accuracy-radius", glslIn(backend, 5, "vec2", "a_bearing_accuracy_radius")) ++
        guard("bearing-visible", glslIn(backend, 6, "vec2", "a_bearing_visible")) ++
        guard("bearing-accuracy-color", glslIn(backend, 7, "vec4", "a_bearing_accuracy_color_min") ++ glslIn(backend, 8, "vec4", "a_bearing_accuracy_color_max")) ++
        glslUbo(backend, sector_uniform_id, "SectorUBO", sector_ubo_members) ++
        glslOut(backend, 0, "vec2", "v_px") ++
        glslOut(backend, 1, "vec2", "v_dir") ++
        glslOut(backend, 2, "vec4", "v_color") ++
        glslOut(backend, 3, "float", "v_half_width") ++
        glslOut(backend, 4, "float", "v_radius") ++
        glslOut(backend, 5, "float", "v_visible") ++
        "void main() {\n" ++
        glslEvalFloat("bearing", "a_bearing", "bearing", "bearing_t", "bearing") ++
        glslEvalFloat("bearing_accuracy", "a_bearing_accuracy", "bearing_accuracy", "bearing_accuracy_t", "bearing-accuracy") ++
        glslEvalFloat("bearing_accuracy_radius", "a_bearing_accuracy_radius", "bearing_accuracy_radius", "bearing_accuracy_radius_t", "bearing-accuracy-radius") ++
        glslEvalFloat("bearing_visible", "a_bearing_visible", "bearing_visible", "bearing_visible_t", "bearing-visible") ++
        glslEvalColor("vec4", "bearing_accuracy_color", "a_bearing_accuracy_color_min", "a_bearing_accuracy_color_max", "bearing_accuracy_color", "bearing_accuracy_color_t", "bearing-accuracy-color") ++
        glslVertexPrologue(backend) ++
        "    float half_px = bearing_accuracy_radius + 1.5;\n" ++
        glslVertexEpilogue(backend) ++
        "    v_dir = u.frame.zw;\n" ++
        "    v_half_width = radians(bearing_accuracy);\n" ++
        "    v_radius = bearing_accuracy_radius;\n" ++
        "    v_visible = bearing_visible;\n" ++
        "    v_color = bearing_accuracy_color;\n" ++
        "    v_px = corner * half_px;\n" ++
        "}\n";
}

fn sectorFragmentGlsl(comptime backend: GLSL) []const u8 {
    return glslIn(backend, 0, "vec2", "v_px") ++
        glslIn(backend, 1, "vec2", "v_dir") ++
        glslIn(backend, 2, "vec4", "v_color") ++
        glslIn(backend, 3, "float", "v_half_width") ++
        glslIn(backend, 4, "float", "v_radius") ++
        glslIn(backend, 5, "float", "v_visible") ++
        (if (backend == .vulkan) "layout(location = 0) out vec4 fragColor;\n" else "") ++
        "void main() {\n" ++
        // The core location indicator's fade: a full-radius radial gradient
        // and an angular feather derived from the pixel footprint.
        "    float r = length(v_px);\n" ++
        "    float rn = r / max(v_radius, 1e-4);\n" ++
        "    float angle = acos(clamp(dot(v_px / max(r, 1e-4), v_dir), -1.0, 1.0));\n" ++
        "    float feather = length(fwidth(v_px)) / max(r, 1e-4);\n" ++
        "    float angular = v_half_width >= 3.14159265 ? 1.0 :\n" ++
        "        1.0 - smoothstep(v_half_width - feather, v_half_width + feather, angle);\n" ++
        "    float opacity = angular * (1.0 - smoothstep(0.0, 1.0, rn));\n" ++
        "    float gate = smoothstep(0.0, 1.0, v_radius) * step(1e-4, v_half_width) * v_visible;\n" ++
        "    fragColor = v_color * opacity * gate;\n" ++
        "}\n";
}

fn shadowVertexGlsl(comptime backend: GLSL) []const u8 {
    return glslIn(backend, 0, if (backend == .vulkan) "ivec2" else "vec2", "a_pos") ++
        glslIn(backend, 1, if (backend == .vulkan) "ivec2" else "vec2", "a_prev_pos") ++
        glslIn(backend, 2, "float", "a_start_time") ++
        guard("shadow-radius", glslIn(backend, 3, "vec2", "a_shadow_radius")) ++
        guard("shadow-color", glslIn(backend, 4, "vec4", "a_shadow_color_min") ++ glslIn(backend, 5, "vec4", "a_shadow_color_max")) ++
        glslUbo(backend, shadow_uniform_id, "ShadowUBO", shadow_ubo_members) ++
        glslOut(backend, 0, "vec2", "v_px") ++
        glslOut(backend, 1, "vec4", "v_color") ++
        glslOut(backend, 2, "float", "v_radius") ++
        "void main() {\n" ++
        glslEvalFloat("shadow_radius", "a_shadow_radius", "shadow_radius", "shadow_radius_t", "shadow-radius") ++
        glslEvalColor("vec4", "shadow_color", "a_shadow_color_min", "a_shadow_color_max", "shadow_color", "shadow_color_t", "shadow-color") ++
        glslVertexPrologue(backend) ++
        "    float half_px = shadow_radius + 1.5;\n" ++
        glslVertexEpilogue(backend) ++
        "    v_px = corner * half_px;\n" ++
        "    v_color = shadow_color;\n" ++
        "    v_radius = shadow_radius;\n" ++
        "}\n";
}

fn shadowFragmentGlsl(comptime backend: GLSL) []const u8 {
    return glslIn(backend, 0, "vec2", "v_px") ++
        glslIn(backend, 1, "vec4", "v_color") ++
        glslIn(backend, 2, "float", "v_radius") ++
        (if (backend == .vulkan) "layout(location = 0) out vec4 fragColor;\n" else "") ++
        "void main() {\n" ++
        "    float d = length(v_px * vec2(1.0, 1.6));\n" ++
        "    float alpha = 1.0 - smoothstep(v_radius - 0.5, v_radius + 0.5, d);\n" ++
        "    fragColor = v_color * alpha * smoothstep(0.0, 1.0, v_radius);\n" ++
        "}\n";
}

fn pulseVertexGlsl(comptime backend: GLSL) []const u8 {
    return glslIn(backend, 0, if (backend == .vulkan) "ivec2" else "vec2", "a_pos") ++
        glslIn(backend, 1, if (backend == .vulkan) "ivec2" else "vec2", "a_prev_pos") ++
        glslIn(backend, 2, "float", "a_start_time") ++
        guard("puck-radius", glslIn(backend, 3, "vec2", "a_puck_radius")) ++
        guard("pulse-radius", glslIn(backend, 4, "vec2", "a_pulse_radius")) ++
        guard("pulse-period", glslIn(backend, 5, "vec2", "a_pulse_period")) ++
        guard("pulse-color", glslIn(backend, 6, "vec4", "a_pulse_color_min") ++ glslIn(backend, 7, "vec4", "a_pulse_color_max")) ++
        glslUbo(backend, pulse_uniform_id, "PulseUBO", pulse_ubo_members) ++
        glslOut(backend, 0, "vec2", "v_px") ++
        glslOut(backend, 1, "vec2", "v_radii") ++
        glslOut(backend, 2, "float", "v_phase") ++
        glslOut(backend, 3, "vec4", "v_color") ++
        "void main() {\n" ++
        glslEvalFloat("puck_radius", "a_puck_radius", "puck_radius", "puck_radius_t", "puck-radius") ++
        glslEvalFloat("pulse_radius", "a_pulse_radius", "pulse_radius", "pulse_radius_t", "pulse-radius") ++
        glslEvalFloat("pulse_period", "a_pulse_period", "pulse_period", "pulse_period_t", "pulse-period") ++
        glslEvalColor("vec4", "pulse_color", "a_pulse_color_min", "a_pulse_color_max", "pulse_color", "pulse_color_t", "pulse-color") ++
        glslVertexPrologue(backend) ++
        "    float half_px = max(pulse_radius, puck_radius + 2.0) + 1.5;\n" ++
        glslVertexEpilogue(backend) ++
        "    v_radii = vec2(puck_radius, pulse_radius);\n" ++
        "    v_phase = fract(u.frame.y / max(pulse_period, 0.05));\n" ++
        "    v_color = pulse_color;\n" ++
        "    v_px = corner * half_px;\n" ++
        "}\n";
}

fn pulseFragmentGlsl(comptime backend: GLSL) []const u8 {
    return glslIn(backend, 0, "vec2", "v_px") ++
        glslIn(backend, 1, "vec2", "v_radii") ++
        glslIn(backend, 2, "float", "v_phase") ++
        glslIn(backend, 3, "vec4", "v_color") ++
        (if (backend == .vulkan) "layout(location = 0) out vec4 fragColor;\n" else "") ++
        "void main() {\n" ++
        "    float r = length(v_px);\n" ++
        "    float ring_r = mix(v_radii.x + 2.0, v_radii.y, v_phase);\n" ++
        "    float band = 1.0 - smoothstep(1.0, 2.0, abs(r - ring_r));\n" ++
        "    float fade = 1.0 - v_phase;\n" ++
        "    float gate = smoothstep(0.0, 1.0, v_radii.y);\n" ++
        "    fragColor = v_color * band * fade * gate;\n" ++
        "}\n";
}

fn arrowVertexGlsl(comptime backend: GLSL) []const u8 {
    return glslIn(backend, 0, if (backend == .vulkan) "ivec2" else "vec2", "a_pos") ++
        glslIn(backend, 1, if (backend == .vulkan) "ivec2" else "vec2", "a_prev_pos") ++
        glslIn(backend, 2, "float", "a_start_time") ++
        guard("bearing", glslIn(backend, 3, "vec2", "a_bearing")) ++
        guard("puck-radius", glslIn(backend, 4, "vec2", "a_puck_radius")) ++
        guard("puck-border-width", glslIn(backend, 5, "vec2", "a_puck_border_width")) ++
        guard("bearing-visible", glslIn(backend, 6, "vec2", "a_bearing_visible")) ++
        guard("bearing-arrow-color", glslIn(backend, 7, "vec4", "a_bearing_arrow_color_min") ++ glslIn(backend, 8, "vec4", "a_bearing_arrow_color_max")) ++
        glslUbo(backend, arrow_uniform_id, "ArrowUBO", arrow_ubo_members) ++
        glslOut(backend, 0, "vec2", "v_px") ++
        glslOut(backend, 1, "vec2", "v_dir") ++
        glslOut(backend, 2, "vec4", "v_color") ++
        glslOut(backend, 3, "float", "v_outer") ++
        glslOut(backend, 4, "float", "v_visible") ++
        "void main() {\n" ++
        glslEvalFloat("bearing", "a_bearing", "bearing", "bearing_t", "bearing") ++
        glslEvalFloat("puck_radius", "a_puck_radius", "puck_radius", "puck_radius_t", "puck-radius") ++
        glslEvalFloat("puck_border_width", "a_puck_border_width", "puck_border_width", "puck_border_width_t", "puck-border-width") ++
        glslEvalFloat("bearing_visible", "a_bearing_visible", "bearing_visible", "bearing_visible_t", "bearing-visible") ++
        glslEvalColor("vec4", "bearing_arrow_color", "a_bearing_arrow_color_min", "a_bearing_arrow_color_max", "bearing_arrow_color", "bearing_arrow_color_t", "bearing-arrow-color") ++
        glslVertexPrologue(backend) ++
        // The arrow derives from the puck, matching the core indicator's
        // bearing image: apex at 1.8x the outer puck radius.
        "    float outer = puck_radius + puck_border_width;\n" ++
        "    float half_px = outer * 1.8 + 1.5;\n" ++
        glslVertexEpilogue(backend) ++
        "    v_dir = u.frame.zw;\n" ++
        "    v_color = bearing_arrow_color;\n" ++
        "    v_outer = outer;\n" ++
        "    v_visible = bearing_visible;\n" ++
        "    v_px = corner * half_px;\n" ++
        "}\n";
}

fn arrowFragmentGlsl(comptime backend: GLSL) []const u8 {
    return glslIn(backend, 0, "vec2", "v_px") ++
        glslIn(backend, 1, "vec2", "v_dir") ++
        glslIn(backend, 2, "vec4", "v_color") ++
        glslIn(backend, 3, "float", "v_outer") ++
        glslIn(backend, 4, "float", "v_visible") ++
        (if (backend == .vulkan) "layout(location = 0) out vec4 fragColor;\n" else "") ++
        glsl_sd_triangle ++
        "void main() {\n" ++
        "    vec2 apex = v_dir * (v_outer * 1.8);\n" ++
        "    vec2 base = v_dir * (v_outer * 0.5);\n" ++
        "    vec2 perp = vec2(-v_dir.y, v_dir.x) * (v_outer * 0.7);\n" ++
        "    float sd = sdTriangle(v_px, apex, base + perp, base - perp);\n" ++
        "    float alpha = 1.0 - smoothstep(-0.5, 0.5, sd);\n" ++
        "    fragColor = v_color * alpha * smoothstep(0.0, 1.0, v_outer) * v_visible;\n" ++
        "}\n";
}

fn puckVertexGlsl(comptime backend: GLSL) []const u8 {
    return glslIn(backend, 0, if (backend == .vulkan) "ivec2" else "vec2", "a_pos") ++
        glslIn(backend, 1, if (backend == .vulkan) "ivec2" else "vec2", "a_prev_pos") ++
        glslIn(backend, 2, "float", "a_start_time") ++
        guard("puck-radius", glslIn(backend, 3, "vec2", "a_puck_radius")) ++
        guard("puck-border-width", glslIn(backend, 4, "vec2", "a_puck_border_width")) ++
        guard("puck-color", glslIn(backend, 5, "vec4", "a_puck_color_min") ++ glslIn(backend, 6, "vec4", "a_puck_color_max")) ++
        guard("puck-border-color", glslIn(backend, 7, "vec4", "a_puck_border_color_min") ++ glslIn(backend, 8, "vec4", "a_puck_border_color_max")) ++
        glslUbo(backend, puck_uniform_id, "PuckUBO", puck_ubo_members) ++
        glslOut(backend, 0, "vec2", "v_px") ++
        glslOut(backend, 1, "vec4", "v_fill") ++
        glslOut(backend, 2, "vec4", "v_border") ++
        glslOut(backend, 3, "float", "v_radius") ++
        glslOut(backend, 4, "float", "v_border_w") ++
        "void main() {\n" ++
        glslEvalFloat("puck_radius", "a_puck_radius", "puck_radius", "puck_radius_t", "puck-radius") ++
        glslEvalFloat("puck_border_width", "a_puck_border_width", "puck_border_width", "puck_border_width_t", "puck-border-width") ++
        glslEvalColor("vec4", "puck_color", "a_puck_color_min", "a_puck_color_max", "puck_color", "puck_color_t", "puck-color") ++
        glslEvalColor("vec4", "puck_border_color", "a_puck_border_color_min", "a_puck_border_color_max", "puck_border_color", "puck_border_color_t", "puck-border-color") ++
        glslVertexPrologue(backend) ++
        "    float half_px = puck_radius + puck_border_width + 1.5;\n" ++
        glslVertexEpilogue(backend) ++
        "    v_px = corner * half_px;\n" ++
        "    v_fill = puck_color;\n" ++
        "    v_border = puck_border_color;\n" ++
        "    v_radius = puck_radius;\n" ++
        "    v_border_w = puck_border_width;\n" ++
        "}\n";
}

fn puckFragmentGlsl(comptime backend: GLSL) []const u8 {
    return glslIn(backend, 0, "vec2", "v_px") ++
        glslIn(backend, 1, "vec4", "v_fill") ++
        glslIn(backend, 2, "vec4", "v_border") ++
        glslIn(backend, 3, "float", "v_radius") ++
        glslIn(backend, 4, "float", "v_border_w") ++
        (if (backend == .vulkan) "layout(location = 0) out vec4 fragColor;\n" else "") ++
        "void main() {\n" ++
        "    float d = length(v_px);\n" ++
        "    float outer = v_radius + v_border_w;\n" ++
        "    float edge = 1.0 - smoothstep(outer - 0.5, outer + 0.5, d);\n" ++
        "    float ring = smoothstep(v_radius - 0.5, v_radius + 0.5, d);\n" ++
        "    fragColor = mix(v_fill, v_border, ring) * edge;\n" ++
        "}\n";
}

// --------------------------------------------------------------------------
// Metal sources: one complete MSL source per shader, with a UBO struct that
// matches the std140 layout and entry points <shader>Vertex/<shader>Fragment.
// --------------------------------------------------------------------------

fn mslUbo(comptime name: []const u8, comptime members: []const u8) []const u8 {
    return "struct alignas(16) " ++ name ++ " {\n" ++ members ++ "};\n";
}

fn mslMembers(comptime glsl_members: []const u8) []const u8 {
    @setEvalBranchQuota(1_000_000);
    comptime var result: []const u8 = "";
    var rest = glsl_members;
    while (std.mem.indexOf(u8, rest, "vec4")) |index| {
        result = result ++ rest[0..index] ++ "float4";
        rest = rest[index + "vec4".len ..];
    }
    result = result ++ rest;
    var rest2 = result;
    var final: []const u8 = "";
    while (std.mem.indexOf(u8, rest2, "mat4")) |index| {
        final = final ++ rest2[0..index] ++ "float4x4";
        rest2 = rest2[index + "mat4".len ..];
    }
    return final ++ rest2;
}

fn mslAttr(comptime location: u8, comptime msl_type: []const u8, comptime name: []const u8) []const u8 {
    return "    " ++ msl_type ++ " " ++ name ++ " [[attribute(" ++ num(location) ++ ")]];\n";
}

const msl_transition =
    "    float dt = u.frame.y - in.a_start_time;\n" ++
    "    if (dt < 0.0) dt += 3600.0;\n" ++
    "    float t = clamp(dt / 0.3, 0.0, 1.0);\n" ++
    "    t = t * t * (3.0 - 2.0 * t);\n";

const msl_project =
    "    float2 center = floor(encoded * 0.5);\n" ++
    "    float2 corner = (encoded - 2.0 * center) * 2.0 - 1.0;\n" ++
    "    float2 tile_pos = mix(float2(in.a_prev_pos), center, t);\n" ++
    "    float4 p = u.matrix * float4(tile_pos, 0.0, 1.0);\n" ++
    "    float4 q = u.matrix * float4(tile_pos + float2(1.0, 0.0), 0.0, 1.0);\n" ++
    "    float px_per_unit = length((q.xy / q.w - p.xy / p.w) * u.camera.zw * 0.5);\n";

const msl_sd_triangle =
    "float sdTriangle(float2 p, float2 p0, float2 p1, float2 p2) {\n" ++
    "    float2 e0 = p1 - p0;\n" ++
    "    float2 e1 = p2 - p1;\n" ++
    "    float2 e2 = p0 - p2;\n" ++
    "    float2 v0 = p - p0;\n" ++
    "    float2 v1 = p - p1;\n" ++
    "    float2 v2 = p - p2;\n" ++
    "    float2 pq0 = v0 - e0 * clamp(dot(v0, e0) / max(dot(e0, e0), 1e-6), 0.0, 1.0);\n" ++
    "    float2 pq1 = v1 - e1 * clamp(dot(v1, e1) / max(dot(e1, e1), 1e-6), 0.0, 1.0);\n" ++
    "    float2 pq2 = v2 - e2 * clamp(dot(v2, e2) / max(dot(e2, e2), 1e-6), 0.0, 1.0);\n" ++
    "    float s = sign(e0.x * e2.y - e0.y * e2.x);\n" ++
    "    float2 d = min(min(float2(dot(pq0, pq0), s * (v0.x * e0.y - v0.y * e0.x)),\n" ++
    "                       float2(dot(pq1, pq1), s * (v1.x * e1.y - v1.y * e1.x))),\n" ++
    "                   float2(dot(pq2, pq2), s * (v2.x * e2.y - v2.y * e2.x)));\n" ++
    "    return -sqrt(d.x) * sign(d.y);\n" ++
    "}\n";

const msl_emit =
    "    out.position = p + float4(corner * half_px * u.camera.xy * p.w, 0.0, 0.0);\n" ++
    "    out.position.z = 0.0;\n";

const accuracy_metal_source =
    mslUbo("AccuracyUBO", mslMembers(accuracy_ubo_members)) ++
    "struct AccuracyVertex {\n" ++
    mslAttr(0, "short2", "a_pos") ++
    mslAttr(1, "short2", "a_prev_pos") ++
    mslAttr(2, "float", "a_start_time") ++
    mslAttr(3, "float", "a_scale") ++
    guard("accuracy-radius", mslAttr(4, "float2", "a_accuracy_radius")) ++
    guard("accuracy-border-width", mslAttr(5, "float2", "a_accuracy_border_width")) ++
    guard("accuracy-color", mslAttr(6, "float4", "a_accuracy_color_min") ++ mslAttr(7, "float4", "a_accuracy_color_max")) ++
    guard("accuracy-border-color", mslAttr(8, "float4", "a_accuracy_border_color_min") ++ mslAttr(9, "float4", "a_accuracy_border_color_max")) ++
    "};\n" ++
    "struct AccuracyVaryings {\n" ++
    "    float4 position [[position]];\n" ++
    "    float2 px;\n" ++
    "    float4 fill;\n" ++
    "    float4 border;\n" ++
    "    float radius_px;\n" ++
    "};\n" ++
    "vertex AccuracyVaryings accuracyVertex(AccuracyVertex in [[stage_in]],\n" ++
    "    constant AccuracyUBO& u [[buffer(MLN_PLUGIN_UNIFORM_" ++ num(accuracy_uniform_id) ++ "_BINDING)]]) {\n" ++
    "    AccuracyVaryings out;\n" ++
    glslEvalFloat("accuracy_radius", "in.a_accuracy_radius", "accuracy_radius", "accuracy_radius_t", "accuracy-radius") ++
    glslEvalFloat("accuracy_border_width", "in.a_accuracy_border_width", "accuracy_border_width", "accuracy_border_width_t", "accuracy-border-width") ++
    glslEvalColor("float4", "accuracy_color", "in.a_accuracy_color_min", "in.a_accuracy_color_max", "accuracy_color", "accuracy_color_t", "accuracy-color") ++
    glslEvalColor("float4", "accuracy_border_color", "in.a_accuracy_border_color_min", "in.a_accuracy_border_color_max", "accuracy_border_color", "accuracy_border_color_t", "accuracy-border-color") ++
    "    float2 encoded = float2(in.a_pos);\n" ++
    msl_transition ++
    msl_project ++
    "    out.position = p;\n" ++
    "    out.position.z = 0.0;\n" ++
    "    out.px = corner;\n" ++
    "    out.fill = accuracy_color;\n" ++
    "    out.border = accuracy_border_color;\n" ++
    "    out.radius_px = accuracy_radius * px_per_unit / max(in.a_scale, 1e-6);\n" ++
    "    return out;\n" ++
    "}\n" ++
    "fragment half4 accuracyFragment(AccuracyVaryings in [[stage_in]]) {\n" ++
    "    float gate = smoothstep(0.0, 1.0, in.radius_px);\n" ++
    "    return half4(mix(in.fill, in.border, step(0.0, in.px.x)) * gate);\n" ++
    "}\n";

const sector_metal_source =
    mslUbo("SectorUBO", mslMembers(sector_ubo_members)) ++
    "struct SectorVertex {\n" ++
    mslAttr(0, "short2", "a_pos") ++
    mslAttr(1, "short2", "a_prev_pos") ++
    mslAttr(2, "float", "a_start_time") ++
    guard("bearing", mslAttr(3, "float2", "a_bearing")) ++
    guard("bearing-accuracy", mslAttr(4, "float2", "a_bearing_accuracy")) ++
    guard("bearing-accuracy-radius", mslAttr(5, "float2", "a_bearing_accuracy_radius")) ++
    guard("bearing-visible", mslAttr(6, "float2", "a_bearing_visible")) ++
    guard("bearing-accuracy-color", mslAttr(7, "float4", "a_bearing_accuracy_color_min") ++ mslAttr(8, "float4", "a_bearing_accuracy_color_max")) ++
    "};\n" ++
    "struct SectorVaryings {\n" ++
    "    float4 position [[position]];\n" ++
    "    float2 px;\n" ++
    "    float2 dir;\n" ++
    "    float4 color;\n" ++
    "    float half_width;\n" ++
    "    float radius;\n" ++
    "    float visible;\n" ++
    "};\n" ++
    "vertex SectorVaryings sectorVertex(SectorVertex in [[stage_in]],\n" ++
    "    constant SectorUBO& u [[buffer(MLN_PLUGIN_UNIFORM_" ++ num(sector_uniform_id) ++ "_BINDING)]]) {\n" ++
    "    SectorVaryings out;\n" ++
    glslEvalFloat("bearing", "in.a_bearing", "bearing", "bearing_t", "bearing") ++
    glslEvalFloat("bearing_accuracy", "in.a_bearing_accuracy", "bearing_accuracy", "bearing_accuracy_t", "bearing-accuracy") ++
    glslEvalFloat("bearing_accuracy_radius", "in.a_bearing_accuracy_radius", "bearing_accuracy_radius", "bearing_accuracy_radius_t", "bearing-accuracy-radius") ++
    glslEvalFloat("bearing_visible", "in.a_bearing_visible", "bearing_visible", "bearing_visible_t", "bearing-visible") ++
    glslEvalColor("float4", "bearing_accuracy_color", "in.a_bearing_accuracy_color_min", "in.a_bearing_accuracy_color_max", "bearing_accuracy_color", "bearing_accuracy_color_t", "bearing-accuracy-color") ++
    "    float2 encoded = float2(in.a_pos);\n" ++
    msl_transition ++
    msl_project ++
    "    float half_px = bearing_accuracy_radius + 1.5;\n" ++
    msl_emit ++
    "    out.dir = u.frame.zw;\n" ++
    "    out.half_width = radians(bearing_accuracy);\n" ++
    "    out.radius = bearing_accuracy_radius;\n" ++
    "    out.visible = bearing_visible;\n" ++
    "    out.color = bearing_accuracy_color;\n" ++
    "    out.px = corner * half_px;\n" ++
    "    return out;\n" ++
    "}\n" ++
    "fragment half4 sectorFragment(SectorVaryings in [[stage_in]]) {\n" ++
    "    float r = length(in.px);\n" ++
    "    float rn = r / max(in.radius, 1e-4);\n" ++
    "    float angle = acos(clamp(dot(in.px / max(r, 1e-4), in.dir), -1.0, 1.0));\n" ++
    "    float feather = length(fwidth(in.px)) / max(r, 1e-4);\n" ++
    "    float angular = in.half_width >= 3.14159265 ? 1.0 :\n" ++
    "        1.0 - smoothstep(in.half_width - feather, in.half_width + feather, angle);\n" ++
    "    float opacity = angular * (1.0 - smoothstep(0.0, 1.0, rn));\n" ++
    "    float gate = smoothstep(0.0, 1.0, in.radius) * step(1e-4, in.half_width) * in.visible;\n" ++
    "    return half4(in.color * opacity * gate);\n" ++
    "}\n";

const shadow_metal_source =
    mslUbo("ShadowUBO", mslMembers(shadow_ubo_members)) ++
    "struct ShadowVertex {\n" ++
    mslAttr(0, "short2", "a_pos") ++
    mslAttr(1, "short2", "a_prev_pos") ++
    mslAttr(2, "float", "a_start_time") ++
    guard("shadow-radius", mslAttr(3, "float2", "a_shadow_radius")) ++
    guard("shadow-color", mslAttr(4, "float4", "a_shadow_color_min") ++ mslAttr(5, "float4", "a_shadow_color_max")) ++
    "};\n" ++
    "struct ShadowVaryings {\n" ++
    "    float4 position [[position]];\n" ++
    "    float2 px;\n" ++
    "    float4 color;\n" ++
    "    float radius;\n" ++
    "};\n" ++
    "vertex ShadowVaryings shadowVertex(ShadowVertex in [[stage_in]],\n" ++
    "    constant ShadowUBO& u [[buffer(MLN_PLUGIN_UNIFORM_" ++ num(shadow_uniform_id) ++ "_BINDING)]]) {\n" ++
    "    ShadowVaryings out;\n" ++
    glslEvalFloat("shadow_radius", "in.a_shadow_radius", "shadow_radius", "shadow_radius_t", "shadow-radius") ++
    glslEvalColor("float4", "shadow_color", "in.a_shadow_color_min", "in.a_shadow_color_max", "shadow_color", "shadow_color_t", "shadow-color") ++
    "    float2 encoded = float2(in.a_pos);\n" ++
    msl_transition ++
    msl_project ++
    "    float half_px = shadow_radius + 1.5;\n" ++
    msl_emit ++
    "    out.px = corner * half_px;\n" ++
    "    out.color = shadow_color;\n" ++
    "    out.radius = shadow_radius;\n" ++
    "    return out;\n" ++
    "}\n" ++
    "fragment half4 shadowFragment(ShadowVaryings in [[stage_in]]) {\n" ++
    "    float d = length(in.px * float2(1.0, 1.6));\n" ++
    "    float alpha = 1.0 - smoothstep(in.radius - 0.5, in.radius + 0.5, d);\n" ++
    "    return half4(in.color * alpha * smoothstep(0.0, 1.0, in.radius));\n" ++
    "}\n";

const pulse_metal_source =
    mslUbo("PulseUBO", mslMembers(pulse_ubo_members)) ++
    "struct PulseVertex {\n" ++
    mslAttr(0, "short2", "a_pos") ++
    mslAttr(1, "short2", "a_prev_pos") ++
    mslAttr(2, "float", "a_start_time") ++
    guard("puck-radius", mslAttr(3, "float2", "a_puck_radius")) ++
    guard("pulse-radius", mslAttr(4, "float2", "a_pulse_radius")) ++
    guard("pulse-period", mslAttr(5, "float2", "a_pulse_period")) ++
    guard("pulse-color", mslAttr(6, "float4", "a_pulse_color_min") ++ mslAttr(7, "float4", "a_pulse_color_max")) ++
    "};\n" ++
    "struct PulseVaryings {\n" ++
    "    float4 position [[position]];\n" ++
    "    float2 px;\n" ++
    "    float2 radii;\n" ++
    "    float phase;\n" ++
    "    float4 color;\n" ++
    "};\n" ++
    "vertex PulseVaryings pulseVertex(PulseVertex in [[stage_in]],\n" ++
    "    constant PulseUBO& u [[buffer(MLN_PLUGIN_UNIFORM_" ++ num(pulse_uniform_id) ++ "_BINDING)]]) {\n" ++
    "    PulseVaryings out;\n" ++
    glslEvalFloat("puck_radius", "in.a_puck_radius", "puck_radius", "puck_radius_t", "puck-radius") ++
    glslEvalFloat("pulse_radius", "in.a_pulse_radius", "pulse_radius", "pulse_radius_t", "pulse-radius") ++
    glslEvalFloat("pulse_period", "in.a_pulse_period", "pulse_period", "pulse_period_t", "pulse-period") ++
    glslEvalColor("float4", "pulse_color", "in.a_pulse_color_min", "in.a_pulse_color_max", "pulse_color", "pulse_color_t", "pulse-color") ++
    "    float2 encoded = float2(in.a_pos);\n" ++
    msl_transition ++
    msl_project ++
    "    float half_px = max(pulse_radius, puck_radius + 2.0) + 1.5;\n" ++
    msl_emit ++
    "    out.radii = float2(puck_radius, pulse_radius);\n" ++
    "    out.phase = fract(u.frame.y / max(pulse_period, 0.05));\n" ++
    "    out.color = pulse_color;\n" ++
    "    out.px = corner * half_px;\n" ++
    "    return out;\n" ++
    "}\n" ++
    "fragment half4 pulseFragment(PulseVaryings in [[stage_in]]) {\n" ++
    "    float r = length(in.px);\n" ++
    "    float ring_r = mix(in.radii.x + 2.0, in.radii.y, in.phase);\n" ++
    "    float band = 1.0 - smoothstep(1.0, 2.0, abs(r - ring_r));\n" ++
    "    float fade = 1.0 - in.phase;\n" ++
    "    float gate = smoothstep(0.0, 1.0, in.radii.y);\n" ++
    "    return half4(in.color * band * fade * gate);\n" ++
    "}\n";

const arrow_metal_source =
    mslUbo("ArrowUBO", mslMembers(arrow_ubo_members)) ++
    "struct ArrowVertex {\n" ++
    mslAttr(0, "short2", "a_pos") ++
    mslAttr(1, "short2", "a_prev_pos") ++
    mslAttr(2, "float", "a_start_time") ++
    guard("bearing", mslAttr(3, "float2", "a_bearing")) ++
    guard("puck-radius", mslAttr(4, "float2", "a_puck_radius")) ++
    guard("puck-border-width", mslAttr(5, "float2", "a_puck_border_width")) ++
    guard("bearing-visible", mslAttr(6, "float2", "a_bearing_visible")) ++
    guard("bearing-arrow-color", mslAttr(7, "float4", "a_bearing_arrow_color_min") ++ mslAttr(8, "float4", "a_bearing_arrow_color_max")) ++
    "};\n" ++
    "struct ArrowVaryings {\n" ++
    "    float4 position [[position]];\n" ++
    "    float2 px;\n" ++
    "    float2 dir;\n" ++
    "    float4 color;\n" ++
    "    float outer;\n" ++
    "    float visible;\n" ++
    "};\n" ++
    msl_sd_triangle ++
    "vertex ArrowVaryings arrowVertex(ArrowVertex in [[stage_in]],\n" ++
    "    constant ArrowUBO& u [[buffer(MLN_PLUGIN_UNIFORM_" ++ num(arrow_uniform_id) ++ "_BINDING)]]) {\n" ++
    "    ArrowVaryings out;\n" ++
    glslEvalFloat("bearing", "in.a_bearing", "bearing", "bearing_t", "bearing") ++
    glslEvalFloat("puck_radius", "in.a_puck_radius", "puck_radius", "puck_radius_t", "puck-radius") ++
    glslEvalFloat("puck_border_width", "in.a_puck_border_width", "puck_border_width", "puck_border_width_t", "puck-border-width") ++
    glslEvalFloat("bearing_visible", "in.a_bearing_visible", "bearing_visible", "bearing_visible_t", "bearing-visible") ++
    glslEvalColor("float4", "bearing_arrow_color", "in.a_bearing_arrow_color_min", "in.a_bearing_arrow_color_max", "bearing_arrow_color", "bearing_arrow_color_t", "bearing-arrow-color") ++
    "    float2 encoded = float2(in.a_pos);\n" ++
    msl_transition ++
    msl_project ++
    "    float outer = puck_radius + puck_border_width;\n" ++
    "    float half_px = outer * 1.8 + 1.5;\n" ++
    msl_emit ++
    "    out.dir = u.frame.zw;\n" ++
    "    out.color = bearing_arrow_color;\n" ++
    "    out.outer = outer;\n" ++
    "    out.visible = bearing_visible;\n" ++
    "    out.px = corner * half_px;\n" ++
    "    return out;\n" ++
    "}\n" ++
    "fragment half4 arrowFragment(ArrowVaryings in [[stage_in]]) {\n" ++
    "    float2 apex = in.dir * (in.outer * 1.8);\n" ++
    "    float2 base = in.dir * (in.outer * 0.5);\n" ++
    "    float2 perp = float2(-in.dir.y, in.dir.x) * (in.outer * 0.7);\n" ++
    "    float sd = sdTriangle(in.px, apex, base + perp, base - perp);\n" ++
    "    float alpha = 1.0 - smoothstep(-0.5, 0.5, sd);\n" ++
    "    return half4(in.color * alpha * smoothstep(0.0, 1.0, in.outer) * in.visible);\n" ++
    "}\n";

const puck_metal_source =
    mslUbo("PuckUBO", mslMembers(puck_ubo_members)) ++
    "struct PuckVertex {\n" ++
    mslAttr(0, "short2", "a_pos") ++
    mslAttr(1, "short2", "a_prev_pos") ++
    mslAttr(2, "float", "a_start_time") ++
    guard("puck-radius", mslAttr(3, "float2", "a_puck_radius")) ++
    guard("puck-border-width", mslAttr(4, "float2", "a_puck_border_width")) ++
    guard("puck-color", mslAttr(5, "float4", "a_puck_color_min") ++ mslAttr(6, "float4", "a_puck_color_max")) ++
    guard("puck-border-color", mslAttr(7, "float4", "a_puck_border_color_min") ++ mslAttr(8, "float4", "a_puck_border_color_max")) ++
    "};\n" ++
    "struct PuckVaryings {\n" ++
    "    float4 position [[position]];\n" ++
    "    float2 px;\n" ++
    "    float4 fill;\n" ++
    "    float4 border;\n" ++
    "    float radius;\n" ++
    "    float border_w;\n" ++
    "};\n" ++
    "vertex PuckVaryings puckVertex(PuckVertex in [[stage_in]],\n" ++
    "    constant PuckUBO& u [[buffer(MLN_PLUGIN_UNIFORM_" ++ num(puck_uniform_id) ++ "_BINDING)]]) {\n" ++
    "    PuckVaryings out;\n" ++
    glslEvalFloat("puck_radius", "in.a_puck_radius", "puck_radius", "puck_radius_t", "puck-radius") ++
    glslEvalFloat("puck_border_width", "in.a_puck_border_width", "puck_border_width", "puck_border_width_t", "puck-border-width") ++
    glslEvalColor("float4", "puck_color", "in.a_puck_color_min", "in.a_puck_color_max", "puck_color", "puck_color_t", "puck-color") ++
    glslEvalColor("float4", "puck_border_color", "in.a_puck_border_color_min", "in.a_puck_border_color_max", "puck_border_color", "puck_border_color_t", "puck-border-color") ++
    "    float2 encoded = float2(in.a_pos);\n" ++
    msl_transition ++
    msl_project ++
    "    float half_px = puck_radius + puck_border_width + 1.5;\n" ++
    msl_emit ++
    "    out.px = corner * half_px;\n" ++
    "    out.fill = puck_color;\n" ++
    "    out.border = puck_border_color;\n" ++
    "    out.radius = puck_radius;\n" ++
    "    out.border_w = puck_border_width;\n" ++
    "    return out;\n" ++
    "}\n" ++
    "fragment half4 puckFragment(PuckVaryings in [[stage_in]]) {\n" ++
    "    float d = length(in.px);\n" ++
    "    float outer = in.radius + in.border_w;\n" ++
    "    float edge = 1.0 - smoothstep(outer - 0.5, outer + 0.5, d);\n" ++
    "    float ring = smoothstep(in.radius - 0.5, in.radius + 0.5, d);\n" ++
    "    return half4(mix(in.fill, in.border, ring) * edge);\n" ++
    "}\n";

// --------------------------------------------------------------------------
// Descriptors
// --------------------------------------------------------------------------

fn shaderSource(
    comptime backend: c.mln_plugin_backend,
    comptime vertex: []const u8,
    comptime fragment: []const u8,
    comptime vertex_entry: []const u8,
    comptime fragment_entry: []const u8,
) c.mln_plugin_shader_source_v1 {
    return .{
        .struct_size = @sizeOf(c.mln_plugin_shader_source_v1),
        .backend = backend,
        .vertex_source = str(vertex),
        .fragment_source = if (fragment.len == 0) empty_str else str(fragment),
        .vertex_entry_point = if (vertex_entry.len == 0) empty_str else str(vertex_entry),
        .fragment_entry_point = if (fragment_entry.len == 0) empty_str else str(fragment_entry),
    };
}

fn shaderSources(
    comptime vertex_opengl: []const u8,
    comptime fragment_opengl: []const u8,
    comptime vertex_vulkan: []const u8,
    comptime fragment_vulkan: []const u8,
    comptime metal: []const u8,
    comptime vertex_entry: []const u8,
    comptime fragment_entry: []const u8,
) [3]c.mln_plugin_shader_source_v1 {
    return .{
        shaderSource(c.MLN_PLUGIN_BACKEND_OPENGL, vertex_opengl, fragment_opengl, "", ""),
        shaderSource(c.MLN_PLUGIN_BACKEND_VULKAN, vertex_vulkan, fragment_vulkan, "", ""),
        shaderSource(c.MLN_PLUGIN_BACKEND_METAL, metal, "", vertex_entry, fragment_entry),
    };
}

const accuracy_sources = shaderSources(
    accuracyVertexGlsl(.opengl),
    accuracyFragmentGlsl(.opengl),
    accuracyVertexGlsl(.vulkan),
    accuracyFragmentGlsl(.vulkan),
    accuracy_metal_source,
    "accuracyVertex",
    "accuracyFragment",
);

const sector_sources = shaderSources(
    sectorVertexGlsl(.opengl),
    sectorFragmentGlsl(.opengl),
    sectorVertexGlsl(.vulkan),
    sectorFragmentGlsl(.vulkan),
    sector_metal_source,
    "sectorVertex",
    "sectorFragment",
);

const shadow_sources = shaderSources(
    shadowVertexGlsl(.opengl),
    shadowFragmentGlsl(.opengl),
    shadowVertexGlsl(.vulkan),
    shadowFragmentGlsl(.vulkan),
    shadow_metal_source,
    "shadowVertex",
    "shadowFragment",
);

const pulse_sources = shaderSources(
    pulseVertexGlsl(.opengl),
    pulseFragmentGlsl(.opengl),
    pulseVertexGlsl(.vulkan),
    pulseFragmentGlsl(.vulkan),
    pulse_metal_source,
    "pulseVertex",
    "pulseFragment",
);

const arrow_sources = shaderSources(
    arrowVertexGlsl(.opengl),
    arrowFragmentGlsl(.opengl),
    arrowVertexGlsl(.vulkan),
    arrowFragmentGlsl(.vulkan),
    arrow_metal_source,
    "arrowVertex",
    "arrowFragment",
);

const puck_sources = shaderSources(
    puckVertexGlsl(.opengl),
    puckFragmentGlsl(.opengl),
    puckVertexGlsl(.vulkan),
    puckFragmentGlsl(.vulkan),
    puck_metal_source,
    "puckVertex",
    "puckFragment",
);

fn attribute(
    comptime id: u32,
    comptime location: u32,
    comptime name: []const u8,
    comptime attribute_type: c.mln_plugin_vertex_attribute_type,
) c.mln_plugin_shader_attribute_v1 {
    return .{
        .struct_size = @sizeOf(c.mln_plugin_shader_attribute_v1),
        .attribute_id = id,
        .location = location,
        .name = str(name),
        .type = attribute_type,
    };
}

const accuracy_attributes = [_]c.mln_plugin_shader_attribute_v1{
    attribute(0, 0, "a_pos", c.MLN_PLUGIN_VERTEX_INT16_X2),
    attribute(1, 1, "a_prev_pos", c.MLN_PLUGIN_VERTEX_INT16_X2),
    attribute(2, 2, "a_start_time", c.MLN_PLUGIN_VERTEX_FLOAT),
    attribute(3, 3, "a_scale", c.MLN_PLUGIN_VERTEX_FLOAT),
    attribute(4, 4, "a_accuracy_radius", c.MLN_PLUGIN_VERTEX_FLOAT_X2),
    attribute(5, 5, "a_accuracy_border_width", c.MLN_PLUGIN_VERTEX_FLOAT_X2),
    attribute(6, 6, "a_accuracy_color_min", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
    attribute(7, 7, "a_accuracy_color_max", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
    attribute(8, 8, "a_accuracy_border_color_min", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
    attribute(9, 9, "a_accuracy_border_color_max", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
    attribute(10, 10, "a_position", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
};

const sector_attributes = [_]c.mln_plugin_shader_attribute_v1{
    attribute(0, 0, "a_pos", c.MLN_PLUGIN_VERTEX_INT16_X2),
    attribute(1, 1, "a_prev_pos", c.MLN_PLUGIN_VERTEX_INT16_X2),
    attribute(2, 2, "a_start_time", c.MLN_PLUGIN_VERTEX_FLOAT),
    attribute(3, 3, "a_bearing", c.MLN_PLUGIN_VERTEX_FLOAT_X2),
    attribute(4, 4, "a_bearing_accuracy", c.MLN_PLUGIN_VERTEX_FLOAT_X2),
    attribute(5, 5, "a_bearing_accuracy_radius", c.MLN_PLUGIN_VERTEX_FLOAT_X2),
    attribute(6, 6, "a_bearing_visible", c.MLN_PLUGIN_VERTEX_FLOAT_X2),
    attribute(7, 7, "a_bearing_accuracy_color_min", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
    attribute(8, 8, "a_bearing_accuracy_color_max", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
    attribute(9, 9, "a_position", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
};

const shadow_attributes = [_]c.mln_plugin_shader_attribute_v1{
    attribute(0, 0, "a_pos", c.MLN_PLUGIN_VERTEX_INT16_X2),
    attribute(1, 1, "a_prev_pos", c.MLN_PLUGIN_VERTEX_INT16_X2),
    attribute(2, 2, "a_start_time", c.MLN_PLUGIN_VERTEX_FLOAT),
    attribute(3, 3, "a_shadow_radius", c.MLN_PLUGIN_VERTEX_FLOAT_X2),
    attribute(4, 4, "a_shadow_color_min", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
    attribute(5, 5, "a_shadow_color_max", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
    attribute(6, 6, "a_position", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
};

const pulse_attributes = [_]c.mln_plugin_shader_attribute_v1{
    attribute(0, 0, "a_pos", c.MLN_PLUGIN_VERTEX_INT16_X2),
    attribute(1, 1, "a_prev_pos", c.MLN_PLUGIN_VERTEX_INT16_X2),
    attribute(2, 2, "a_start_time", c.MLN_PLUGIN_VERTEX_FLOAT),
    attribute(3, 3, "a_puck_radius", c.MLN_PLUGIN_VERTEX_FLOAT_X2),
    attribute(4, 4, "a_pulse_radius", c.MLN_PLUGIN_VERTEX_FLOAT_X2),
    attribute(5, 5, "a_pulse_period", c.MLN_PLUGIN_VERTEX_FLOAT_X2),
    attribute(6, 6, "a_pulse_color_min", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
    attribute(7, 7, "a_pulse_color_max", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
    attribute(8, 8, "a_position", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
};

const arrow_attributes = [_]c.mln_plugin_shader_attribute_v1{
    attribute(0, 0, "a_pos", c.MLN_PLUGIN_VERTEX_INT16_X2),
    attribute(1, 1, "a_prev_pos", c.MLN_PLUGIN_VERTEX_INT16_X2),
    attribute(2, 2, "a_start_time", c.MLN_PLUGIN_VERTEX_FLOAT),
    attribute(3, 3, "a_bearing", c.MLN_PLUGIN_VERTEX_FLOAT_X2),
    attribute(4, 4, "a_puck_radius", c.MLN_PLUGIN_VERTEX_FLOAT_X2),
    attribute(5, 5, "a_puck_border_width", c.MLN_PLUGIN_VERTEX_FLOAT_X2),
    attribute(6, 6, "a_bearing_visible", c.MLN_PLUGIN_VERTEX_FLOAT_X2),
    attribute(7, 7, "a_bearing_arrow_color_min", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
    attribute(8, 8, "a_bearing_arrow_color_max", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
    attribute(9, 9, "a_position", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
};

const puck_attributes = [_]c.mln_plugin_shader_attribute_v1{
    attribute(0, 0, "a_pos", c.MLN_PLUGIN_VERTEX_INT16_X2),
    attribute(1, 1, "a_prev_pos", c.MLN_PLUGIN_VERTEX_INT16_X2),
    attribute(2, 2, "a_start_time", c.MLN_PLUGIN_VERTEX_FLOAT),
    attribute(3, 3, "a_puck_radius", c.MLN_PLUGIN_VERTEX_FLOAT_X2),
    attribute(4, 4, "a_puck_border_width", c.MLN_PLUGIN_VERTEX_FLOAT_X2),
    attribute(5, 5, "a_puck_color_min", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
    attribute(6, 6, "a_puck_color_max", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
    attribute(7, 7, "a_puck_border_color_min", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
    attribute(8, 8, "a_puck_border_color_max", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
    attribute(9, 9, "a_position", c.MLN_PLUGIN_VERTEX_FLOAT_X4),
};

fn uniformBlock(
    comptime uniform_id: u32,
    comptime name: []const u8,
    comptime byte_size: u32,
) c.mln_plugin_uniform_block_descriptor_v1 {
    return .{
        .struct_size = @sizeOf(c.mln_plugin_uniform_block_descriptor_v1),
        .uniform_id = uniform_id,
        .name = str(name),
        .byte_size = byte_size,
        .stage_mask = c.MLN_PLUGIN_SHADER_STAGE_VERTEX | c.MLN_PLUGIN_SHADER_STAGE_FRAGMENT,
        .scope = c.MLN_PLUGIN_UNIFORM_DRAWABLE,
    };
}

const accuracy_uniform_blocks = [_]c.mln_plugin_uniform_block_descriptor_v1{
    uniformBlock(accuracy_uniform_id, "AccuracyUBO", @sizeOf(AccuracyUBO)),
};
const sector_uniform_blocks = [_]c.mln_plugin_uniform_block_descriptor_v1{
    uniformBlock(sector_uniform_id, "SectorUBO", @sizeOf(SectorUBO)),
};
const shadow_uniform_blocks = [_]c.mln_plugin_uniform_block_descriptor_v1{
    uniformBlock(shadow_uniform_id, "ShadowUBO", @sizeOf(ShadowUBO)),
};
const pulse_uniform_blocks = [_]c.mln_plugin_uniform_block_descriptor_v1{
    uniformBlock(pulse_uniform_id, "PulseUBO", @sizeOf(PulseUBO)),
};
const arrow_uniform_blocks = [_]c.mln_plugin_uniform_block_descriptor_v1{
    uniformBlock(arrow_uniform_id, "ArrowUBO", @sizeOf(ArrowUBO)),
};
const puck_uniform_blocks = [_]c.mln_plugin_uniform_block_descriptor_v1{
    uniformBlock(puck_uniform_id, "PuckUBO", @sizeOf(PuckUBO)),
};

fn floatBinding(
    comptime property: []const u8,
    comptime uniform_id: u32,
    comptime uniform_offset: u32,
    comptime attribute_id: u32,
    comptime interpolation_offset: u32,
) c.mln_plugin_shader_property_binding_v1 {
    return .{
        .struct_size = @sizeOf(c.mln_plugin_shader_property_binding_v1),
        .property_name = str(property),
        .encoding = c.MLN_PLUGIN_PROPERTY_ENCODING_FLOAT,
        .uniform_id = uniform_id,
        .uniform_byte_offset = uniform_offset,
        .minimum_attribute_id = attribute_id,
        .maximum_attribute_id = attribute_id,
        .interpolation_uniform_id = uniform_id,
        .interpolation_uniform_byte_offset = interpolation_offset,
    };
}

fn colorBinding(
    comptime property: []const u8,
    comptime uniform_id: u32,
    comptime uniform_offset: u32,
    comptime min_attribute_id: u32,
    comptime max_attribute_id: u32,
    comptime interpolation_offset: u32,
) c.mln_plugin_shader_property_binding_v1 {
    return .{
        .struct_size = @sizeOf(c.mln_plugin_shader_property_binding_v1),
        .property_name = str(property),
        .encoding = c.MLN_PLUGIN_PROPERTY_ENCODING_COLOR,
        .uniform_id = uniform_id,
        .uniform_byte_offset = uniform_offset,
        .minimum_attribute_id = min_attribute_id,
        .maximum_attribute_id = max_attribute_id,
        .interpolation_uniform_id = uniform_id,
        .interpolation_uniform_byte_offset = interpolation_offset,
    };
}

const accuracy_property_bindings = [_]c.mln_plugin_shader_property_binding_v1{
    floatBinding("accuracy-radius", accuracy_uniform_id, @offsetOf(AccuracyUBO, "accuracy_radius"), 4, @offsetOf(AccuracyUBO, "accuracy_radius_t")),
    floatBinding("accuracy-border-width", accuracy_uniform_id, @offsetOf(AccuracyUBO, "accuracy_border_width"), 5, @offsetOf(AccuracyUBO, "accuracy_border_width_t")),
    colorBinding("accuracy-color", accuracy_uniform_id, @offsetOf(AccuracyUBO, "accuracy_color"), 6, 7, @offsetOf(AccuracyUBO, "accuracy_color_t")),
    colorBinding("accuracy-border-color", accuracy_uniform_id, @offsetOf(AccuracyUBO, "accuracy_border_color"), 8, 9, @offsetOf(AccuracyUBO, "accuracy_border_color_t")),
};

const sector_property_bindings = [_]c.mln_plugin_shader_property_binding_v1{
    floatBinding("bearing", sector_uniform_id, @offsetOf(SectorUBO, "bearing"), 3, @offsetOf(SectorUBO, "bearing_t")),
    floatBinding("bearing-accuracy", sector_uniform_id, @offsetOf(SectorUBO, "bearing_accuracy"), 4, @offsetOf(SectorUBO, "bearing_accuracy_t")),
    floatBinding("bearing-accuracy-radius", sector_uniform_id, @offsetOf(SectorUBO, "bearing_accuracy_radius"), 5, @offsetOf(SectorUBO, "bearing_accuracy_radius_t")),
    floatBinding("bearing-visible", sector_uniform_id, @offsetOf(SectorUBO, "bearing_visible"), 6, @offsetOf(SectorUBO, "bearing_visible_t")),
    colorBinding("bearing-accuracy-color", sector_uniform_id, @offsetOf(SectorUBO, "bearing_accuracy_color"), 7, 8, @offsetOf(SectorUBO, "bearing_accuracy_color_t")),
};

const shadow_property_bindings = [_]c.mln_plugin_shader_property_binding_v1{
    floatBinding("shadow-radius", shadow_uniform_id, @offsetOf(ShadowUBO, "shadow_radius"), 3, @offsetOf(ShadowUBO, "shadow_radius_t")),
    colorBinding("shadow-color", shadow_uniform_id, @offsetOf(ShadowUBO, "shadow_color"), 4, 5, @offsetOf(ShadowUBO, "shadow_color_t")),
};

const pulse_property_bindings = [_]c.mln_plugin_shader_property_binding_v1{
    floatBinding("puck-radius", pulse_uniform_id, @offsetOf(PulseUBO, "puck_radius"), 3, @offsetOf(PulseUBO, "puck_radius_t")),
    floatBinding("pulse-radius", pulse_uniform_id, @offsetOf(PulseUBO, "pulse_radius"), 4, @offsetOf(PulseUBO, "pulse_radius_t")),
    floatBinding("pulse-period", pulse_uniform_id, @offsetOf(PulseUBO, "pulse_period"), 5, @offsetOf(PulseUBO, "pulse_period_t")),
    colorBinding("pulse-color", pulse_uniform_id, @offsetOf(PulseUBO, "pulse_color"), 6, 7, @offsetOf(PulseUBO, "pulse_color_t")),
};

const arrow_property_bindings = [_]c.mln_plugin_shader_property_binding_v1{
    floatBinding("bearing", arrow_uniform_id, @offsetOf(ArrowUBO, "bearing"), 3, @offsetOf(ArrowUBO, "bearing_t")),
    floatBinding("puck-radius", arrow_uniform_id, @offsetOf(ArrowUBO, "puck_radius"), 4, @offsetOf(ArrowUBO, "puck_radius_t")),
    floatBinding("puck-border-width", arrow_uniform_id, @offsetOf(ArrowUBO, "puck_border_width"), 5, @offsetOf(ArrowUBO, "puck_border_width_t")),
    floatBinding("bearing-visible", arrow_uniform_id, @offsetOf(ArrowUBO, "bearing_visible"), 6, @offsetOf(ArrowUBO, "bearing_visible_t")),
    colorBinding("bearing-arrow-color", arrow_uniform_id, @offsetOf(ArrowUBO, "bearing_arrow_color"), 7, 8, @offsetOf(ArrowUBO, "bearing_arrow_color_t")),
};

const puck_property_bindings = [_]c.mln_plugin_shader_property_binding_v1{
    floatBinding("puck-radius", puck_uniform_id, @offsetOf(PuckUBO, "puck_radius"), 3, @offsetOf(PuckUBO, "puck_radius_t")),
    floatBinding("puck-border-width", puck_uniform_id, @offsetOf(PuckUBO, "puck_border_width"), 4, @offsetOf(PuckUBO, "puck_border_width_t")),
    colorBinding("puck-color", puck_uniform_id, @offsetOf(PuckUBO, "puck_color"), 5, 6, @offsetOf(PuckUBO, "puck_color_t")),
    colorBinding("puck-border-color", puck_uniform_id, @offsetOf(PuckUBO, "puck_border_color"), 7, 8, @offsetOf(PuckUBO, "puck_border_color_t")),
};

fn shaderDescriptor(
    comptime shader_id: []const u8,
    comptime sources: *const [3]c.mln_plugin_shader_source_v1,
    comptime attributes_ptr: []const c.mln_plugin_shader_attribute_v1,
    comptime uniform_blocks_ptr: []const c.mln_plugin_uniform_block_descriptor_v1,
    comptime property_bindings_ptr: []const c.mln_plugin_shader_property_binding_v1,
) c.mln_plugin_shader_descriptor_v1 {
    return .{
        .struct_size = @sizeOf(c.mln_plugin_shader_descriptor_v1),
        .shader_id = str(shader_id),
        .sources = sources,
        .source_count = sources.len,
        .attributes = attributes_ptr.ptr,
        .attribute_count = attributes_ptr.len,
        .uniform_blocks = uniform_blocks_ptr.ptr,
        .uniform_block_count = uniform_blocks_ptr.len,
        .property_bindings = property_bindings_ptr.ptr,
        .property_binding_count = property_bindings_ptr.len,
    };
}

const shaders = [_]c.mln_plugin_shader_descriptor_v1{
    shaderDescriptor(shader_accuracy, &accuracy_sources, &accuracy_attributes, &accuracy_uniform_blocks, &accuracy_property_bindings),
    shaderDescriptor(shader_sector, &sector_sources, &sector_attributes, &sector_uniform_blocks, &sector_property_bindings),
    shaderDescriptor(shader_shadow, &shadow_sources, &shadow_attributes, &shadow_uniform_blocks, &shadow_property_bindings),
    shaderDescriptor(shader_pulse, &pulse_sources, &pulse_attributes, &pulse_uniform_blocks, &pulse_property_bindings),
    shaderDescriptor(shader_arrow, &arrow_sources, &arrow_attributes, &arrow_uniform_blocks, &arrow_property_bindings),
    shaderDescriptor(shader_puck, &puck_sources, &puck_attributes, &puck_uniform_blocks, &puck_property_bindings),
};

const camera_only_capabilities = c.MLN_PLUGIN_EXPRESSION_CAMERA;
const data_driven_capabilities =
    c.MLN_PLUGIN_EXPRESSION_CAMERA | c.MLN_PLUGIN_EXPRESSION_FEATURE |
    c.MLN_PLUGIN_EXPRESSION_COMPOSITE | c.MLN_PLUGIN_EXPRESSION_FEATURE_STATE;

fn floatProperty(
    comptime name: []const u8,
    comptime default: f32,
    comptime data_driven: bool,
    comptime minimum: ?f32,
    comptime maximum: ?f32,
) c.mln_plugin_property_descriptor_v1 {
    return .{
        .struct_size = @sizeOf(c.mln_plugin_property_descriptor_v1),
        .name = str(name),
        .type = c.MLN_PLUGIN_VALUE_FLOAT,
        .default_value = .{
            .struct_size = @sizeOf(c.mln_plugin_value),
            .type = c.MLN_PLUGIN_VALUE_FLOAT,
            .data = .{ .float_value = default },
        },
        .expression_capabilities = if (data_driven) data_driven_capabilities else camera_only_capabilities,
        .supports_transitions = 1,
        .has_minimum = if (minimum != null) 1 else 0,
        .has_maximum = if (maximum != null) 1 else 0,
        .minimum = minimum orelse 0,
        .maximum = maximum orelse 0,
        .enum_values = null,
        .enum_value_count = 0,
    };
}

fn colorProperty(
    comptime name: []const u8,
    comptime r: f32,
    comptime g: f32,
    comptime b: f32,
    comptime a: f32,
) c.mln_plugin_property_descriptor_v1 {
    return .{
        .struct_size = @sizeOf(c.mln_plugin_property_descriptor_v1),
        .name = str(name),
        .type = c.MLN_PLUGIN_VALUE_COLOR,
        .default_value = .{
            .struct_size = @sizeOf(c.mln_plugin_value),
            .type = c.MLN_PLUGIN_VALUE_COLOR,
            .data = .{ .color_value = .{ .r = r, .g = g, .b = b, .a = a } },
        },
        .expression_capabilities = camera_only_capabilities,
        .supports_transitions = 1,
        .has_minimum = 0,
        .has_maximum = 0,
        .minimum = 0,
        .maximum = 0,
        .enum_values = null,
        .enum_value_count = 0,
    };
}

const default_puck_radius = 8.0;
const default_puck_border_width = 2.0;
const default_bearing_accuracy_radius = 64.0;

const property_descriptors = [_]c.mln_plugin_property_descriptor_v1{
    .{
        .struct_size = @sizeOf(c.mln_plugin_property_descriptor_v1),
        .name = str("position"),
        .type = c.MLN_PLUGIN_VALUE_FLOAT2,
        .default_value = .{
            .struct_size = @sizeOf(c.mln_plugin_value),
            .type = c.MLN_PLUGIN_VALUE_FLOAT2,
            .data = .{ .float2_value = .{ .x = 0, .y = 0 } },
        },
        .expression_capabilities = camera_only_capabilities,
        .supports_transitions = 1,
        .has_minimum = 0,
        .has_maximum = 0,
        .minimum = 0,
        .maximum = 0,
        .enum_values = null,
        .enum_value_count = 0,
    },
    floatProperty("bearing", 0, false, null, null),
    floatProperty("bearing-visible", 0, false, 0, 1),
    floatProperty("accuracy-radius", 0, false, 0, null),
    floatProperty("accuracy-border-width", 0, false, 0, null),
    floatProperty("bearing-accuracy", 0, false, 0, 180),
    floatProperty("bearing-accuracy-radius", default_bearing_accuracy_radius, false, 0, null),
    floatProperty("shadow-radius", 0, false, 0, null),
    floatProperty("puck-radius", default_puck_radius, false, 0, null),
    floatProperty("puck-border-width", default_puck_border_width, false, 0, null),
    floatProperty("pulse-radius", 0, false, 0, null),
    floatProperty("pulse-period", 1.5, false, 0.1, null),
    colorProperty("puck-color", 0.17, 0.54, 0.94, 1.0),
    colorProperty("puck-border-color", 1.0, 1.0, 1.0, 1.0),
    colorProperty("accuracy-color", 0.17, 0.54, 0.94, 0.15),
    colorProperty("accuracy-border-color", 0.17, 0.54, 0.94, 0.4),
    colorProperty("bearing-accuracy-color", 0.17, 0.54, 0.94, 0.3),
    colorProperty("bearing-arrow-color", 1.0, 1.0, 1.0, 1.0),
    colorProperty("shadow-color", 0.0, 0.0, 0.0, 0.25),
    colorProperty("pulse-color", 0.17, 0.54, 0.94, 0.5),
};

// --------------------------------------------------------------------------
// Frame building
//
// The layer is source-free: geometry arrives per rendered frame from the
// host's frame callback, with the camera-evaluated paint values and a screen
// projection. The position property transitions on the host, so build_frame
// simply projects the evaluated position and emits the six component quads in
// frame units: a virtual tile grid of 8192 units over the viewport, mapped to
// NDC by the frame matrix written in update_uniform_block.
// --------------------------------------------------------------------------

/// Frame-unit grid over the viewport; frameMatrix maps it to NDC.
/// Mercator world-pixel projection of one indicator: every drawable's matrix
/// is the frame projection translated to the puck, and quad vertices expand to
/// screen size in the vertex shader from it.
const ring_point_count = 73;
const accuracy_vertex_count = 1 + ring_point_count * 2;
const quad_vertex_count = (quads_per_point - 1) * vertices_per_quad;
const vertices_per_point = accuracy_vertex_count + quad_vertex_count;
const accuracy_index_count = 72 * 3 + 72 * 6;
const index_count = accuracy_index_count + (quads_per_point - 1) * indices_per_quad;
const segment_count = quads_per_point + 1;

const corners = [vertices_per_quad][2]i16{ .{ 0, 0 }, .{ 1, 0 }, .{ 1, 1 }, .{ 0, 1 } };

const Frame = struct {
    vertices: [vertices_per_point]Vertex,
    indices: [index_count]u16,
    segments: [segment_count]c.mln_plugin_segment_v1,
    drawables: [quads_per_point]c.mln_plugin_drawable_descriptor_v1,
    accuracy_bindings: [4]c.mln_plugin_attribute_binding_v1,
    stream_bindings: [3]c.mln_plugin_attribute_binding_v1,
    stream: c.mln_plugin_vertex_stream_v1,
    proj_matrix: [16]f32,
    center: [2]f32,
    bearing_dir: [2]f32,
};

fn streamBinding(attribute_id: u32, byte_offset: u32) c.mln_plugin_attribute_binding_v1 {
    return .{
        .struct_size = @sizeOf(c.mln_plugin_attribute_binding_v1),
        .attribute_id = attribute_id,
        .stream_id = 0,
        .byte_offset = byte_offset,
    };
}

// build_frame may run concurrently on several render threads, one per map, so
// each thread gets its own frame storage.
threadlocal var frame_storage: ?*Frame = null;

fn frameStorage() ?*Frame {
    if (frame_storage == null) {
        const frame = allocator.create(Frame) catch return null;
        frame.* = .{
            .vertices = undefined,
            .indices = undefined,
            .segments = undefined,
            .drawables = undefined,
            .accuracy_bindings = .{
                streamBinding(0, @offsetOf(Vertex, "packed_pos")),
                streamBinding(1, @offsetOf(Vertex, "prev_pos")),
                streamBinding(2, @offsetOf(Vertex, "start_time")),
                streamBinding(3, @offsetOf(Vertex, "scale")),
            },
            .stream_bindings = .{
                streamBinding(0, @offsetOf(Vertex, "packed_pos")),
                streamBinding(1, @offsetOf(Vertex, "prev_pos")),
                streamBinding(2, @offsetOf(Vertex, "start_time")),
            },
            .stream = std.mem.zeroes(c.mln_plugin_vertex_stream_v1),
            .proj_matrix = undefined,
            .center = undefined,
            .bearing_dir = undefined,
        };
        frame_storage = frame;
    }
    return frame_storage;
}

fn findFloat2(properties: []const c.mln_plugin_property_value_v1, comptime name: []const u8, default: [2]f64) [2]f64 {
    for (properties) |property| {
        if (property.struct_size < @sizeOf(c.mln_plugin_property_value_v1)) continue;
        if (property.name.data == null or property.name.size != name.len) continue;
        if (!std.mem.eql(u8, property.name.data[0..name.len], name)) continue;
        if (property.value.type != c.MLN_PLUGIN_VALUE_FLOAT2) continue;
        return .{ property.value.data.float2_value.x, property.value.data.float2_value.y };
    }
    return default;
}

fn packOffset(value: f64) i16 {
    return @intFromFloat(std.math.clamp(@round(value), -16000.0, 16000.0));
}

fn projectMercator(ctx: *const c.mln_plugin_frame_context_v1, latitude: f64, longitude: f64) [2]f64 {
    var x: f64 = 0;
    var y: f64 = 0;
    ctx.project_mercator.?(ctx, latitude, longitude, &x, &y);
    return .{ x, y };
}

fn projectScreen(ctx: *const c.mln_plugin_frame_context_v1, latitude: f64, longitude: f64) [2]f64 {
    var x: f64 = 0;
    var y: f64 = 0;
    ctx.project_screen.?(ctx, latitude, longitude, &x, &y);
    return .{ x, y };
}

fn destination(
    ctx: *const c.mln_plugin_frame_context_v1,
    latitude: f64,
    longitude: f64,
    distance_meters: f64,
    bearing_deg: f64,
) [2]f64 {
    var out_lat: f64 = 0;
    var out_lng: f64 = 0;
    ctx.destination.?(ctx, latitude, longitude, distance_meters, bearing_deg, &out_lat, &out_lng);
    return .{ out_lat, out_lng };
}

fn buildFrame(
    context: [*c]const c.mln_plugin_frame_context_v1,
    bucket: [*c]c.mln_plugin_bucket_v1,
) callconv(.c) c.mln_plugin_status {
    if (context == null or bucket == null) return c.MLN_PLUGIN_STATUS_INVALID_ARGUMENT;
    const ctx: *const c.mln_plugin_frame_context_v1 = context;
    const out: *c.mln_plugin_bucket_v1 = bucket;
    if (ctx.struct_size < @sizeOf(c.mln_plugin_frame_context_v1) or
        out.struct_size < @sizeOf(c.mln_plugin_bucket_v1) or
        ctx.project_screen == null or ctx.project_mercator == null or ctx.destination == null or
        ctx.viewport_width == 0 or ctx.viewport_height == 0 or
        (ctx.property_count != 0 and ctx.properties == null))
    {
        return c.MLN_PLUGIN_STATUS_INVALID_ARGUMENT;
    }
    const frame = frameStorage() orelse return c.MLN_PLUGIN_STATUS_CALLBACK_ERROR;
    const props: []const c.mln_plugin_property_value_v1 =
        if (ctx.property_count != 0) ctx.properties[0..ctx.property_count] else &.{};

    const position = findFloat2(props, "position", .{ 0, 0 });
    const bearing = findFloat(props, "bearing", 0);
    const accuracy_radius = findFloat(props, "accuracy-radius", 0);
    const accuracy_border_width = findFloat(props, "accuracy-border-width", 0);

    const center_mercator = projectMercator(ctx, position[0], position[1]);
    for (&frame.proj_matrix, ctx.proj_matrix) |*dst, value| dst.* = @floatCast(value);
    frame.center = .{ @floatCast(center_mercator[0]), @floatCast(center_mercator[1]) };

    // The on-screen bearing direction comes from projecting a second point
    // along the bearing, which keeps arrows and the sector honest under pitch.
    const far = destination(ctx, position[0], position[1], 1000.0, bearing);
    const near_screen = projectScreen(ctx, position[0], position[1]);
    const far_screen = projectScreen(ctx, far[0], far[1]);
    const delta = [2]f64{ far_screen[0] - near_screen[0], far_screen[1] - near_screen[1] };
    const delta_len = @max(@sqrt(delta[0] * delta[0] + delta[1] * delta[1]), 1e-9);
    frame.bearing_dir = .{ @floatCast(delta[0] / delta_len), @floatCast(delta[1] / delta_len) };

    // Meters per mercator world pixel at the position's latitude; the shader's
    // epsilon projection turns world pixels into screen pixels.
    const cos_latitude = @max(@cos(std.math.degreesToRadians(position[0])), 1e-4);
    const meters_per_unit = earth_circumference_meters * cos_latitude / (512.0 * std.math.exp2(ctx.zoom));
    const scale: f32 = @floatCast(@max(meters_per_unit, 1e-9));

    // Screen pixels per world pixel at the position, from the frame matrix,
    // mirroring the vertex shader's epsilon projection.
    const px_per_unit = blk: {
        const m = frame.proj_matrix;
        const cx: f32 = @floatCast(center_mercator[0]);
        const cy: f32 = @floatCast(center_mercator[1]);
        const pw = m[3] * cx + m[7] * cy + m[15];
        const qw = pw + m[3];
        const p_ndc_x = (m[0] * cx + m[4] * cy + m[12]) / pw;
        const p_ndc_y = (m[1] * cx + m[5] * cy + m[13]) / pw;
        const q_ndc_x = (m[0] * (cx + 1) + m[4] * cy + m[12]) / qw;
        const q_ndc_y = (m[1] * (cx + 1) + m[5] * cy + m[13]) / qw;
        const dx = (q_ndc_x - p_ndc_x) * 0.5 * @as(f32, @floatFromInt(ctx.viewport_width));
        const dy = (q_ndc_y - p_ndc_y) * 0.5 * @as(f32, @floatFromInt(ctx.viewport_height));
        break :blk @max(@sqrt(dx * dx + dy * dy), 1e-6);
    };

    const start_time: f32 = @floatCast(ctx.time_seconds);
    const border_meters = accuracy_border_width * meters_per_unit / px_per_unit;
    const inner_radius = @max(accuracy_radius - border_meters, 0.0);

    // The accuracy circle is a ground polygon: a fill fan over the inner
    // radius and a border strip out to the accuracy radius, so it foreshortens
    // like terrain under pitch.
    frame.vertices[0] = .{ .packed_pos = .{ 0, 0 }, .prev_pos = .{ 0, 0 }, .scale = scale, .start_time = start_time };
    for (0..ring_point_count) |i| {
        const ring_bearing = @as(f64, @floatFromInt(i)) * (360.0 / 72.0);
        const outer = destination(ctx, position[0], position[1], accuracy_radius, ring_bearing);
        const outer_mercator = projectMercator(ctx, outer[0], outer[1]);
        frame.vertices[1 + i] = .{
            .packed_pos = .{
                packOffset((outer_mercator[0] - center_mercator[0]) * 2 + 1),
                packOffset((outer_mercator[1] - center_mercator[1]) * 2 + 1),
            },
            .prev_pos = .{ 0, 0 },
            .scale = scale,
            .start_time = start_time,
        };
        const inner = destination(ctx, position[0], position[1], inner_radius, ring_bearing);
        const inner_mercator = projectMercator(ctx, inner[0], inner[1]);
        frame.vertices[1 + ring_point_count + i] = .{
            .packed_pos = .{
                packOffset((inner_mercator[0] - center_mercator[0]) * 2),
                packOffset((inner_mercator[1] - center_mercator[1]) * 2),
            },
            .prev_pos = .{ 0, 0 },
            .scale = scale,
            .start_time = start_time,
        };
    }
    for (0..quad_vertex_count) |index| {
        const corner = corners[index % vertices_per_quad];
        frame.vertices[accuracy_vertex_count + index] = .{
            .packed_pos = .{ corner[0], corner[1] },
            .prev_pos = .{ 0, 0 },
            .scale = scale,
            .start_time = start_time,
        };
    }

    var index_cursor: usize = 0;
    // Border strip between the outer ring (vertices 1..73) and the inner ring
    // (vertices 74..146).
    for (1..ring_point_count) |i| {
        const outer_base: u16 = @intCast(i);
        const inner_base: u16 = @intCast(ring_point_count + i);
        frame.indices[index_cursor] = outer_base;
        frame.indices[index_cursor + 1] = inner_base;
        frame.indices[index_cursor + 2] = inner_base + 1;
        frame.indices[index_cursor + 3] = outer_base;
        frame.indices[index_cursor + 4] = inner_base + 1;
        frame.indices[index_cursor + 5] = outer_base + 1;
        index_cursor += 6;
    }
    // Fill fan over the inner ring.
    for (1..ring_point_count) |i| {
        frame.indices[index_cursor] = 0;
        frame.indices[index_cursor + 1] = @intCast(ring_point_count + i);
        frame.indices[index_cursor + 2] = @intCast(ring_point_count + i + 1);
        index_cursor += 3;
    }
    // The five screen-space quads share the quad index pattern.
    const quad_index_pattern = [indices_per_quad]u16{ 0, 1, 2, 0, 2, 3 };
    for (0..quads_per_point - 1) |quad| {
        for (quad_index_pattern, 0..) |value, j| {
            frame.indices[index_cursor + quad * indices_per_quad + j] =
                @intCast(accuracy_vertex_count + quad * vertices_per_quad + value);
        }
    }

    frame.segments[0] = .{
        .struct_size = @sizeOf(c.mln_plugin_segment_v1),
        .vertex_offset = 0,
        .index_offset = 0,
        .vertex_length = accuracy_vertex_count,
        .index_length = @intCast(72 * 6),
    };
    frame.segments[1] = .{
        .struct_size = @sizeOf(c.mln_plugin_segment_v1),
        .vertex_offset = 0,
        .index_offset = @intCast(72 * 6),
        .vertex_length = accuracy_vertex_count,
        .index_length = @intCast(72 * 3),
    };
    for (0..quads_per_point - 1) |quad| {
        frame.segments[2 + quad] = .{
            .struct_size = @sizeOf(c.mln_plugin_segment_v1),
            .vertex_offset = 0,
            .index_offset = @intCast(accuracy_index_count + quad * indices_per_quad),
            .vertex_length = vertices_per_point,
            .index_length = indices_per_quad,
        };
    }

    const drawable_specs = [quads_per_point]struct { key: u64, shader: []const u8, segment: usize, count: usize }{
        .{ .key = drawable_accuracy, .shader = shader_accuracy, .segment = 0, .count = 2 },
        .{ .key = drawable_sector, .shader = shader_sector, .segment = 2, .count = 1 },
        .{ .key = drawable_shadow, .shader = shader_shadow, .segment = 3, .count = 1 },
        .{ .key = drawable_pulse, .shader = shader_pulse, .segment = 4, .count = 1 },
        .{ .key = drawable_arrow, .shader = shader_arrow, .segment = 5, .count = 1 },
        .{ .key = drawable_puck, .shader = shader_puck, .segment = 6, .count = 1 },
    };
    for (&frame.drawables, drawable_specs) |*drawable, spec| {
        const bindings: []const c.mln_plugin_attribute_binding_v1 =
            if (spec.key == drawable_accuracy) &frame.accuracy_bindings else &frame.stream_bindings;
        drawable.* = .{
            .struct_size = @sizeOf(c.mln_plugin_drawable_descriptor_v1),
            .drawable_key = spec.key,
            .shader_id = .{ .data = spec.shader.ptr, .size = spec.shader.len },
            .attributes = bindings.ptr,
            .attribute_count = bindings.len,
            .segments = &frame.segments[spec.segment],
            .segment_count = spec.count,
        };
    }

    frame.stream = .{
        .struct_size = @sizeOf(c.mln_plugin_vertex_stream_v1),
        .stream_id = 0,
        .data = @ptrCast(&frame.vertices),
        .data_size = @sizeOf(@TypeOf(frame.vertices)),
        .vertex_count = vertices_per_point,
        .stride = @sizeOf(Vertex),
    };
    out.vertex_streams = &frame.stream;
    out.vertex_stream_count = 1;
    out.indices = &frame.indices;
    out.index_count = frame.indices.len;
    out.drawables = &frame.drawables;
    out.drawable_count = frame.drawables.len;
    out.query_radius = 0;
    out.feature_vertex_ranges = null;
    out.feature_vertex_range_count = 0;
    return c.MLN_PLUGIN_STATUS_OK;
}

// --------------------------------------------------------------------------
// Uniform updates
// --------------------------------------------------------------------------

fn timeSeconds() f32 {
    // Wrap on an hour to keep f32 precision; the shaders correct a single
    // wrap, and the wrap lands at a ring restart for the pulse.
    var ts: std.c.timespec = undefined;
    if (std.c.clock_gettime(.MONOTONIC, &ts) != 0) return 0;
    const seconds = @as(f64, @floatFromInt(ts.sec)) + @as(f64, @floatFromInt(ts.nsec)) / 1_000_000_000.0;
    return @floatCast(@mod(seconds, 3600.0));
}

/// The frame projection translated to the puck center, matching the mercator
/// offsets build_frame packs into the vertices.
fn frameMatrix(frame: ?*const Frame) [16]f32 {
    const fallback_scale = 2.0 / 8192.0;
    const frame_state = frame orelse
        return .{ fallback_scale, 0, 0, 0, 0, -fallback_scale, 0, 0, 0, 0, 1, 0, -1, 1, 0, 1 };
    const m = frame_state.proj_matrix;
    const cx = frame_state.center[0];
    const cy = frame_state.center[1];
    return .{
        m[0],                          m[1],                          m[2],                          m[3],
        m[4],                          m[5],                          m[6],                          m[7],
        m[8],                          m[9],                          m[10],                         m[11],
        m[0] * cx + m[4] * cy + m[12], m[1] * cx + m[5] * cy + m[13], m[2] * cx + m[6] * cy + m[14], m[3] * cx + m[7] * cy + m[15],
    };
}

fn writeBlock(
    comptime T: type,
    context: *const c.mln_plugin_uniform_context_v1,
    output: [*c]u8,
    output_size: usize,
) c.mln_plugin_status {
    if (output_size != @sizeOf(T)) return c.MLN_PLUGIN_STATUS_INVALID_ARGUMENT;
    var block: T = std.mem.zeroes(T);
    block.matrix = frameMatrix(frame_storage);
    block.camera = .{
        context.pixels_to_gl_units[0],
        context.pixels_to_gl_units[1],
        @floatFromInt(context.viewport_width),
        @floatFromInt(context.viewport_height),
    };
    block.frame = if (frame_storage) |frame|
        .{ @floatCast(context.bearing), timeSeconds(), frame.bearing_dir[0], frame.bearing_dir[1] }
    else
        .{ @floatCast(context.bearing), timeSeconds(), 0, 0 };
    @memcpy(output[0..@sizeOf(T)], std.mem.asBytes(&block));
    return c.MLN_PLUGIN_STATUS_OK;
}

fn updateUniformBlock(
    context: [*c]const c.mln_plugin_uniform_context_v1,
    uniform_id: c_uint,
    output: [*c]u8,
    output_size: usize,
) callconv(.c) c.mln_plugin_status {
    if (context == null or output == null) return c.MLN_PLUGIN_STATUS_INVALID_ARGUMENT;
    const ctx: *const c.mln_plugin_uniform_context_v1 = context;
    if (ctx.struct_size < @sizeOf(c.mln_plugin_uniform_context_v1)) return c.MLN_PLUGIN_STATUS_INVALID_ARGUMENT;
    return switch (uniform_id) {
        accuracy_uniform_id => writeBlock(AccuracyUBO, ctx, output, output_size),
        sector_uniform_id => writeBlock(SectorUBO, ctx, output, output_size),
        shadow_uniform_id => writeBlock(ShadowUBO, ctx, output, output_size),
        pulse_uniform_id => writeBlock(PulseUBO, ctx, output, output_size),
        arrow_uniform_id => writeBlock(ArrowUBO, ctx, output, output_size),
        puck_uniform_id => writeBlock(PuckUBO, ctx, output, output_size),
        else => c.MLN_PLUGIN_STATUS_INVALID_ARGUMENT,
    };
}

// --------------------------------------------------------------------------
// Feature queries
// --------------------------------------------------------------------------

fn findFloat(properties: []const c.mln_plugin_property_value_v1, comptime name: []const u8, default: f64) f64 {
    for (properties) |property| {
        if (property.struct_size < @sizeOf(c.mln_plugin_property_value_v1)) continue;
        if (property.name.data == null or property.name.size != name.len) continue;
        if (!std.mem.eql(u8, property.name.data[0..name.len], name)) continue;
        if (property.value.type != c.MLN_PLUGIN_VALUE_FLOAT) continue;
        return property.value.data.float_value;
    }
    return default;
}

fn shouldAnimate(
    properties: [*c]const c.mln_plugin_property_value_v1,
    property_count: usize,
) callconv(.c) u8 {
    if (property_count != 0 and properties == null) return 0;
    const props: []const c.mln_plugin_property_value_v1 =
        if (property_count != 0) properties[0..property_count] else &.{};
    return if (findFloat(props, "pulse-radius", 0) > 0) 1 else 0;
}

// --------------------------------------------------------------------------
// Registration
// --------------------------------------------------------------------------

const layer_type = c.mln_plugin_layer_type_v1{
    .struct_size = @sizeOf(c.mln_plugin_layer_type_v1),
    .layer_type = str(layer_type_name),
    .backend_mask = c.MLN_PLUGIN_BACKEND_OPENGL | c.MLN_PLUGIN_BACKEND_VULKAN | c.MLN_PLUGIN_BACKEND_METAL,
    .properties = &property_descriptors,
    .property_count = property_descriptors.len,
    .geometry_type_mask = c.MLN_PLUGIN_GEOMETRY_POINT,
    .shaders = &shaders,
    .shader_count = shaders.len,
    .create_layout = null,
    .layout_feature = null,
    .finish_layout = null,
    .destroy_layout = null,
    .query_feature = null,
    .update_uniform_block = updateUniformBlock,
    .get_query_radius = null,
    .should_animate = shouldAnimate,
    .source_free = 1,
    .build_frame = buildFrame,
};

const plugin_descriptor = c.mln_plugin_descriptor_v1{
    .struct_size = @sizeOf(c.mln_plugin_descriptor_v1),
    .abi_version = c.MLN_PLUGIN_ABI_VERSION_1,
    .plugin_id = str("org.maplibre.maplibre-native-ffi.location-puck"),
    .plugin_version = str("0.1.0"),
    .minimum_host_abi = c.MLN_PLUGIN_ABI_VERSION_1,
    .maximum_host_abi = c.MLN_PLUGIN_ABI_VERSION_1,
    .layer_types = &layer_type,
    .layer_type_count = 1,
};

pub const RegisterError = error{PluginRegistrationFailed};

/// Shared-library entry point. A consumer that cannot take a C function's
/// address loads this library, obtains the host's register function from
/// mln_plugin_get_register_function_v1, and passes it here, so the library
/// never links the host. Nothing in the export's reference tree may name a
/// maplibre-native-c symbol.
export fn mln_location_puck_register(
    register_fn: c.mln_plugin_register_function_v1,
    error_message: [*c]u8,
    error_message_capacity: usize,
) c.mln_plugin_status {
    const register_impl = register_fn orelse return c.MLN_PLUGIN_STATUS_INVALID_ARGUMENT;
    return register_impl(&plugin_descriptor, error_message, error_message_capacity);
}

/// Registers the "location-puck" layer type in process. Registration is
/// process-wide and must happen before any style that uses the layer type
/// loads. A repeated identical registration counts as success.
pub fn register() RegisterError!void {
    const register_fn = c.mln_plugin_get_register_function_v1() orelse
        return error.PluginRegistrationFailed;
    try registerWith(register_fn);
}

/// Registers through an explicit register function, reporting the host's
/// diagnostic on failure.
pub fn registerWith(register_fn: c.mln_plugin_register_function_v1) RegisterError!void {
    var error_buffer: [256]u8 = std.mem.zeroes([256]u8);
    const status = mln_location_puck_register(register_fn, &error_buffer, error_buffer.len);
    switch (status) {
        c.MLN_PLUGIN_STATUS_OK, c.MLN_PLUGIN_STATUS_ALREADY_REGISTERED => {},
        else => {
            const message = std.mem.sliceTo(&error_buffer, 0);
            std.debug.print("location-puck plugin registration failed (status {d}): {s}\n", .{ status, message });
            return error.PluginRegistrationFailed;
        },
    }
}

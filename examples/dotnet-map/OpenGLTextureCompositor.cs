using Maplibre.Native.Render;

namespace Maplibre.Native.Examples.DotnetMap;

internal sealed class OpenGLTextureCompositor : ITextureCompositor
{
    private const uint Texture2D = OpenGLBorrowedTexture.Texture2D;
    private const uint TextureMinFilter = 0x2801;
    private const uint TextureMagFilter = 0x2800;
    private const uint Linear = 0x2601;
    private const uint VertexShader = 0x8B31;
    private const uint FragmentShader = 0x8B30;
    private const uint CompileStatus = 0x8B81;
    private const uint LinkStatus = 0x8B82;
    private const uint Framebuffer = 0x8D40;
    private const uint CullFace = 0x0B44;
    private const uint DepthTest = 0x0B71;
    private const uint ScissorTest = 0x0C11;
    private const uint ColorBufferBit = 0x4000;
    private const uint Texture0 = 0x84C0;
    private const uint Triangles = 0x0004;

    private readonly OpenGLContext context;
    private uint program;
    private uint vertexArray;
    private Viewport viewport;

    public OpenGLTextureCompositor(OpenGLContext context, Viewport viewport)
    {
        this.context = context;
        this.viewport = viewport;
        context.MakeCurrentForRendering();
        try
        {
            program = CreateTextureProgram();
            vertexArray = context.GenVertexArray();
            context.UseProgram(program);
            var sampler = context.GetUniformLocation(program, "map_texture");
            if (sampler >= 0)
            {
                context.Uniform1(sampler, 0);
            }

            context.UseProgram(0);
            CheckError("initialize OpenGL texture compositor");
        }
        catch
        {
            Dispose();
            throw;
        }
    }

    public void Resize(Viewport viewport)
    {
        this.viewport = viewport;
    }

    public bool Draw(OpenGLOwnedTextureFrame frame)
    {
        if (frame.Width == 0 || frame.Height == 0)
        {
            throw new InvalidOperationException(
                "MapLibre returned an empty OpenGL owned texture frame."
            );
        }

        if (frame.Target != Texture2D)
        {
            throw new InvalidOperationException(
                $"MapLibre owned texture target is {frame.Target}, expected GL_TEXTURE_2D."
            );
        }

        DrawTexture(frame.Texture);
        return true;
    }

    public void DrawTexture(uint texture)
    {
        if (texture == 0)
        {
            throw new InvalidOperationException("OpenGL texture name is zero.");
        }

        context.MakeCurrentForRendering();
        context.BindFramebuffer(Framebuffer, 0);
        context.Disable(CullFace);
        context.Disable(DepthTest);
        context.Disable(ScissorTest);
        context.Viewport(0, 0, viewport.PhysicalWidth, viewport.PhysicalHeight);
        context.ClearColor(0.08f, 0.09f, 0.11f, 1.0f);
        context.Clear(ColorBufferBit);
        context.UseProgram(program);
        context.BindVertexArray(vertexArray);
        context.ActiveTexture(Texture0);
        context.BindTexture(Texture2D, texture);
        context.TexParameter(Texture2D, TextureMinFilter, (int)Linear);
        context.TexParameter(Texture2D, TextureMagFilter, (int)Linear);
        context.DrawArrays(Triangles, 0, 3);
        context.BindTexture(Texture2D, 0);
        context.BindVertexArray(0);
        context.UseProgram(0);
        CheckError("draw OpenGL texture");
    }

    public void Dispose()
    {
        try
        {
            context.MakeCurrentForRendering();
        }
        catch (ObjectDisposedException)
        {
            return;
        }

        if (vertexArray != 0)
        {
            context.DeleteVertexArray(vertexArray);
            vertexArray = 0;
        }

        if (program != 0)
        {
            context.DeleteProgram(program);
            program = 0;
        }
    }

    private uint CreateTextureProgram()
    {
        var vertex = CompileShader(VertexShader, VertexShaderSource(), "vertex");
        var fragment = 0u;
        var linkedProgram = 0u;
        try
        {
            fragment = CompileShader(FragmentShader, FragmentShaderSource(), "fragment");
            linkedProgram = context.CreateProgram();
            context.AttachShader(linkedProgram, vertex);
            context.AttachShader(linkedProgram, fragment);
            context.LinkProgram(linkedProgram);
            if (context.GetProgram(linkedProgram, LinkStatus) == 0)
            {
                throw new InvalidOperationException(
                    "OpenGL texture compositor link failed: "
                        + context.GetProgramInfoLog(linkedProgram)
                );
            }

            return linkedProgram;
        }
        catch
        {
            if (linkedProgram != 0)
            {
                context.DeleteProgram(linkedProgram);
            }

            throw;
        }
        finally
        {
            if (linkedProgram != 0 && vertex != 0)
            {
                context.DetachShader(linkedProgram, vertex);
            }

            if (linkedProgram != 0 && fragment != 0)
            {
                context.DetachShader(linkedProgram, fragment);
            }

            if (vertex != 0)
            {
                context.DeleteShader(vertex);
            }

            if (fragment != 0)
            {
                context.DeleteShader(fragment);
            }
        }
    }

    private uint CompileShader(uint kind, string source, string name)
    {
        var shader = context.CreateShader(kind);
        context.ShaderSource(shader, source);
        context.CompileShader(shader);
        if (context.GetShader(shader, CompileStatus) == 0)
        {
            var log = context.GetShaderInfoLog(shader);
            context.DeleteShader(shader);
            throw new InvalidOperationException(
                $"OpenGL texture compositor {name} shader compile failed: {log}"
            );
        }

        return shader;
    }

    private string VertexShaderSource() =>
        context.IsGles
            ? """
                #version 300 es
                out vec2 out_uv;
                const vec2 positions[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
                const vec2 uvs[3] = vec2[3](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));
                void main() {
                  gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
                  out_uv = uvs[gl_VertexID];
                }
                """
            : """
                #version 130
                out vec2 out_uv;
                vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
                vec2 uvs[3] = vec2[](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));
                void main() {
                  gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
                  out_uv = uvs[gl_VertexID];
                }
                """;

    private string FragmentShaderSource() =>
        context.IsGles
            ? """
                #version 300 es
                precision mediump float;
                uniform sampler2D map_texture;
                in vec2 out_uv;
                out vec4 out_color;
                void main() {
                  out_color = texture(map_texture, out_uv);
                }
                """
            : """
                #version 130
                uniform sampler2D map_texture;
                in vec2 out_uv;
                out vec4 out_color;
                void main() {
                  out_color = texture(map_texture, out_uv);
                }
                """;

    private void CheckError(string operation)
    {
        var error = context.GetError();
        if (error != 0)
        {
            throw new InvalidOperationException(
                $"{operation} failed with OpenGL error 0x{error:x}"
            );
        }
    }
}

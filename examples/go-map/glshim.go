package main

/*
#cgo pkg-config: sdl3
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <stdint.h>

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLbitfield;
typedef char GLchar;
typedef float GLfloat;

typedef void (*PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef void (*PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (*PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
typedef void (*PFNGLBINDTEXTUREPROC)(GLenum target, GLuint texture);
typedef void (*PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef void (*PFNGLCLEARPROC)(GLbitfield mask);
typedef void (*PFNGLCLEARCOLORPROC)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (*PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef GLuint (*PFNGLCREATEPROGRAMPROC)(void);
typedef GLuint (*PFNGLCREATESHADERPROC)(GLenum type);
typedef void (*PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef void (*PFNGLDELETESHADERPROC)(GLuint shader);
typedef void (*PFNGLDELETETEXTURESPROC)(GLsizei n, const GLuint *textures);
typedef void (*PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);
typedef void (*PFNGLDISABLEPROC)(GLenum cap);
typedef void (*PFNGLDRAWARRAYSPROC)(GLenum mode, GLint first, GLsizei count);
typedef void (*PFNGLFINISHPROC)(void);
typedef void (*PFNGLGENTEXTURESPROC)(GLsizei n, GLuint *textures);
typedef void (*PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef GLenum (*PFNGLGETERRORPROC)(void);
typedef void (*PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (*PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
typedef void (*PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (*PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
typedef GLint (*PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
typedef void (*PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void (*PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar *const* string, const GLint *length);
typedef void (*PFNGLTEXIMAGE2DPROC)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
typedef void (*PFNGLTEXPARAMETERIPROC)(GLenum target, GLenum pname, GLint param);
typedef void (*PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void (*PFNGLUSEPROGRAMPROC)(GLuint program);
typedef void (*PFNGLVIEWPORTPROC)(GLint x, GLint y, GLsizei width, GLsizei height);

typedef PFNGLACTIVETEXTUREPROC PFNGLActiveTexturePROC;
typedef PFNGLATTACHSHADERPROC PFNGLAttachShaderPROC;
typedef PFNGLBINDFRAMEBUFFERPROC PFNGLBindFramebufferPROC;
typedef PFNGLBINDTEXTUREPROC PFNGLBindTexturePROC;
typedef PFNGLBINDVERTEXARRAYPROC PFNGLBindVertexArrayPROC;
typedef PFNGLCLEARPROC PFNGLClearPROC;
typedef PFNGLCLEARCOLORPROC PFNGLClearColorPROC;
typedef PFNGLCOMPILESHADERPROC PFNGLCompileShaderPROC;
typedef PFNGLCREATEPROGRAMPROC PFNGLCreateProgramPROC;
typedef PFNGLCREATESHADERPROC PFNGLCreateShaderPROC;
typedef PFNGLDELETEPROGRAMPROC PFNGLDeleteProgramPROC;
typedef PFNGLDELETESHADERPROC PFNGLDeleteShaderPROC;
typedef PFNGLDELETETEXTURESPROC PFNGLDeleteTexturesPROC;
typedef PFNGLDELETEVERTEXARRAYSPROC PFNGLDeleteVertexArraysPROC;
typedef PFNGLDISABLEPROC PFNGLDisablePROC;
typedef PFNGLDRAWARRAYSPROC PFNGLDrawArraysPROC;
typedef PFNGLFINISHPROC PFNGLFinishPROC;
typedef PFNGLGENTEXTURESPROC PFNGLGenTexturesPROC;
typedef PFNGLGENVERTEXARRAYSPROC PFNGLGenVertexArraysPROC;
typedef PFNGLGETERRORPROC PFNGLGetErrorPROC;
typedef PFNGLGETPROGRAMINFOLOGPROC PFNGLGetProgramInfoLogPROC;
typedef PFNGLGETPROGRAMIVPROC PFNGLGetProgramivPROC;
typedef PFNGLGETSHADERINFOLOGPROC PFNGLGetShaderInfoLogPROC;
typedef PFNGLGETSHADERIVPROC PFNGLGetShaderivPROC;
typedef PFNGLGETUNIFORMLOCATIONPROC PFNGLGetUniformLocationPROC;
typedef PFNGLLINKPROGRAMPROC PFNGLLinkProgramPROC;
typedef PFNGLSHADERSOURCEPROC PFNGLShaderSourcePROC;
typedef PFNGLTEXIMAGE2DPROC PFNGLTexImage2DPROC;
typedef PFNGLTEXPARAMETERIPROC PFNGLTexParameteriPROC;
typedef PFNGLUNIFORM1IPROC PFNGLUniform1iPROC;
typedef PFNGLUSEPROGRAMPROC PFNGLUseProgramPROC;
typedef PFNGLVIEWPORTPROC PFNGLViewportPROC;

static PFNGLACTIVETEXTUREPROC pglActiveTexture;
static PFNGLATTACHSHADERPROC pglAttachShader;
static PFNGLBINDFRAMEBUFFERPROC pglBindFramebuffer;
static PFNGLBINDTEXTUREPROC pglBindTexture;
static PFNGLBINDVERTEXARRAYPROC pglBindVertexArray;
static PFNGLCLEARPROC pglClear;
static PFNGLCLEARCOLORPROC pglClearColor;
static PFNGLCOMPILESHADERPROC pglCompileShader;
static PFNGLCREATEPROGRAMPROC pglCreateProgram;
static PFNGLCREATESHADERPROC pglCreateShader;
static PFNGLDELETEPROGRAMPROC pglDeleteProgram;
static PFNGLDELETESHADERPROC pglDeleteShader;
static PFNGLDELETETEXTURESPROC pglDeleteTextures;
static PFNGLDELETEVERTEXARRAYSPROC pglDeleteVertexArrays;
static PFNGLDISABLEPROC pglDisable;
static PFNGLDRAWARRAYSPROC pglDrawArrays;
static PFNGLFINISHPROC pglFinish;
static PFNGLGENTEXTURESPROC pglGenTextures;
static PFNGLGENVERTEXARRAYSPROC pglGenVertexArrays;
static PFNGLGETERRORPROC pglGetError;
static PFNGLGETPROGRAMINFOLOGPROC pglGetProgramInfoLog;
static PFNGLGETPROGRAMIVPROC pglGetProgramiv;
static PFNGLGETSHADERINFOLOGPROC pglGetShaderInfoLog;
static PFNGLGETSHADERIVPROC pglGetShaderiv;
static PFNGLGETUNIFORMLOCATIONPROC pglGetUniformLocation;
static PFNGLLINKPROGRAMPROC pglLinkProgram;
static PFNGLSHADERSOURCEPROC pglShaderSource;
static PFNGLTEXIMAGE2DPROC pglTexImage2D;
static PFNGLTEXPARAMETERIPROC pglTexParameteri;
static PFNGLUNIFORM1IPROC pglUniform1i;
static PFNGLUSEPROGRAMPROC pglUseProgram;
static PFNGLVIEWPORTPROC pglViewport;

#define LOAD_GL(name) do { pgl##name = (PFNGL##name##PROC)SDL_GL_GetProcAddress("gl" #name); if (!pgl##name) return "gl" #name; } while (0)

static const char *mln_go_map_gl_load(void) {
  LOAD_GL(ActiveTexture);
  LOAD_GL(AttachShader);
  LOAD_GL(BindFramebuffer);
  LOAD_GL(BindTexture);
  LOAD_GL(BindVertexArray);
  LOAD_GL(Clear);
  LOAD_GL(ClearColor);
  LOAD_GL(CompileShader);
  LOAD_GL(CreateProgram);
  LOAD_GL(CreateShader);
  LOAD_GL(DeleteProgram);
  LOAD_GL(DeleteShader);
  LOAD_GL(DeleteTextures);
  LOAD_GL(DeleteVertexArrays);
  LOAD_GL(Disable);
  LOAD_GL(DrawArrays);
  LOAD_GL(Finish);
  LOAD_GL(GenTextures);
  LOAD_GL(GenVertexArrays);
  LOAD_GL(GetError);
  LOAD_GL(GetProgramInfoLog);
  LOAD_GL(GetProgramiv);
  LOAD_GL(GetShaderInfoLog);
  LOAD_GL(GetShaderiv);
  LOAD_GL(GetUniformLocation);
  LOAD_GL(LinkProgram);
  LOAD_GL(ShaderSource);
  LOAD_GL(TexImage2D);
  LOAD_GL(TexParameteri);
  LOAD_GL(Uniform1i);
  LOAD_GL(UseProgram);
  LOAD_GL(Viewport);
  return NULL;
}

static void wrap_glActiveTexture(GLenum texture) { pglActiveTexture(texture); }
static void wrap_glAttachShader(GLuint program, GLuint shader) { pglAttachShader(program, shader); }
static void wrap_glBindFramebuffer(GLenum target, GLuint framebuffer) { pglBindFramebuffer(target, framebuffer); }
static void wrap_glBindTexture(GLenum target, GLuint texture) { pglBindTexture(target, texture); }
static void wrap_glBindVertexArray(GLuint array) { pglBindVertexArray(array); }
static void wrap_glClear(GLbitfield mask) { pglClear(mask); }
static void wrap_glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) { pglClearColor(red, green, blue, alpha); }
static void wrap_glCompileShader(GLuint shader) { pglCompileShader(shader); }
static GLuint wrap_glCreateProgram(void) { return pglCreateProgram(); }
static GLuint wrap_glCreateShader(GLenum type) { return pglCreateShader(type); }
static void wrap_glDeleteProgram(GLuint program) { pglDeleteProgram(program); }
static void wrap_glDeleteShader(GLuint shader) { pglDeleteShader(shader); }
static void wrap_glDeleteTexture(GLuint texture) { pglDeleteTextures(1, &texture); }
static void wrap_glDeleteVertexArray(GLuint array) { pglDeleteVertexArrays(1, &array); }
static void wrap_glDisable(GLenum cap) { pglDisable(cap); }
static void wrap_glDrawArrays(GLenum mode, GLint first, GLsizei count) { pglDrawArrays(mode, first, count); }
static void wrap_glFinish(void) { if (pglFinish) pglFinish(); }
static GLuint wrap_glGenTexture(void) { GLuint texture = 0; pglGenTextures(1, &texture); return texture; }
static GLuint wrap_glGenVertexArray(void) { GLuint array = 0; pglGenVertexArrays(1, &array); return array; }
static GLenum wrap_glGetError(void) { return pglGetError ? pglGetError() : 0; }
static void wrap_glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) { pglGetProgramInfoLog(program, bufSize, length, infoLog); }
static GLint wrap_glGetProgramiv(GLuint program, GLenum pname) { GLint params = 0; pglGetProgramiv(program, pname, &params); return params; }
static void wrap_glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) { pglGetShaderInfoLog(shader, bufSize, length, infoLog); }
static GLint wrap_glGetShaderiv(GLuint shader, GLenum pname) { GLint params = 0; pglGetShaderiv(shader, pname, &params); return params; }
static GLint wrap_glGetUniformLocation(GLuint program, const GLchar *name) { return pglGetUniformLocation(program, name); }
static void wrap_glLinkProgram(GLuint program) { pglLinkProgram(program); }
static void wrap_glShaderSource(GLuint shader, const GLchar *source, GLint length) { const GLchar *sources[1] = { source }; GLint lengths[1] = { length }; pglShaderSource(shader, 1, sources, lengths); }
static void wrap_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type) { pglTexImage2D(target, level, internalformat, width, height, border, format, type, NULL); }
static void wrap_glTexParameteri(GLenum target, GLenum pname, GLint param) { pglTexParameteri(target, pname, param); }
static void wrap_glUniform1i(GLint location, GLint v0) { pglUniform1i(location, v0); }
static void wrap_glUseProgram(GLuint program) { pglUseProgram(program); }
static void wrap_glViewport(GLint x, GLint y, GLsizei width, GLsizei height) { pglViewport(x, y, width, height); }
*/
import "C"

import (
	"fmt"
	"unsafe"
)

func glLoad() error {
	if missing := C.mln_go_map_gl_load(); missing != nil {
		return fmt.Errorf("SDL_GL_GetProcAddress failed for %s", C.GoString(missing))
	}
	return nil
}

func glActiveTexture(texture uint32) { C.wrap_glActiveTexture(C.GLenum(texture)) }
func glAttachShader(program, shader uint32) {
	C.wrap_glAttachShader(C.GLuint(program), C.GLuint(shader))
}

func glBindFramebuffer(target, framebuffer uint32) {
	C.wrap_glBindFramebuffer(C.GLenum(target), C.GLuint(framebuffer))
}

func glBindTexture(target, texture uint32) { C.wrap_glBindTexture(C.GLenum(target), C.GLuint(texture)) }
func glBindVertexArray(array uint32)       { C.wrap_glBindVertexArray(C.GLuint(array)) }
func glClear(mask uint32)                  { C.wrap_glClear(C.GLbitfield(mask)) }
func glClearColor(red, green, blue, alpha float32) {
	C.wrap_glClearColor(C.GLfloat(red), C.GLfloat(green), C.GLfloat(blue), C.GLfloat(alpha))
}
func glCompileShader(shader uint32)     { C.wrap_glCompileShader(C.GLuint(shader)) }
func glCreateProgram() uint32           { return uint32(C.wrap_glCreateProgram()) }
func glCreateShader(kind uint32) uint32 { return uint32(C.wrap_glCreateShader(C.GLenum(kind))) }
func glDeleteProgram(program uint32)    { C.wrap_glDeleteProgram(C.GLuint(program)) }
func glDeleteShader(shader uint32)      { C.wrap_glDeleteShader(C.GLuint(shader)) }
func glDeleteTexture(texture uint32)    { C.wrap_glDeleteTexture(C.GLuint(texture)) }
func glDeleteVertexArray(array uint32)  { C.wrap_glDeleteVertexArray(C.GLuint(array)) }
func glDisable(cap uint32)              { C.wrap_glDisable(C.GLenum(cap)) }
func glDrawArrays(mode uint32, first int32, count int32) {
	C.wrap_glDrawArrays(C.GLenum(mode), C.GLint(first), C.GLsizei(count))
}
func glFinish()                { C.wrap_glFinish() }
func glGenTexture() uint32     { return uint32(C.wrap_glGenTexture()) }
func glGenVertexArray() uint32 { return uint32(C.wrap_glGenVertexArray()) }
func glGetError() uint32       { return uint32(C.wrap_glGetError()) }
func glGetProgramiv(program uint32, pname uint32) int32 {
	return int32(C.wrap_glGetProgramiv(C.GLuint(program), C.GLenum(pname)))
}

func glGetShaderiv(shader uint32, pname uint32) int32 {
	return int32(C.wrap_glGetShaderiv(C.GLuint(shader), C.GLenum(pname)))
}

func glGetUniformLocation(program uint32, name string) int32 {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))
	return int32(C.wrap_glGetUniformLocation(C.GLuint(program), (*C.GLchar)(unsafe.Pointer(cName))))
}
func glLinkProgram(program uint32) { C.wrap_glLinkProgram(C.GLuint(program)) }
func glShaderSource(shader uint32, source string) {
	cSource := C.CString(source)
	defer C.free(unsafe.Pointer(cSource))
	C.wrap_glShaderSource(C.GLuint(shader), (*C.GLchar)(unsafe.Pointer(cSource)), C.GLint(len(source)))
}

func glTexImage2D(target uint32, level int32, internalFormat int32, width int32, height int32, border int32, format uint32, typ uint32) {
	C.wrap_glTexImage2D(C.GLenum(target), C.GLint(level), C.GLint(internalFormat), C.GLsizei(width), C.GLsizei(height), C.GLint(border), C.GLenum(format), C.GLenum(typ))
}

func glTexParameteri(target uint32, pname uint32, param int32) {
	C.wrap_glTexParameteri(C.GLenum(target), C.GLenum(pname), C.GLint(param))
}

func glUniform1i(location int32, value int32) {
	C.wrap_glUniform1i(C.GLint(location), C.GLint(value))
}
func glUseProgram(program uint32) { C.wrap_glUseProgram(C.GLuint(program)) }
func glViewport(x, y, width, height int32) {
	C.wrap_glViewport(C.GLint(x), C.GLint(y), C.GLsizei(width), C.GLsizei(height))
}

func glGetProgramInfoLog(program uint32) string {
	buffer := make([]byte, 1024)
	var length C.GLsizei
	C.wrap_glGetProgramInfoLog(C.GLuint(program), C.GLsizei(len(buffer)), &length, (*C.GLchar)(unsafe.Pointer(&buffer[0])))
	return string(buffer[:minInt(int(length), len(buffer))])
}

func glGetShaderInfoLog(shader uint32) string {
	buffer := make([]byte, 1024)
	var length C.GLsizei
	C.wrap_glGetShaderInfoLog(C.GLuint(shader), C.GLsizei(len(buffer)), &length, (*C.GLchar)(unsafe.Pointer(&buffer[0])))
	return string(buffer[:minInt(int(length), len(buffer))])
}

func minInt(a, b int) int {
	if a < b {
		return a
	}
	return b
}
